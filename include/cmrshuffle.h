#ifndef INCLUDE_CMRSHUFFLE_H
#define INCLUDE_CMRSHUFFLE_H

/**
 * @file include/cmrshuffle.h
 * @brief Shuffle stage (between Map and Reduce)
 * @author WebConn
 */

#include "cmrmap.h"
#include "cmrreduce.h"

#include <sys/types.h>

pid_t cmrshuffle(struct cmr_map_output *map, struct cmr_reduce_output *reduce);

#endif
