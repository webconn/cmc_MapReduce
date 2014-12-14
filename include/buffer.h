#ifndef INCLUDE_BUFFER_H
#define INCLUDE_BUFFER_H

struct buffer {
        int pos;
        int size;
        char *buffer;
};

struct buffer *buf_init(int init_len);
void buf_add(struct buffer *buf, char val);
int buf_is_free(struct buffer *buf);
void buf_attach(struct buffer *buf, const char *val, int len);
char *buf_get(struct buffer *buf);
void buf_reset(struct buffer *buf);
void buf_free(struct buffer *buf);

#endif
