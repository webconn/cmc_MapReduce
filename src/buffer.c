#include "buffer.h"

/**
 * @file src/buffer.c
 * @brief Dynamically resizeable buffer operations
 * @author WebConn
 */

#include <stdlib.h>
#include <string.h>

/**
 * Initialize buffer
 * @param init_len Initial buffer length
 * @return Pointer to the new buffer
 */
struct buffer *buf_init(int init_len)
{
        struct buffer *ret = (struct buffer *) malloc(sizeof (struct buffer));
        ret->size = init_len;
        ret->pos = 0;
        ret->buffer = (char *) malloc(init_len * sizeof (char));

        return ret;
}

static inline void _buf_expand(struct buffer *buf)
{
        buf->size *= 2;
        buf->buffer = (char *) malloc(buf->size * sizeof (char));
}

/**
 * Check if buffer is free
 * @param buf Pointer to buffer
 * @return 1 if buffer is free, 0 if is not
 */
int buf_is_free(struct buffer *buf)
{
        return buf->pos == 0;
}

/**
 * Add byte to the buffer
 * @param buf Pointer to buffer
 * @param val Byte to add in buffer
 * @return Nothing
 */
void buf_add(struct buffer *buf, char val)
{
        buf->buffer[buf->pos++] = val;
        if (buf->pos == buf->size)
                _buf_expand(buf);
}

/**
 * Concatenate (attach) byte array to the buffer
 * @param buf Pointer to buffer
 * @param val Pointer to byte array
 * @param len Array length
 * @return Nothing
 */
void buf_attach(struct buffer *buf, const char *val, int len)
{
        while (buf->pos + len > buf->size)
                _buf_expand(buf);
        memcpy(&buf->buffer[buf->pos], val, len);
        buf->pos += len;
}

/**
 * Expand buffer size
 * @param buf Pointer to buffer
 * @param size Size of extra space required
 * @return Nothing
 */
void buf_expand(struct buffer *buf, int size)
{
        while (buf->pos + size > buf->size)
                _buf_expand(buf);
}

/**
 * Get buffer content as C string (with termitanor symbol
 * @param buf Pointer to buffer
 * @return Pointer to buffer content (as C string)
 */
char *buf_get(struct buffer *buf)
{
        buf->buffer[buf->pos] = '\0';
        return buf->buffer;
}

/**
 * Delete buffer
 * @param buf Pointer to buffer
 * @return Nothing
 */
void buf_free(struct buffer *buf)
{
        free(buf->buffer);
        free(buf);
}

/**
 * Reset buffer (remove content)
 * @param buf Pointer to buffer
 * @return Noting
 */
void buf_reset(struct buffer *buf)
{
        buf->pos = 0;
}
