#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <time.h>

#include "cmrsplit.h"
#include "cmrconfig.h"
#include "cmrio.h"

static off_t find_nearest_eol(int fd, int low_limit)
{
        off_t base_pos = lseek(fd, 0, SEEK_CUR);
        off_t eof_pos = lseek(fd, 0, SEEK_END);

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

inline static struct cmr_split *cmrsplit_local(struct cmr_config *cfg)
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

                        ret->chunks_num = cfg->map_num;
                        ret->chunks = (struct cmr_chunk *) malloc(cfg->map_num * sizeof (struct cmr_chunk));

                        off_t start = 0;
                        off_t end = piece_size;

                        for (int i=0; i<cfg->map_num; i++) {
                                /* find start of string */
                                lseek(fd, end, SEEK_SET);
                                end = find_nearest_eol(fd, start);

                                if (start == end) {
                                        ret->chunks_num = i;
                                        break;
                                }
                                
                                ret->chunks[i].fd = fd;
                                ret->chunks[i].start = start;
                                ret->chunks[i].len = end - start;

                                start = end;
                                end += piece_size;
                        }

                        cfg->map_num = ret->chunks_num;
                        
                } else { /* multiple files; force number of mappers */
                        cfg->map_num = cfg->filenames_num;
                        
                        ret->chunks_num = cfg->map_num;
                        ret->chunks = (struct cmr_chunk *) malloc(cfg->map_num * sizeof (struct cmr_chunk));

                        for (int i=0; i<cfg->filenames_num; i++) {
                                int fd = open(cfg->filenames[i], O_RDONLY);
                                int len = lseek(fd, 0, SEEK_END);
                                lseek(fd, 0, SEEK_SET);

                                ret->chunks[i].fd = fd;
                                ret->chunks[i].start = 0;
                                ret->chunks[i].len = len;
                        }
                }
        }

        return ret;
}

inline static struct cmr_split *cmrsplit_deserialize(int fdi)
{
        struct cmr_split *ret = (struct cmr_split *) malloc(sizeof (struct cmr_split));

        FILE *fd = fdopen(fdi, "r");
        if (!fd) {
                perror(" [SPLIT] Deserialze: error opening pipe for reading: ");
                return NULL;
        }

        ret->source = SPLIT_FILES;
        ret->chunks_num = 0;

        int size = 10;
        ret->chunks = (struct cmr_chunk *) malloc(size * sizeof (struct cmr_chunk));

        char buffer[PATH_MAX + 32];
        char filename[PATH_MAX];

        int line = 0;

        while (fgets(buffer, PATH_MAX + 31, fd) != NULL) {
                line++;

                char *p = buffer;
                char *fn = filename;
                int ending = ' ';
                
                while (*p != '\0' && *p == ' ') p++;

                if (*p == '\0')
                        continue; /* this string is empty */

                if (*p == '"') {
                        p++;
                        ending = '"';
                }

                while (*p != '\0' && *p != ending) {
                        if (*p == '\\')
                                p++;
                        *fn++ = *p++;
                }

                if (*p == '\0') {
                        fprintf(stderr, " [SPLIT] Deserializer: unexpected EOL at line %d\n", line);
                        free(ret->chunks);
                        free(ret);
                        return NULL;
                }

                *fn = '\0';
                p++;

                /* Read start and len */
                off_t start;
                int len;

                if (sscanf(p, "%d%d", &start, &len) != 2) {
                        fprintf(stderr, " [SPLIT] Deserializer: unexpected EOL at line %d\n", line);
                        free(ret->chunks);
                        free(ret);
                        return NULL;
                }

                ret->chunks[ret->chunks_num].fd = open(filename, O_RDONLY);
                ret->chunks[ret->chunks_num].start = start;
                ret->chunks[ret->chunks_num].len = len;

                ret->chunks_num++;

                if (ret->chunks_num == size) {
                        size *= 2;
                        ret->chunks = (struct cmr_chunk *) realloc(ret->chunks, size * sizeof (struct cmr_chunk));
                }
        }

        return ret;
}

inline static struct cmr_split *cmrsplit_external(struct cmr_config *cfg)
{
        /* To get external splitting, just run splitter in external process and 
         * deserialize its output data */
        int pipefd[2];
        pipe(pipefd);

        clock_t timer = clock();
        pid_t spl = fork();

        if (spl == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], 1);

                /* Count arguments and create new argv */
                int num_args = 0; 
                char **tmp = cfg->split_argv;
                while (*tmp++ != NULL)
                        num_args++;

                /* Create and fill new argv */
                char **argv = (char **) malloc((num_args + cfg->filenames_num + 1) * sizeof (char *));
                for (int i = 0; i < num_args; i++)
                        argv[i] = cfg->split_argv[i];
                for (int i = 0; i < cfg->filenames_num; i++)
                        argv[num_args + i] = cfg->filenames[i];
                argv[num_args + cfg->filenames_num] = NULL;

                execvp(argv[0], argv);
                perror(" [SPLIT] Error running external splitter: ");
                exit(1);
        }

        close(pipefd[1]);
        fprintf(stderr, " [SPLIT] Run external splitter with PID %d\n", spl);

        /* Deserialize output stream */
        struct cmr_split *ret = cmrsplit_deserialize(pipefd[0]);

        int status;
        wait(&status);

        double wtime = (double) (clock() - timer) / CLOCKS_PER_SEC;

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, " [SPLIT] External process didn't exited normally, aborting...\n");
                exit(1);
        }
        fprintf(stderr, " [SPLIT] External splitter process (PID=%d) exited with status %d, time %.5lf s\n", spl, WEXITSTATUS(status), wtime);

        return ret;
}

struct cmr_split *cmrsplit(struct cmr_config *cfg)
{
        if (cfg->split_argv != NULL) {
                return cmrsplit_external(cfg);
        } else {
                return cmrsplit_local(cfg);
        }
}

void cmrsplit_free(struct cmr_split *f)
{
        if (f->source == SPLIT_FILES) {
                for (int i=0; i<f->chunks_num; i++)
                        close(f->chunks[i].fd);
                free(f->chunks);
        }
        free(f);
}
