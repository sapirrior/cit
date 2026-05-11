#include "cit_compress.h"
#include "internal.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Bitstream Utilities */
typedef struct {
    uint8_t *buf;
    size_t pos;
    uint8_t bit_buf;
    int bit_count;
} BitWriter;

static void bw_init(BitWriter *bw, uint8_t *buf) {
    bw->buf = buf;
    bw->pos = 0;
    bw->bit_buf = 0;
    bw->bit_count = 0;
}

static void bw_write_bit(BitWriter *bw, int bit) {
    if (bit) bw->bit_buf |= (1 << (7 - bw->bit_count));
    bw->bit_count++;
    if (bw->bit_count == 8) {
        bw->buf[bw->pos++] = bw->bit_buf;
        bw->bit_buf = 0;
        bw->bit_count = 0;
    }
}

static void bw_write_byte(BitWriter *bw, uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        bw_write_bit(bw, (byte >> i) & 1);
    }
}

static void bw_flush(BitWriter *bw) {
    if (bw->bit_count > 0) {
        bw->buf[bw->pos++] = bw->bit_buf;
        bw->bit_buf = 0;
        bw->bit_count = 0;
    }
}

typedef struct {
    const uint8_t *buf;
    size_t pos;
    size_t size;
    uint8_t bit_buf;
    int bit_count;
} BitReader;

static void br_init(BitReader *br, const uint8_t *buf, size_t size) {
    br->buf = buf;
    br->pos = 0;
    br->size = size;
    br->bit_buf = 0;
    br->bit_count = 0;
}

static int br_read_bit(BitReader *br) {
    if (br->bit_count == 0) {
        if (br->pos >= br->size) return 0;
        br->bit_buf = br->buf[br->pos++];
        br->bit_count = 8;
    }
    int bit = (br->bit_buf >> (br->bit_count - 1)) & 1;
    br->bit_count--;
    return bit;
}

static uint8_t br_read_byte(BitReader *br) {
    uint8_t byte = 0;
    for (int i = 7; i >= 0; i--) {
        if (br_read_bit(br)) byte |= (1 << i);
    }
    return byte;
}

/* LZSS Implementation */

static uint32_t get_hash(const uint8_t *p) {
    return ((p[0] << 10) ^ (p[1] << 5) ^ p[2]) & HASH_MASK;
}

size_t cit_compress_bound(size_t src_len) {
    return src_len + (src_len / 8) + 64;
}

int cit_compress(const void *src, size_t src_len, void *dst, size_t *dst_len) {
    if (!src || !dst || !dst_len) return -1;
    const uint8_t *in = (const uint8_t *)src;
    uint8_t *out = (uint8_t *)dst;

    *(uint32_t *)out = CITZ_MAGIC;
    *(uint64_t *)(out + 4) = (uint64_t)src_len;

    BitWriter bw;
    bw_init(&bw, out + 12);

    int32_t *hash_table = malloc(HASH_SIZE * sizeof(int32_t));
    for (int i = 0; i < HASH_SIZE; i++) hash_table[i] = -1;

    size_t in_pos = 0;
    while (in_pos < src_len) {
        uint32_t match_len = 0;
        uint32_t match_off = 0;

        if (in_pos + MIN_MATCH <= src_len) {
            uint32_t h = get_hash(in + in_pos);
            int32_t pos = hash_table[h];
            if (pos != -1 && (in_pos - pos) < WINDOW_SIZE) {
                uint32_t current_len = 0;
                while (current_len < MAX_MATCH && (in_pos + current_len) < src_len && 
                       in[pos + current_len] == in[in_pos + current_len]) {
                    current_len++;
                }
                if (current_len >= MIN_MATCH) {
                    match_len = current_len;
                    match_off = (uint32_t)(in_pos - pos);
                }
            }
            hash_table[h] = (int32_t)in_pos;
        }

        if (match_len >= MIN_MATCH) {
            bw_write_bit(&bw, 1); // Match flag
            
            // Length: 8 bits (range 3-258 mapped to 0-255)
            uint8_t l = (uint8_t)(match_len - MIN_MATCH);
            bw_write_byte(&bw, l);

            // Offset: 16 bits
            bw_write_byte(&bw, (uint8_t)(match_off >> 8));
            bw_write_byte(&bw, (uint8_t)(match_off & 0xFF));

            in_pos += match_len;
        } else {
            bw_write_bit(&bw, 0); // Literal flag
            bw_write_byte(&bw, in[in_pos]);
            in_pos++;
        }
    }

    bw_flush(&bw);
    *dst_len = bw.pos + 12;
    free(hash_table);
    return 0;
}

int cit_decompress(const void *src, size_t src_len, void *dst, size_t dst_len) {
    if (!src || !dst) return -1;
    const uint8_t *in = (const uint8_t *)src;
    uint8_t *out = (uint8_t *)dst;

    if (*(uint32_t *)in != CITZ_MAGIC) return -1;
    uint64_t original_size = *(uint64_t *)(in + 4);
    if (original_size != dst_len) return -1;

    BitReader br;
    br_init(&br, in + 12, src_len - 12);

    size_t out_pos = 0;
    while (out_pos < dst_len) {
        if (br_read_bit(&br)) {
            // Match
            uint32_t length = (uint32_t)br_read_byte(&br) + MIN_MATCH;
            uint8_t off_hi = br_read_byte(&br);
            uint8_t off_lo = br_read_byte(&br);
            uint32_t offset = (off_hi << 8) | off_lo;

            if (offset > out_pos || offset == 0) return -1;

            for (uint32_t i = 0; i < length; i++) {
                out[out_pos] = out[out_pos - offset];
                out_pos++;
                if (out_pos > dst_len) return -1;
            }
        } else {
            // Literal
            out[out_pos++] = br_read_byte(&br);
        }
    }
    return 0;
}
