#include "cmrio.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
