#ifndef INCLUDE_CMRIO_H
#define INCLUDE_CMRIO_H

#include <sys/types.h>

int cmrresend(int from_fd, int to_fd, off_t size);

#endif
