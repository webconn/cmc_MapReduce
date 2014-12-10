#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "cmrconfig.h"
#include "cmrsplit.h"
#include "cmrio.h"
#include "ui.h"

int main(int argc, char *argv[])
{
        struct cmr_config *config = ui_parse(argc, argv);

        if (config->filenames_num > 0) {
                printf("Files to parse: \n");
                for (int i=0; i<config->filenames_num; i++) {
                        printf("%s\n", config->filenames[i]);
                }
        } else {
                printf("Parsing input stream, number of lines: %d\n", config->str_num);
        }

        /* Split input stream */
        struct cmr_split *split = cmrsplit(config);
  
        if (!split) {
                fprintf(stderr, " [SPLIT] FATAL: Error processing chunks list\n");
                cmrconfig_free(config);
                exit(1);
        }
        if (split->source == SPLIT_FILES) {
                printf("Configuration of files:\n");
                for (int i=0; i<split->chunks_num; i++) {
                        //printf("Piece %02d, fd %d, start %d, size %d\n", i, split->chunks[i].fd, split->chunks[i].start, split->chunks[i].len);

                        /* print current piece */
                        lseek(split->chunks[i].fd, split->chunks[i].start, SEEK_SET);
                        cmrresend(split->chunks[i].fd, 1, split->chunks[i].len);
                }
        }

        cmrconfig_free(config);
        cmrsplit_free(split);

        return 0;
}
