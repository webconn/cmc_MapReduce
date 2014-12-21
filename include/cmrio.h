#ifndef INCLUDE_CMRIO_H
#define INCLUDE_CMRIO_H

/**
 * @file include/cmrio.h
 * @brief Basic input/output operations
 * @author WebConn
 */

#include <sys/types.h>

#include "cmrsplit.h"
#include "cmrmap.h"

int cmrresend(int from_fd, int to_fd, off_t size);
int cmrstream(int to_fd, char *msg, int msg_len, char **pos);

int cmr_read_chunk(struct cmr_chunk *chunk, char *buffer, int len);

#endif
