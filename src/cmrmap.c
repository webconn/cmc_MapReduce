#include "cmrmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cmrio.h"
#include "cmrsplit.h"
#include "cmrconfig.h"
#include "buffer.h"

static void emit_keyvalue(char *key, char *value)
{
        int key_len = strlen(key);
        if (key_len == 0)
                return;

        int val_len = strlen(value);
        printf("w%d %d %s%s", key_len + val_len, key_len, key, value);
        //fprintf(stderr, "Emit keyvalue: %d %d %s%s\n", key_len + val_len, key_len, key, value);
}

static void cmrmap_create_node(int node_id, struct cmr_config *cfg, struct cmr_chunk *chunk, int in_fd, int out_fd)
{
        dup2(in_fd, 0);
        dup2(out_fd, 1);

        clock_t timer = clock();

        /* If there's no need in wrapper, just redirect I/O and run map function */
        if (!cfg->map_unix) {
                execvp(cfg->map_argv[0], cfg->map_argv);
                perror(" [MAP] Error running mapper node: ");
                exit(1);
        }

        /* If wrapper is required, start mapper in child process and run output serializer */
        /* UNIX-compatible serializer: key -> string from output 
         *                             value -> got from args */
        int out_pipe[2];
        pipe(out_pipe);

        pid_t chld = fork();
        if (!chld) {
                close(out_pipe[0]);
                dup2(out_pipe[1], 1);
                fprintf(stderr, " [MAP] Start mapper inside wrapper node (PID=%d)\n", getpid());
                execvp(cfg->map_argv[0], cfg->map_argv);
                perror(" [MAP] Error running mapper node: ");
                exit(1);
        }
        close(out_pipe[1]);
        dup2(out_pipe[0], 0);

        /* Read input stream, splitting by given symbols, and create key->value pairs */
        struct buffer *buf = buf_init(1024);
        int c = 0;

        //fprintf(stderr, " [MAP] Delimiter string: \"%s\"\n", cfg->map_delims);

        while ((c = getchar()) != EOF) {
                if (strchr(cfg->map_delims, c) != NULL) { /* if current symbol is a delimiter */
                        emit_keyvalue(buf_get(buf), cfg->map_value);
                        buf_reset(buf);                       
                } else {
                        buf_add(buf, c);
                }
        }
        //fprintf(stderr, " [WRAPPER] Got EOF\n");

        if (!buf_is_free(buf)) /* send last element */
                emit_keyvalue(buf_get(buf), cfg->map_value);

        int status = 0;
        while (wait(&status) >= 0);

        if (!WIFEXITED(status) || !WEXITSTATUS(status) == 0) {
                fprintf(stderr, " [MAP] Mapper node %d failed (returned status %d), time %.4f s\n", node_id, WEXITSTATUS(status), (clock() - timer) / CLOCKS_PER_SEC);
                exit(1);
        }
        
        fprintf(stderr, " [MAP] Mapper node %d exited normally, time %.4lf s\n", node_id, (double) (clock() - timer) / CLOCKS_PER_SEC);
        exit(0);
}

static inline void cmrmap_create_file_resender(struct cmr_config *cfg, struct cmr_split *split, struct cmr_map_output *maps, int *ins)
{
        pid_t p = fork();

        if (p) {
                fprintf(stderr, " [MAP] Resender node started with PID %d\n", p);
                return;
        }

        clock_t timer = clock();

        /* child process over there */

        int *eofs = (int *) malloc(split->chunks_num * sizeof (int)); /* EOF flags array */
        int num_streams = split->chunks_num;

        struct buffer **bufs = (struct buffer **) malloc(split->chunks_num * sizeof (struct buffer *));
        for (int i=0; i<split->chunks_num; i++) {
                eofs[i] = 0;
                bufs[i] = buf_init(1024);
                fcntl(ins[i], F_SETFL, O_NONBLOCK); /* set input pipes to non-blocking mode */
        }
        char **ptrs = (char **) malloc(split->chunks_num * sizeof (char *));

        char lbuf[1024];

        while (num_streams > 0) {
                for (int i=0; i<split->chunks_num; i++) {
                        if (eofs[i]) /* if current stream is ended */
                                continue;

                        /* If buffer is free, just fill it with values from chunk */
                        if (buf_is_free(bufs[i])) {
                                //fprintf(stderr, " [RESENDER] Fill buffer %d\n", i);
                                int len = cmr_read_chunk(&split->chunks[i], lbuf, 1024);
                                if (len <= 0) { /* EOF */
                                        fprintf(stderr, " [RESENDER] Reached EOF in %d\n", i);
                                        num_streams--;
                                        eofs[i] = 1;
                                        buf_free(bufs[i]);
                                        close(ins[i]);
                                        continue;
                                }
                                
                                buf_attach(bufs[i], lbuf, len);
                                ptrs[i] = NULL;
                                //fprintf(stderr, " [RESENDER] Buffer %d filled, size %d\n", i, len);
                        }

                        /* Send data to mapper node */
                        int str = cmrstream(ins[i], bufs[i]->buffer, bufs[i]->pos, &ptrs[i]);
                        if (str == 0) {
                                //fprintf(stderr, " [RESENDER] Buffer %d is empty\n", i);
                                bufs[i]->pos = 0;
                        } else if (str == -1) { /* stream error */
                                perror(" [MAP] Error streaming chunk: ");
                        }
                }
        }

        fprintf(stderr, " [MAP] Close resender node, time %.4lf\n", (double) (clock() - timer) / CLOCKS_PER_SEC);
        free(ptrs);
        free(eofs);
        free(bufs);

        exit(0);
}
#if 0
static inline void cmrmap_create_stream_resender(struct cmr_config *cfg, struct cmr_split *split, struct cmr_map_output *maps, int *ins)
{
        pid_t p = fork();

        if (p) {
                fprintf(stderr, " [MAP] Stream resender node started with PID %d\n", p);
                return;
        }

        clock_t timer = clock();

        /* child process over there */

        struct buffer **bufs = (struct buffer **) malloc(split->chunks_num * sizeof (struct buffer *));
        for (int i=0; i<split->chunks_num; i++) {
                bufs[i] = buf_init(1024);
                fcntl(ins[i], F_SETFL, O_NONBLOCK); /* set input pipes to non-blocking mode */
        }
        char **ptrs = (char **) malloc(split->chunks_num * sizeof (char *));

        char lbuf[1024];
        int str_count = split->str_num;

        while (!feof(stdin)) {
                for (int i = 0; i < map->nodes_num; i++) {
                        if (eofs[i]) /* if current stream is ended */
                                continue;

                        /* If buffer is free, just fill it with values from chunk */
                        if (buf_is_free(bufs[i])) {
                                int len = cmr_read_chunk(&split->chunks[i], lbuf, 1024);
                                if (len <= 0) { /* EOF */
                                        eofs[i] = 1;
                                        buf_free(bufs[i]);
                                        close(ins[i]);
                                        continue;
                                }
                                
                                buf_attach(bufs[i], lbuf, len);
                                ptrs[i] = NULL;
                        }

                        /* Send data to mapper node */
                        int str = cmrstream(ins[i], bufs[i]->buffer, bufs[i]->pos, &ptrs[i]);
                        if (str == 0) {
                                fprintf(stderr, " [MAP] Reached end of stream\n");
                                num_streams--;
                                bufs[i]->pos = 0; /* end of stream */
                        } else if (str == -1) { /* stream error */
                                perror(" [MAP] Error streaming chunk: ");
                        }
                }
        }

        fprintf(stderr, " [MAP] Close resender node, time %.4lf\n", (double) (clock() - timer) / CLOCKS_PER_SEC);
        free(ptrs);
        free(eofs);
        free(bufs);

        exit(0);
}
#endif
struct cmr_map_output *cmrmap(struct cmr_split *split, struct cmr_config *cfg)
{
        fprintf(stderr, " [MAP] Create %d map nodes\n", split->chunks_num);

        struct cmr_map_output *ret = (struct cmr_map_output *) malloc(sizeof (struct cmr_map_output));

        ret->nodes_num = split->chunks_num;
        ret->nodes = (pid_t *) malloc(ret->nodes_num * sizeof (pid_t));
        ret->outs = (int *) malloc(ret->nodes_num * sizeof (int));

        int *ins = (int *) malloc(ret->nodes_num * sizeof (int)); /* input pipes descriptors */

        int infd[2], outfd[2];

        /* 1. Create requied number of parser nodes */
        for (int i = 0; i < split->chunks_num; i++) {
                pipe(infd);
                pipe(outfd);
                
                ins[i] = infd[1];

                ret->outs[i] = outfd[0];
                ret->nodes[i] = fork();

                if (ret->nodes[i] == 0) { /* node */
                        for (int j = 0; j <= i; j++)
                                close(ins[j]); /* close input pipe */
                        if (split->source == SPLIT_FILES)
                                lseek(split->chunks[i].fd, split->chunks[i].start, SEEK_SET);

                        fprintf(stderr, " [MAP] Node %d: file %d, start %lld, len %lld\n", i, split->chunks[i].fd, (long long) split->chunks[i].start, (long long) split->chunks[i].len);

                        cmrmap_create_node(i, cfg, &split->chunks[i], infd[0], outfd[1]);
                }
                close(outfd[1]);
                
                fprintf(stderr, " [MAP] Node %d started with PID %d\n", i, ret->nodes[i]);
        }

        /* 2. Create resender node */
        cmrmap_create_file_resender(cfg, split, ret, ins);

        /* 3. Close input pipes for not to confuse mappers */
        for (int i = 0; i < split->chunks_num; i++) {
                close(ins[i]);
        }
        free(ins);

        /* Now parser nodes will deal with data, so just return node list */
        return ret;
}

void cmrmap_free(struct cmr_map_output *m)
{
        free(m->nodes);
        free(m->outs);
        free(m);
}
