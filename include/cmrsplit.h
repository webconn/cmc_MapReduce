#ifndef INCLUDE_CMRSPLIT_H
#define INCLUDE_CMRSPLIT_H

#include <sys/types.h>

enum cmr_split_source {
        SPLIT_STREAM = 0,
        SPLIT_FILES,
};

struct cmr_split_piece {
        int fd;                 /* file descriptor*/
        size_t start;           /* starting byte */
        size_t len;             /* ending byte */
};

struct cmr_split {
        enum cmr_split_source source;   /** split source */
        
        union {
                int pieces_num;         /** number of pieces in SPLIT_FILES case */
                int str_num;            /** number of strings to send from input stream */
        };

        struct cmr_split_piece *pieces; /** set of pieces for SPLIT_FILES */
};


#endif
