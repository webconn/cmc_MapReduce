#include <stdio.h>
#include <stdlib.h>

#include "cmrconfig.h"
#include "cmrsplit.h"
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
                printf("Parsing input stream\n");
        }

        /* Split input stream */
        //struct cmr_split *split = cmr_split(config);
        
        cmrconfig_free(config);

        return 0;
}
