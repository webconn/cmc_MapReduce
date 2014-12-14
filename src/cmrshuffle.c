#include "cmrshuffle.h"

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

static void hashtable_insert(struct cmr_hashtable_key *hashtable, char *key, char *value)
{
        int hash = get_hash(key);
        //fprintf(stderr, "Pair <\"%s\", \"%s\">: hash %d\n", key, value, hash); 

        struct cmr_hashtable_key *elem = &hashtable[hash];

        while (1) {
                if (elem->key == NULL)
                        break;
                if (elem->next == NULL)
                        break;

                if (strcmp(elem->key, key) != 0)
                        elem = elem->next;

        }

        if (elem->key != NULL) {
                if (strcmp(elem->key, key) == 0) { /* key found */
                        free(key);
                } else { /* key is not found */
                        elem->next = (struct cmr_hashtable_key *) malloc(sizeof (struct cmr_hashtable_key));
                        elem = elem->next;
                        elem->key = key;
                }
        } else { /* no keys with this hash */
                elem->key = key;
        }

        if (elem->tail == NULL) {
                elem->values = (struct cmr_hashtable_value_item *) malloc(sizeof (struct cmr_hashtable_value_item));
                elem->tail = elem->values;
        } else {
                elem->tail->next = (struct cmr_hashtable_value_item *) malloc(sizeof (struct cmr_hashtable_value_item));
                elem->tail = elem->tail->next;
        }

        elem->tail->value = value;
        elem->tail->next = NULL;
        elem->num_values++;
}

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


        while (num_inputs > 0) {
                for (int i = 0; i < map->nodes_num; i++) {
                        if (eofs[i] == 1)
                                continue; 

                        int r = 0;
                        errno = 0;
                        if ((r = getc(pipes[i])) <= 0) {
                                if (r == EOF) { /* end of input stream */
                                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                                errno = 0; /* spike but works */
                                                continue; /* just avoid blocking */
                                        }

                                        fprintf(stderr, " [SHUFFLER] Got EOF\n");
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
                        char *value = (char *) malloc((lbuf.full_size - lbuf.key_size + 1) * sizeof (char));
                        
                        fread(key, lbuf.key_size, 1, pipes[i]); /* read key */
                        key[lbuf.key_size] = '\0';

                        fread(value, lbuf.full_size - lbuf.key_size, 1, pipes[i]); /* read value */
                        value[lbuf.full_size - lbuf.key_size] = '\0';

                        //fprintf(stderr, "Got keyvalue: <\"%s\", \"%s\">\n", key, value);
                        hashtable_insert(hashtable, key, value);

                        /* Set non-blocking flag again */
                        fcntl(map->outs[i], F_SETFL, oldflg | O_NONBLOCK);
                }
        }

        /* Parse hashtable and send data to reducers */
        /* Now for debugging: view hash table */
        for (int i=0; i<CONFIG_DFL_HASHTABLE_SIZE; i++) {
                if (hashtable[i].key == NULL)
                        continue;

                fprintf(stderr, "Key \"%s\", values: (%d) { ", hashtable[i].key, hashtable[i].num_values);

                struct cmr_hashtable_value_item *start = hashtable[i].values;
                while (start != NULL) {
                        fprintf(stderr, "\"%s\" ", start->value);
                        start = start->next;
                }
                fprintf(stderr, "}\n");
        }

        fprintf(stderr, " [SHUFFLE] Shuffler exit normally, time %.4lf\n", (double) (clock() - timer) / CLOCKS_PER_SEC);
        exit(0);
}
