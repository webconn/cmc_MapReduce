#ifndef INCLUDE_CMRMAP_H
#define INCLUDE_CMRMAP_H

/**
 * @file include/cmrmap.h
 * @brief Environment for Map nodes
 * @author WebConn
 */

#include <sys/types.h>

#include "cmrsplit.h"
#include "cmrconfig.h"

/** Map stage output structure */
struct cmr_map_output {
        int nodes_num;  /**< number of nodes created */

        pid_t *nodes;   /**< set of nodes' PIDs */
        int *outs;      /**< file descriptors of output pipes */
};

struct cmr_map_output *cmrmap(struct cmr_split *split, struct cmr_config *cfg);
void cmrmap_free(struct cmr_map_output *m);

#endif
