#ifndef INCLUDE_UI_H
#define INCLUDE_UI_H

#include "cmrconfig.h"

void print_help(FILE *stream);
struct cmr_config *ui_parse(int argc, char *argv[]);

#endif
