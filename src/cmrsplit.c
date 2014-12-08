#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "cmrsplit.h"
#include "cmrconfig.h"

static off_t find_nearest_eol(int fd, int low_limit)
{
        off_t base_pos = lseek(fd, 0, SEEK_CUR);
        off_t eof_pos = lseek(fd, 0, SEEK_END);
        off_t cur_pos = base_pos;

        off_t offset = 10;
        off_t base_offset = 0;

        int f_search = 1, f_reverse = 1;
        off_t result = 0;

        while (f_search) {
                char buf;

                /* search in positive direction */
                lseek(fd, base_pos + base_offset, SEEK_SET);
                if (base_pos + base_offset >= eof_pos) { /* end of file reached */
                        result = eof_pos;
                        f_search = 0;
                        break;
                }

                for (int i=0; i<offset; i++) {
                        if (read(fd, &buf, 1) <= 0 || buf == '\n') { /* found EOL or EOF */              
                                result = lseek(fd, 0, SEEK_CUR);
                                f_search = 0;
                                break;
                        }
                }
                
                if (!f_search)
                        break;

                /* not in positive - search in negative */
                if (f_reverse) {
                        int local_offset = offset;

                        if (base_pos - base_offset - offset < low_limit) {
                                f_reverse = 0;
                                local_offset = offset - (base_pos - base_offset - offset - low_limit);
                                lseek(fd, low_limit, SEEK_SET);
                        } else {
                                lseek(fd, base_pos - base_offset - offset, SEEK_SET);
                        }

                        for (int i=0; i<local_offset; i++) {
                                if (read(fd, &buf, 1) <= 0 || buf == '\n') {
                                        result = lseek(fd, 0, SEEK_CUR);
                                        f_search = 0;
                                        break;
                                }
                        }
                }

                base_offset += offset;
        }

        return result;
}

struct cmr_split *cmrsplit(struct cmr_config *cfg)
{
        struct cmr_split *ret = (struct cmr_split *) malloc(sizeof (struct cmr_split));

        if (cfg->filenames_num == 0) { /* input stream parsing */
                ret->source = SPLIT_STREAM;
                ret->str_num = cfg->str_num;
        } else {
                ret->source = SPLIT_FILES;
                /* Calculate streams */
                if (cfg->filenames_num == 1) { /* single file; split it! */
                        int fd = open(cfg->filenames[0], O_RDONLY);
                        int len = lseek(fd, 0, SEEK_END);
                        lseek(fd, 0, SEEK_SET);

                        /* Calculate base length of string */
                        int piece_size = len / cfg->map_num;

                        ret->pieces_num = cfg->map_num;
                        ret->pieces = (struct cmr_split_piece *) malloc(cfg->map_num * sizeof (struct cmr_split_piece));

                        off_t start = 0;
                        off_t end = piece_size;

                        for (int i=0; i<cfg->map_num; i++) {
                                /* find start of string */
                                lseek(fd, end, SEEK_SET);
                                end = find_nearest_eol(fd, start);

                                if (start == end) {
                                        ret->pieces_num = i;
                                        break;
                                }
                                
                                ret->pieces[i].fd = fd;
                                ret->pieces[i].start = start;
                                ret->pieces[i].len = end - start;

                                start = end;
                                end += piece_size;
                        }

                        cfg->map_num = ret->pieces_num;
                        
                } else { /* multiple files; force number of mappers */
                        cfg->map_num = cfg->filenames_num;
                        
                        ret->pieces_num = cfg->map_num;
                        ret->pieces = (struct cmr_split_piece *) malloc(cfg->map_num * sizeof (struct cmr_split_piece));

                        for (int i=0; i<cfg->filenames_num; i++) {
                                int fd = open(cfg->filenames[i], O_RDONLY);
                                int len = lseek(fd, 0, SEEK_END);
                                lseek(fd, 0, SEEK_SET);

                                ret->pieces[i].fd = fd;
                                ret->pieces[i].start = 0;
                                ret->pieces[i].len = len;
                        }
                }
        }

        return ret;
}

void cmrsplit_free(struct cmr_split *f)
{
        if (f->source == SPLIT_FILES) {
                for (int i=0; i<f->pieces_num; i++)
                        close(f->pieces[i].fd);
                free(f->pieces);
        }
        free(f);
}
