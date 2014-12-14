#include "cmrio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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

int cmr_read_chunk(struct cmr_chunk *chunk, char *buffer, int len)
{
        off_t pos = lseek(chunk->fd, 0, SEEK_CUR);
        return read(chunk->fd, buffer, len > (chunk->start + chunk->len - pos) ? (chunk->start + chunk->len - pos) : len);
}
