#ifndef CIT_INTERNAL_H
#define CIT_INTERNAL_H

#include <stdint.h>
#include <stddef.h>

/* CIT-LZ Constants */

#define CITZ_MAGIC 0x41544943 // "CITA" (Cit Archive)

/* LZSS Parameters */
#define WINDOW_SIZE 65536      // 64KB Sliding Window
#define WINDOW_MASK 65535
#define MIN_MATCH   3
#define MAX_MATCH   258        // Standard DEFLATE-style match length
#define HASH_SIZE   16384      // 14-bit hash table
#define HASH_MASK   16383

/* Range Coder Parameters */
#define RANGE_TOP   (1u << 24)
#define RANGE_BOT   (1u << 16)

#endif // CIT_INTERNAL_H
