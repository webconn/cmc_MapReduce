#include "buffer.h"

#include <stdlib.h>
#include <string.h>

struct buffer *buf_init(int init_len)
{
        struct buffer *ret = (struct buffer *) malloc(sizeof (struct buffer));
        ret->size = init_len;
        ret->pos = 0;
        ret->buffer = (char *) malloc(init_len * sizeof (char));

        return ret;
}

static inline void buf_expand(struct buffer *buf)
{
        buf->size *= 2;
        buf->buffer = (char *) malloc(buf->size * sizeof (char));
}

int buf_is_free(struct buffer *buf)
{
        return buf->pos == 0;
}

void buf_add(struct buffer *buf, char val)
{
        buf->buffer[buf->pos++] = val;
        if (buf->pos == buf->size)
                buf_expand(buf);
}

void buf_attach(struct buffer *buf, const char *val, int len)
{
        while (buf->pos + len > buf->size)
                buf_expand(buf);
        memcpy(&buf->buffer[buf->pos], val, len);
        buf->pos += len;
}

char *buf_get(struct buffer *buf)
{
        buf->buffer[buf->pos] = '\0';
        return buf->buffer;
}

void buf_free(struct buffer *buf)
{
        free(buf->buffer);
        free(buf);
}

void buf_reset(struct buffer *buf)
{
        buf->pos = 0;
}
