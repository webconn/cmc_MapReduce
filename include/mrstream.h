#ifndef INCLUDE_MRSTREAM_H
#define INCLUDE_MRSTREAM_H

#include <stdint.h>

typedef enum {
        MR_FILE,
        MR_FILEPART,
        MR_STREAM,
        MR_SOCKET
} mr_stream_type;

typedef enum {
        MR_BYTES,
        MR_KEYVALUE
} mr_data_type;

struct mr_stream {
        mr_stream_type stype;
        mr_data_type dtype;
        void *config; /* pointer to config structure */
};

struct mr_stream *mr_open(mr_stream_type type, mr_data_type data, void *config);


#endif
