#ifndef INCLUDE_CMRREDUCE_H
#define INCLUDE_CMRREDUCE_H

#include "cmrconfig.h"

#include <sys/types.h>

struct cmr_reduce_output {
        int reducers_num;

        pid_t *reducers;
        int *ins;
        int *outs;
};

struct cmr_reduce_output *cmrreduce(struct cmr_config *cfg);
void cmrreduce_free(struct cmr_reduce_output *r);

#endif
