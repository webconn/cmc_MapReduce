#ifndef INCLUDE_CMRSPLIT_H
#define INCLUDE_CMRSPLIT_H

/**
 * @file include/cmrsplit.h
 * @brief Split configuration
 * @author WebConn
 */

#include <sys/types.h>

#include "cmrconfig.h"

/** Input data source */
enum cmr_split_source {
        SPLIT_STREAM = 0,       /**< take data from stdin of cmapreduce */
        SPLIT_FILES,            /**< take data from files */
};

/** Chunk description */
struct cmr_chunk {
        int fd;                 /**< file descriptor */
        size_t start;           /**< starting byte */
        size_t len;             /**< piece size */
};

/** Split configuration structure */
struct cmr_split {
        enum cmr_split_source source;   /**< input data source */
        
        int chunks_num;                 /**< number of chunks in SPLIT_FILES case == number of map nodes */
        int str_num;                    /**< number of strings to send from input stream */

        struct cmr_chunk *chunks;       /**< set of chunks for SPLIT_FILES */
};

struct cmr_split *cmrsplit(struct cmr_config *cfg);
void cmrsplit_free(struct cmr_split *f);

#endif
