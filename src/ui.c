#include "ui.h"

/**
 * @file src/ui.c
 * @brief Console user interface (CLI) of CMapReduce
 * @author WebConn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define _GNU_SOURCE
#include <getopt.h>

#include "config.h"
#include "cmrconfig.h"

static const char *optstring = "-hM:m:R:r:S:l:";

static const struct option opts[] = {
        { .name = "help", .has_arg = 0, .flag = NULL, .val = 'h' },
        { .name = "map", .has_arg = 1, .flag = NULL, .val = 'M' },
        { .name = "reduce", .has_arg = 1, .flag = NULL, .val = 'R' },
        { .name = "split", .has_arg = 1, .flag = NULL, .val = 'S' },
        { .name = "num_map", .has_arg = 1, .flag = NULL, .val = 'm' },
        { .name = "num_reduce", .has_arg = 1, .flag = NULL, .val = 'r' },
        { .name = "lines", .has_arg = 1, .flag = NULL, .val = 'l' },
        { .name = "unix-mapper", .has_arg = 0, .flag = NULL, .val = 'u' },
        { .name = "unix-mapper-delim", .has_arg = 1, .flag = NULL, .val = 'd' },
        { .name = "unix-mapper-value", .has_arg = 1, .flag = NULL, .val = 'v' },
        { .name = "unix-reducer", .has_arg = 0, .flag = NULL, .val = 'U' },
        { 0 }
};

static const char *help_msg = "Usage: cmapreduce -M \"map command\" -R \"reduce comand\" [args] [input_file]\n\n"
        " -h, --help\t\t\tPrint this message\n"
        " -l, --lines=n\t\t\tNumber of lines from input stream to send to mappers from each iteration\n"
        " -M, --map=\"command\"\t\tMap command (with arguments in quotes)\n"
        " -m, --num_map n\t\tNumber of Map processes (default = %d)\n"
        " -R, --reduce=\"command\"\t\tReduce command (with argumenst in quotes)\n"
        " -r, --num_reduce n\t\tNumber of Reduce processes (default = %d)\n"
        " -S, --split=\"command\"\t\t(optional) Split command\n\n"
        "UNIX wrapper configuration:\n"
        "     --unix-mapper\t\t\tEnable UNIX-compatible wrapper for mapper\n"
        "     --unix-mapper-delim=c\t\tDelimiter character(s) for UNIX command output (to split output stream to keys)\n"
        "     --unix-mapper-value=\"value\"\tDefault value for pairs got from UNIX wrapper\n"
        "     --unix-reducer\t\t\tEnable UNIX-compatible wrapper for reducer\n\n";

static void print_help(FILE *stream)
{
        fprintf(stream, help_msg, CONFIG_DFL_MAP_NUM, CONFIG_DFL_REDUCE_NUM); 
}

static char **create_argv(char *str)
{
        int argc = 0;
        char **ret;

        char *old_str = str;

        /* 1. count args */
        int f_word = 0;
        while (*str != '\0') {
                if (!isblank(*str) && f_word == 0) {
                        f_word = 1;
                        argc++;
                } else if (isblank(*str) && f_word == 1) {
                        f_word = 0;
                }

                str++;
        }

        str = old_str;

        ret = (char **) malloc((argc + 1) * sizeof (char *)); /* +1 is for ending NULL */

        /* 2. get argc */
        f_word = 0;
        argc = 0;
        int slen = 0;
        do {
                if (!isblank(*str) && f_word == 0) {
                        f_word = 1;
                        slen = 1;
                        argc++;
                } else if ((isblank(*str) || *str == '\0') && f_word == 1) {
                        f_word = 0;
                        ret[argc - 1] = (char *) malloc((slen + 1) * sizeof (char)); /* +1 is for terminator */
                        strncpy(ret[argc - 1], str - slen, slen);
                        ret[argc - 1][slen] = '\0'; /* append terminator */
                        
                        if (*str == '\0')
                                break;
                } else if (!isblank(*str) && f_word == 1) {
                        slen++;
                } else if (*str == '\0') {
                        break;
                }

                str++;
        } while (1);

        ret[argc] = NULL;
        return ret;
}

/**
 * Parse command line arduments and create CMapReduce dynamic configuration structure
 * @param argc argc from main()
 * @param argv argv from main()
 * @return Pointer to configuration structure
 */
struct cmr_config *ui_parse(int argc, char *argv[])
{
        struct cmr_config *ret = (struct cmr_config *) malloc(sizeof (struct cmr_config));

        ret->map_num = CONFIG_DFL_MAP_NUM;
        ret->reduce_num = CONFIG_DFL_REDUCE_NUM;
        ret->map_unix = 0;
        ret->reduce_unix = 0;

        ret->map_delims = (char *) malloc(sizeof (CONFIG_DFL_MAP_DELIMS) * sizeof (char));
        strcpy(ret->map_delims, CONFIG_DFL_MAP_DELIMS);

        ret->map_value = (char *) malloc(sizeof (CONFIG_DFL_MAP_VALUE) * sizeof (char));
        strcpy(ret->map_value, CONFIG_DFL_MAP_VALUE);
        
        ret->filenames_num = 0;
        ret->filenames = (char **) malloc(CONFIG_MIN_FILENAMES * sizeof (char *));
        int filenames_size = CONFIG_MIN_FILENAMES * sizeof (char *);

        int c;
        int f_exit = 0, f_split = 0, f_map = 0, f_reduce = 0;
        int lnum = 0;

        while ((c = getopt_long(argc, argv, optstring, opts, NULL)) > 0) {
                switch (c) {
                        case 'l':
                                lnum = atoi(optarg);
                                break;
                        case 'M':
                                ret->map_argv = create_argv(optarg);
                                f_map = 1;
                                break;
                        case 'm':
                                ret->map_num = atoi(optarg);
                                break;
                        
                        case 'R':
                                ret->reduce_argv = create_argv(optarg);
                                f_reduce = 1;
                                break;
                        case 'r':
                                ret->reduce_num = atoi(optarg);
                                break;

                        case 'S':
                                ret->split_argv = create_argv(optarg);
                                f_split = 1;
                                break;
                        case 'u':
                                ret->map_unix = 1;
                                break;
                        case 'U':
                                ret->reduce_unix = 1;
                                break;
                        case 'd':
                                if (ret->map_delims)
                                        free(ret->map_delims);
                                ret->map_delims = (char *) malloc((strlen(optarg) + 1) * sizeof (char));
                                strcpy(ret->map_delims, optarg);
                                break;
                        case 'v':
                                if (ret->map_value)
                                        free(ret->map_value);
                                ret->map_value = (char *) malloc((strlen(optarg) + 1) * sizeof (char));
                                strcpy(ret->map_value, optarg);
                                break;
                        case 1: /* filename detected */
                                ret->filenames[ret->filenames_num++] = optarg;
                                if (ret->filenames_num == filenames_size) {
                                        filenames_size *= 2;
                                        ret->filenames = (char **) realloc(ret->filenames, filenames_size);
                                }
                                break;

                        case 'h':
                        default:
                                f_exit = 1;
                }
        }

        if (!f_split) {
                ret->split_argv = NULL;
        }

        if (!f_map || !f_reduce || f_exit) {
                print_help(stderr);
                cmrconfig_free(ret);
                exit(1);
        }

        if (ret->filenames_num > 0) {
                for (int i=0; i<ret->filenames_num; i++) {
                        char *tmp = ret->filenames[i];
                        ret->filenames[i] = (char *) malloc(strlen(tmp) + 1);
                        strcpy(ret->filenames[i], tmp);
                }
        } else {
                if (lnum > 0)
                        ret->str_num = lnum;
                else
                        ret->str_num = 1;
        }

        return ret;
}
