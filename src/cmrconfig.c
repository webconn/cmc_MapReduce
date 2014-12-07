#include "cmrconfig.h"

#include <stdio.h>
#include <stdlib.h>

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

        free(f);
}
