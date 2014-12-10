#ifndef INCLUDE_CMRSPLIT_H
#define INCLUDE_CMRSPLIT_H

#include <sys/types.h>

#include "cmrconfig.h"

enum cmr_split_source {
        SPLIT_STREAM = 0,
        SPLIT_FILES,
};

struct cmr_chunk {
        int fd;                 /* file descriptor */
        size_t start;           /* starting byte */
        size_t len;             /* piece size */
};

struct cmr_split {
        enum cmr_split_source source;   /** split source */
        
        union {
                int chunks_num;         /** number of chunks in SPLIT_FILES case */
                int str_num;            /** number of strings to send from input stream */
        };

        struct cmr_chunk *chunks; /** set of chunks for SPLIT_FILES */
};

struct cmr_split *cmrsplit(struct cmr_config *cfg);
void cmrsplit_free(struct cmr_split *f);

#endif
