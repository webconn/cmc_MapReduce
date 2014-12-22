#include "cmrio.h"

/**
 * @file src/cmrio.c
 * @brief Basic input/output operations
 * @author WebConn
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/**
 * Resend couple of bytes from one file to another
 * @param from_fd Source file descriptor
 * @param to_fd Destination file descriptor
 * @param size Number of bytes to resend
 * @return -1 if error is occured, 0 otherwise
 */
int cmrresend(int from_fd, int to_fd, off_t size)
{
        char buffer[1024];

        off_t rd = 0, wr = 0;

        while ((rd = read(from_fd, buffer, size > 1024 ? 1024 : size)) > 0) {
                size -= rd;

                int offset = 0;
                while ((wr = write(to_fd, buffer + offset, rd)) > 0) {
                        offset += wr;
                        rd -= wr;
                }

                if (rd != 0)
                        return -1;
        }

        if (size != 0)
                return -1;

        return 0;
}

/**
 * Non-blocking message streaming to file descriptor
 * @param to_fd Destination file descriptor
 * @param msg Message to stream
 * @param msg_len Message length
 * @param pos Pointer to (char *) - storage for current stream position
 * @return 0 if end of stream is reached, 1 if streaming is not ended, -1 if error occured
 */
int cmrstream(int to_fd, char *msg, int msg_len, char **pos)
{
        if (*pos == NULL)
                *pos = msg;

        int len = msg_len - (*pos - msg);
        if (len == 0)
                return 0; /* end of stream */

        int result = write(to_fd, *pos, len);
        
        if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                return 1; /* continue streaming */
        else if (result == -1)
                return -1; /* I/O error */

        *pos += result;
        return 1;
}

/**
 * Read CMapReduce chunk from file to buffer
 * @param chunk Pointer to chunk desctiptor
 * @param buffer Reading buffer
 * @param len Number of bytes to read from chunk
 * @return Number of bytes read, -1 if error is occured
 */
int cmr_read_chunk(struct cmr_chunk *chunk, char *buffer, int len)
{
        off_t pos = lseek(chunk->fd, 0, SEEK_CUR);
        return read(chunk->fd, buffer, len > (chunk->start + chunk->len - pos) ? (chunk->start + chunk->len - pos) : len);
}
