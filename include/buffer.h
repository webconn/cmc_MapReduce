#ifndef INCLUDE_BUFFER_H
#define INCLUDE_BUFFER_H

/**
 * @file include/buffer.h
 * @brief Dynamically resizeable buffer definition
 * @author WebConn
 */


/** Dynamically resizeable buffer structure */
struct buffer {
        int pos;        /**< Current position in buffer */
        int size;       /**< Buffer size */
        char *buffer;   /**< Buffer data field */
};

struct buffer *buf_init(int init_len);
void buf_add(struct buffer *buf, char val);
int buf_is_free(struct buffer *buf);
void buf_attach(struct buffer *buf, const char *val, int len);
void buf_expand(struct buffer *buf, int size);
char *buf_get(struct buffer *buf);
void buf_reset(struct buffer *buf);
void buf_free(struct buffer *buf);

#endif
