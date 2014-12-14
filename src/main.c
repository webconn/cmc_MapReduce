#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmrconfig.h"
#include "cmrsplit.h"
#include "cmrreduce.h"
#include "cmrshuffle.h"
#include "cmrmap.h"
#include "cmrio.h"
#include "ui.h"

int main(int argc, char *argv[])
{
        /* Parse user input: configuration etc. */
        struct cmr_config *config = ui_parse(argc, argv);

        /* Split input stream */
        struct cmr_split *split = cmrsplit(config);
  
        if (!split) {
                fprintf(stderr, " [SPLIT] FATAL: Error processing chunks list\n");
                cmrconfig_free(config);
                exit(1);
        }

        /* Run mapper nodes and send input stream to them */
        struct cmr_map_output *map = cmrmap(split, config);

        /* Create reducer nodes */
        struct cmr_reduce_output *reduce = cmrreduce(config);

        /* Create shuffler to connect mappers and reducers */
        cmrshuffle(map, reduce);


        while (wait(NULL) >= 0);

        cmrconfig_free(config);
        cmrmap_free(map);
        cmrsplit_free(split);

        return 0;
}
