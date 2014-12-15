#include "cmrconfig.h"

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

void cmrconfig_free(struct cmr_config *f)
{
        if (f->map_argv != NULL) {
                char **m = f->map_argv;
                while (*m != NULL)
                        free(*m++);
                free(f->map_argv);
        }

        if (f->reduce_argv != NULL) {
                char **r = f->reduce_argv;
                while (*r != NULL)
                        free(*r++);
                free(f->reduce_argv);
        }
        
        if (f->split_argv != NULL) {
                char **s = f->split_argv;
                while (*s != NULL)
                        free(*s++);
                free(f->split_argv);
        }

        if (f->filenames != NULL) {
                for (int i=0; i<f->filenames_num; i++)
                        free(f->filenames[i]);

                free(f->filenames);
        }

        if ((char *) f->map_delims != (char *) CONFIG_DFL_MAP_DELIMS)
                free(f->map_delims);

        if ((char *) f->map_value != (char *) CONFIG_DFL_MAP_VALUE)
                free(f->map_value);

        free(f);
}
