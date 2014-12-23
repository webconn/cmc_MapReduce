#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

/**
 * @file include/config.h
 * @brief Default CMapReduce configuration
 * @author WebConn
 */

#define CONFIG_DFL_MAP_NUM      1               /**< Default mappers number */
#define CONFIG_DFL_REDUCE_NUM   1               /**< Default reducers number */
#define CONFIG_MIN_FILENAMES    10              /**< Minimal filenames num (for list buffer) */

#define CONFIG_DFL_MAP_DELIMS   ", \t\n"        /**< Default string delimiters for wrapper */
#define CONFIG_DFL_MAP_VALUE    "1"             /**< Default value for key-value pair in wrapper */

#define CONFIG_DFL_HASHTABLE_SIZE 1048577       /**< Default shuffler hashtable size */
#define CONFIG_DFL_HASH_SEED 257                /**< Default shuffler hashtable seed */
#define CONFIG_DFL_HEAPTABLE_ROW 2048           /**< Default heap chunks number */
#define CONFIG_DFL_HEAP_CHUNK_SIZE 1048577      /**< Default heap chunk size (in bytes) */
//#define CONFIG_DFL_HEAP_CHUNK_SIZE 1024

#endif
