#include "cmrmerge.h"

/**
 * @file src/cmrmerge.c
 * @brief Merge procedure for Reduce stage
 * @author WebConn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/**
 * Basic merger of reducers' output streams
 *
 * Get strings from each reducer and redirect 'em
 * to stdout
 *
 * @param reduce Reduce stage output structure
 * @return Nothing
 */
void cmrmerge(struct cmr_reduce_output *reduce)
{
        FILE **outputs = (FILE **) malloc(reduce->reducers_num * sizeof (FILE *));
        int *eofs = (int *) malloc(reduce->reducers_num * sizeof (int));
        int num_streams = reduce->reducers_num;

        for (int i = 0; i < reduce->reducers_num; i++) {
                fcntl(reduce->outs[i], F_SETFL, O_NONBLOCK);
                outputs[i] = fdopen(reduce->outs[i], "r");
                eofs[i] = 0;
        }

        fprintf(stderr, " [MERGER] Run merger in parent process\n");

        while (num_streams > 0) {
                for (int i = 0; i < reduce->reducers_num; i++) {
                        if (eofs[i] == 1)
                                continue; 

                        int r = 0;
                        errno = 0;
                        if ((r = getc(outputs[i])) <= 0) {
                                if (r == EOF) { /* end of input stream */
                                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                                errno = 0; /* spike but works */
                                                continue; /* just avoid blocking */
                                        }

                                        //fprintf(stderr, " [MERGER] Got EOF\n");
                                        eofs[i] = 1;
                                        fclose(outputs[i]);
                                        num_streams--;
                                        continue;
                                } else {
                                        perror(" [MERGER] Error reading from pipe");
                                        exit(1);
                                }
                        }

                        /* If we are here, we have a string to redirect */
                        putc(r, stdout);

                        int oldflg = fcntl(reduce->outs[i], F_GETFL);
                        fcntl(reduce->outs[i], F_SETFL, oldflg & ~O_NONBLOCK); /* disable non-blocking mode */

                        char buffer[1024];
                        int len = 0;
                        char *res = NULL;
                        do {
                                res = fgets(buffer, 1023, outputs[i]);
                                //fprintf(stderr, " [MERGER] Get buffer: \"%c%s\"\n", r, res == NULL ? "EOF" : res);
                                fputs(buffer, stdout);
                                len = strlen(buffer);
                        } while (res != NULL && buffer[len - 1] != '\n');
                        fflush(stdout);


                        /* Set non-blocking flag again */
                        fcntl(reduce->outs[i], F_SETFL, oldflg | O_NONBLOCK);
                }
        }

        free(outputs);
        free(eofs);
}
