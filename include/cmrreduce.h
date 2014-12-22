#ifndef INCLUDE_CMRREDUCE_H
#define INCLUDE_CMRREDUCE_H

/**
 * @file include/cmrreduce.h
 * @brief Reduce stage
 * @author WebConn
 */

#include "cmrconfig.h"

#include <sys/types.h>

/** Reduce stage output structure */
struct cmr_reduce_output {
        int reducers_num;       /**< Number of Reduce nodes */

        pid_t *reducers;        /**< Set of Reduce nodes' PIDs */
        int *ins;               /**< Set of Reduce nodes' input pipes */
        int *outs;              /**< Set of Reduce nodes' output pipes */
};

struct cmr_reduce_output *cmrreduce(struct cmr_config *cfg);
void cmrreduce_free(struct cmr_reduce_output *r);

#endif
