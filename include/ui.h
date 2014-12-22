#ifndef INCLUDE_UI_H
#define INCLUDE_UI_H

/**
 * @file include/ui.h
 * @brief Console user interface (CLI) parser
 * @author WebConn
 */

#include "cmrconfig.h"

#include <stdio.h>

struct cmr_config *ui_parse(int argc, char *argv[]);

#endif
