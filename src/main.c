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
        
        if (split->source == SPLIT_FILES) {
                printf("Configuration of files:\n");
                for (int i=0; i<split->pieces_num; i++) {
                        //printf("Piece %02d, fd %d, start %d, size %d\n", i, split->pieces[i].fd, split->pieces[i].start, split->pieces[i].len);

                        /* print current piece */
                        lseek(split->pieces[i].fd, split->pieces[i].start, SEEK_SET);
                        cmrresend(split->pieces[i].fd, 1, split->pieces[i].len);
                }
        }

        cmrconfig_free(config);
        cmrsplit_free(split);

        return 0;
}
