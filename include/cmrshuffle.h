#ifndef INCLUDE_CMRSHUFFLE_H
#define INCLUDE_CMRSHUFFLE_H

#include "cmrmap.h"
#include "cmrreduce.h"

#include <sys/types.h>

struct cmr_hashtable_value_item {
        char *value;
        struct cmr_hashtable_value_item *next;
};

struct cmr_hashtable_key {
        char *key;
        int num_values;
        struct cmr_hashtable_value_item *values;
        struct cmr_hashtable_value_item *tail;
        struct cmr_hashtable_key *next;
};

pid_t cmrshuffle(struct cmr_map_output *map, struct cmr_reduce_output *reduce);

#endif
