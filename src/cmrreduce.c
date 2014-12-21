#include "cmrreduce.h"

/**
 * @file src/cmrreduce.c
 * @brief Reduce stage
 * @author WebConn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static inline void create_reducer_node(struct cmr_config *cfg, int in_fd, int out_fd)
{
        dup2(in_fd, 0);
        dup2(out_fd, 1);

        /* At the moment, just run reducer (without wrapper) */
        execvp(cfg->reduce_argv[0], cfg->reduce_argv);
        perror(" [REDUCE] Error running reduce node");
        exit(1);
}

/**
 * Create Reduce nodes and configure inputs and outputs
 * @param cfg Pointer to Dynamic CMapReduce configuration structure
 * @return Pointer to Reduce stage output structure
 */
struct cmr_reduce_output *cmrreduce(struct cmr_config *cfg)
{
        struct cmr_reduce_output *ret = (struct cmr_reduce_output *) malloc(sizeof (struct cmr_reduce_output));

        ret->reducers_num = cfg->reduce_num;
        ret->reducers = (pid_t *) malloc(ret->reducers_num * sizeof (pid_t));
        ret->ins = (int *) malloc(ret->reducers_num * sizeof (int));
        ret->outs = (int *) malloc(ret->reducers_num * sizeof (int));

        for (int i = 0; i < cfg->reduce_num; i++) {
                int infd[2], outfd[2];
                pipe(infd);
                pipe(outfd);

                ret->ins[i] = infd[1];
                ret->outs[i] = outfd[0];

                ret->reducers[i] = fork();

                if (!ret->reducers[i]) { /* child */
                        for (int j=0; j <= i; j++)
                                close(ret->ins[j]);
                        create_reducer_node(cfg, infd[0], outfd[1]);
                }
                close(outfd[1]);
                fprintf(stderr, " [REDUCE] Run reducer node %d, PID %d\n", i, ret->reducers[i]);
        }

        return ret;
}

/**
 * Delete Reduce stage output structure
 * @param r Pointer to Reduce stage output structure
 * @return Nothing
 */
void cmrreduce_free(struct cmr_reduce_output *r)
{
        free(r->reducers);

        for (int i = 0; i < r->reducers_num; i++)
                close(r->ins[i]);

        free(r->ins);
        free(r->outs);
        free(r);
}
