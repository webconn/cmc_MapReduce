#ifndef INCLUDE_CMRCONFIG_H
#define INCLUDE_CMRCONFIG_H

/** Configuration of CMapReduce implementation */
struct cmr_config {
        char **map_argv;        /** argv for Map function */
        int map_num;            /** number of Map streams */

        char **reduce_argv;     /** argv for Reduce function */
        int reduce_num;         /** number of Reduce streams */

        char **split_argv;      /** argv for Split function */

        int filenames_num;      /** number of files in input stream */
        int str_num;            /** number of strings from input stream to send */

        char **filenames;       /** input file name */
};

void cmrconfig_free(struct cmr_config *f);

#endif
