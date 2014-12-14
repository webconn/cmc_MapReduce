#ifndef INCLUDE_CMRMAP_H
#define INCLUDE_CMRMAP_H

#include <sys/types.h>

#include "cmrsplit.h"
#include "cmrconfig.h"

struct cmr_map_output {
        int nodes_num;

        pid_t *nodes;
        int *outs;      /* file descriptors for output pipes */
};

struct cmr_map_output *cmrmap(struct cmr_split *split, struct cmr_config *cfg);
void cmrmap_free(struct cmr_map_output *m);

#endif
