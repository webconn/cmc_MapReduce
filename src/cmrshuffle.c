#include "cmrshuffle.h"

/**
 * @file src/cmrshuffle.c
 * @brief Shuffle stage (between Map and Reduce)
 * @author WebConn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>

#include "cmrconfig.h"
#include "cmrmap.h"
#include "cmrreduce.h"
#include "config.h"

/** Value nodes of Shuffle hashtable */
struct cmr_hashtable_value_item {
        char *value;                                    /**< value */
        struct cmr_hashtable_value_item *next;          /**< next value in list */
};

/** Key nodes of Shuffle hashtable */
struct cmr_hashtable_key {
        char *key;                                      /**< key */
        int num_values;                                 /**< number of values for this key */
        struct cmr_hashtable_value_item *values;        /**< list of values */
        struct cmr_hashtable_value_item *tail;          /**< tail of list of values */
        struct cmr_hashtable_key *next;                 /**< next key in list */
};

static void **heap = NULL;
static int heap_num_chunks = 0;
static long long heap_size = 0;

static void *malloc_heap(int size)
{
        //fprintf(stderr, "  [MALLOC] Allocate %d, heap size %lld\n", size, heap_size);

        if (size <= 0)
                return NULL;

        if (heap == NULL) {
                heap = malloc(CONFIG_DFL_HEAPTABLE_ROW * sizeof (void *));
                memset(heap, 0, CONFIG_DFL_HEAPTABLE_ROW * sizeof (void *));
                heap_num_chunks = CONFIG_DFL_HEAPTABLE_ROW;
        }

        int tail = (heap_size + size) % CONFIG_DFL_HEAP_CHUNK_SIZE;
        if (tail < size) { /* if there's no space in the tail of last chunk */
                heap_size += size - tail;
        }

        int chunk_id = heap_size / CONFIG_DFL_HEAP_CHUNK_SIZE;
        if (chunk_id >= heap_num_chunks) { /* need more chunk rows */
                heap_num_chunks *= 2;
                heap = realloc(heap, heap_num_chunks * CONFIG_DFL_HEAPTABLE_ROW);
                memset(heap + (heap_num_chunks / 2), 0, (heap_num_chunks / 2) * sizeof (void *));
                if (heap == NULL) {
                        perror(" [SHUFFLER] Error allocating memory\n");
                        exit(1);
                }
        }

        if (heap[chunk_id] == NULL) {
                heap[chunk_id] = malloc(CONFIG_DFL_HEAP_CHUNK_SIZE);
                if (heap[chunk_id] == NULL) {
                        perror(" [SHUFFLER] Error allocating memory\n");
                        exit(1);
                }
        }

        void *ret = (char *) heap[chunk_id] + heap_size % CONFIG_DFL_HEAP_CHUNK_SIZE;
        heap_size += size;


        return ret;
}

static void heap_free(void)
{
        for (int i=0; i<heap_num_chunks; i++) {
                if (heap[i] != NULL)
                        free(heap[i]);
        }
        free(heap);
}

static unsigned int get_hash(const char *key)
{
        unsigned int sum = 0;
        unsigned int mul = 1;

        while (*key != '\0') {
                sum += *key * mul;
                mul *= CONFIG_DFL_HASH_SEED;
                key++;
        }

        return sum % CONFIG_DFL_HASHTABLE_SIZE;
}

static inline void hashtable_insert(struct cmr_hashtable_key *hashtable, char *key, char *value)
{
        unsigned int hash = get_hash(key);
        //fprintf(stderr, "Pair <\"%s\", \"%s\">: hash %d\n", key, value, hash); 

        struct cmr_hashtable_key *elem = &hashtable[hash];

        while (1) {
                if (elem->key == NULL)
                        break;
                if (elem->next == NULL)
                        break;

                if (strcmp(elem->key, key) != 0) {
                        elem = elem->next;
                } else {
                        break;
                }
        }

        if (elem->key != NULL) {
                if (strcmp(elem->key, key) != 0) { /* key is not found */
                        elem->next = (struct cmr_hashtable_key *) malloc_heap(sizeof (struct cmr_hashtable_key));
                        elem = elem->next;
                        elem->key = (char *) malloc_heap(strlen(key) + 1);
                        strcpy(elem->key, key);
                        elem->num_values = 0;
                        elem->next = NULL;
                }
        } else { /* no keys with this hash */
                elem->key = (char *) malloc_heap(strlen(key) + 1);
                strcpy(elem->key, key);
        }

        if (elem->tail == NULL) {
                elem->values = (struct cmr_hashtable_value_item *) malloc_heap(sizeof (struct cmr_hashtable_value_item));
                elem->tail = elem->values;
        } else {
                elem->tail->next = (struct cmr_hashtable_value_item *) malloc_heap(sizeof (struct cmr_hashtable_value_item));
                elem->tail = elem->tail->next;
        }

        elem->tail->value = value;
        elem->tail->next = NULL;
        elem->num_values++;
}

static inline void emit_keyvalues(FILE *stream, struct cmr_hashtable_key *val)
{
        int key_len = strlen(val->key);
        fprintf(stream, "w%d %s", key_len, val->key);
        fprintf(stream, " %d ", val->num_values);

        struct cmr_hashtable_value_item *elem = val->values;
        while (elem != NULL) {
                fprintf(stream, "%d %s", (int) strlen(elem->value), elem->value);
                elem = elem->next;
        }
}

/**
 * Create and configure Shuffle node
 * @param map Pointer to Map stage output structure
 * @param reduce Pointer to Reduce stage output structure
 * @return PID of Shuffle node
 */
pid_t cmrshuffle(struct cmr_map_output *map, struct cmr_reduce_output *reduce)
{
        /* Our task is to create process which will interconnect mappers and reducers */
        pid_t ret = fork();
        if (ret) {
                fprintf(stderr, " [SHUFFLER] Run shuffler process with PID %d\n", ret);
                for (int i=0; i<reduce->reducers_num; i++)
                        close(reduce->ins[i]);
                return ret;
        }

        clock_t timer = clock();

        /* 1. Create hashtable for keys */
        static struct cmr_hashtable_key hashtable[CONFIG_DFL_HASHTABLE_SIZE] = { {0} };

        /* 2. Read pairs from input streams */
        int *eofs = (int *) malloc(map->nodes_num * sizeof (int));
        FILE **pipes = (FILE **) malloc(map->nodes_num * sizeof (FILE *));
        int num_inputs = map->nodes_num;

        fprintf(stderr, " [SHUFFLER] Allocated memory for tables\n");
        
        /* First, make all inputs non-blocking, to avoid long locks */
        for (int i=0; i<map->nodes_num; i++) {
                fcntl(map->outs[i], F_SETFL, O_NONBLOCK);
                eofs[i] = 0;
                pipes[i] = fdopen(map->outs[i], "r");
        }

        struct local_buffer {
                int full_size;
                int key_size;
        } lbuf = { 0 };

        fprintf(stderr, " [SHUFFLER] Ready to deal with data\n");

        int counter = 1;
        long long heartbeats = 0;
        while (num_inputs > 0) {
                counter--;
                if (counter == 0) {
                        fprintf(stderr, " [SHUFFLER] Heartbeat %lld\n", heartbeats++);
                        counter = 5000;
                }

                for (int i = 0; i < map->nodes_num; i++) {
                        if (eofs[i] == 1)
                                continue; 

                        int r = 0;
                        errno = 0;

                        //fprintf(stderr, " [SHUFFLER] Trying to get first symbol\n");
                        if ((r = getc(pipes[i])) <= 0) {
                                if (r == EOF) { /* end of input stream */
                                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                                //fprintf(stderr, " [SHUFFLER] Pipe is empty, waiting...\n");
                                                errno = 0; /* spike but works */
                                                continue; /* just avoid blocking */
                                        }

                                        fprintf(stderr, " [SHUFFLER] Got EOF from node %d\n", i);
                                        eofs[i] = 1;
                                        fclose(pipes[i]);
                                        num_inputs--;
                                        continue;
                                } else {
                                        perror(" [SHUFFLER] Error reading from pipe");
                                        exit(1);
                                }
                        }

                        /* If we are here, we have a packet to read */
                        int oldflg = fcntl(map->outs[i], F_GETFL);
                        fcntl(map->outs[i], F_SETFL, oldflg & ~O_NONBLOCK); /* disable non-blocking mode */

                        if (fscanf(pipes[i], "%d%d", &lbuf.full_size, &lbuf.key_size) != 2) {
                                fprintf(stderr, " [SHUFFLER] Error reading packet, wrong format?\n");
                                exit(1);
                        }
                        fgetc(pipes[i]); /* skip extra space symbol */

                        /* Allocate memory for buffer */
                        char *key = (char *) malloc((lbuf.key_size + 1) * sizeof (char));
                        char *value = (char *) malloc_heap((lbuf.full_size - lbuf.key_size + 1) * sizeof (char));
                        
                        fread(key, lbuf.key_size, 1, pipes[i]); /* read key */
                        key[lbuf.key_size] = '\0';

                        fread(value, lbuf.full_size - lbuf.key_size, 1, pipes[i]); /* read value */
                        value[lbuf.full_size - lbuf.key_size] = '\0';

                        //fprintf(stderr, " [SHUFFLE] = Got keyvalue from node %d: <\"%s\", \"%s\">\n", i, key, value);
                        hashtable_insert(hashtable, key, value);
                        free(key);

                        /* Set non-blocking flag again */
                        fcntl(map->outs[i], F_SETFL, oldflg | O_NONBLOCK);
                        //fprintf(stderr, " [SHUFFLE] Ready to get next key\n");
                }
        }
        fprintf(stderr, " [SHUFFLER] Reading completed, send key-values to reducer nodes\n");

        /* Now for debugging: view hash table */
        #if 0
        for (int i=0; i<CONFIG_DFL_HASHTABLE_SIZE; i++) {
                if (hashtable[i].key == NULL)
                        continue;
                
                struct cmr_hashtable_key *elem = &hashtable[i];
                while (elem != NULL) {
                        fprintf(stderr, " [HASH] == (Hash %d) Key \"%s\", values: (%d) { ", i, elem->key, elem->num_values);

                        struct cmr_hashtable_value_item *start = elem->values;
                        while (start != NULL) {
                                fprintf(stderr, "\"%s\" ", start->value);
                                start = start->next;
                        }
                        fprintf(stderr, "}\n");
                        elem = elem->next;
                }
        }
        #endif

        free(pipes);
        free(eofs);

        pipes = (FILE **) malloc(reduce->reducers_num * sizeof (FILE *));
        for (int i = 0; i < reduce->reducers_num; i++) {
                pipes[i] = fdopen(reduce->ins[i], "w");
        }

        /* Parse hashtable and send data to reducers */

        for (int i=0; i<CONFIG_DFL_HASHTABLE_SIZE; i++) {
                if (hashtable[i].key == NULL)
                        continue;

                struct cmr_hashtable_key *elem = &hashtable[i];
                while (elem != NULL) {
                        counter--;
                        if (counter == 0) {
                                fprintf(stderr, " [SHUFFLER] Heartbeat %lld\n", heartbeats++);
                                counter = 5000;
                        }
                        /* Emit key-values */
                        emit_keyvalues(pipes[i % reduce->reducers_num], elem);
                        elem = elem->next;
                }
        }

        for (int i = 0; i < reduce->reducers_num; i++) {
                fclose(pipes[i]);
        }
        free(pipes);
        heap_free();

        fprintf(stderr, " [SHUFFLE] Shuffler exit normally, time %.4lf, memory usage %lld bytes\n", (double) (clock() - timer) / CLOCKS_PER_SEC, heap_size);
        exit(0);
}
