#include "object.h"
#include "sha256.h"
#include "utils.h"
#include "cit_compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hash_to_hex(const uint8_t hash[32], char hex[65]) {
    for (int i = 0; i < 32; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = 0;
}

char *write_object(const void *buf, size_t len, obj_type type) {
    const char *type_str;
    switch (type) {
        case OBJ_BLOB: type_str = "blob"; break;
        case OBJ_TREE: type_str = "tree"; break;
        case OBJ_COMMIT: type_str = "commit"; break;
        default: return NULL;
    }

    // Header: "type size\0"
    char header[64];
    int header_len = sprintf(header, "%s %zu", type_str, len) + 1;

    size_t total_len = header_len + len;
    uint8_t *combined = malloc(total_len);
    if (!combined) return NULL;
    memcpy(combined, header, header_len);
    memcpy(combined + header_len, buf, len);

    SHA256_CTX ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, combined, total_len);
    sha256_final(&ctx, hash);

    char *hex = malloc(65);
    if (!hex) {
        free(combined);
        return NULL;
    }
    hash_to_hex(hash, hex);

    char dir[256];
    snprintf(dir, sizeof(dir), ".cit/objects/%.2s", hex);
    mkdir_p(dir);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, hex + 2);

    // Compress using CIT-LZ
    size_t dest_cap = cit_compress_bound(total_len);
    void *dest = malloc(dest_cap);
    size_t dest_len = 0;
    if (!dest) {
        free(combined);
        free(hex);
        return NULL;
    }

    if (cit_compress(combined, total_len, dest, &dest_len) != 0) {
        free(combined);
        free(dest);
        free(hex);
        return NULL;
    }
    free(combined);

    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(dest, 1, dest_len, f);
        fclose(f);
    }
    free(dest);
    return hex;
}

void *read_object(const char *hex, size_t *len, obj_type *type) {
    char path[512];
    snprintf(path, sizeof(path), ".cit/objects/%.2s/%s", hex, hex + 2);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long compressed_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *compressed = malloc(compressed_len);
    if (!compressed) {
        fclose(f);
        return NULL;
    }
    if (fread(compressed, 1, compressed_len, f) != (size_t)compressed_len) {
        free(compressed);
        fclose(f);
        return NULL;
    }
    fclose(f);

    // Read original size from CITZ header (offset 4)
    uint64_t original_size = *(uint64_t *)((uint8_t *)compressed + 4);
    void *dest = malloc(original_size + 1);
    if (!dest) {
        free(compressed);
        return NULL;
    }

    if (cit_decompress(compressed, compressed_len, dest, original_size) != 0) {
        free(compressed);
        free(dest);
        return NULL;
    }
    free(compressed);

    char *header = (char *)dest;
    size_t header_len = 0;
    for (size_t i = 0; i < original_size; i++) {
        if (header[i] == '\0') {
            header_len = i + 1;
            break;
        }
    }
    
    if (header_len == 0) {
        free(dest);
        return NULL;
    }

    if (strncmp(header, "blob ", 5) == 0) *type = OBJ_BLOB;
    else if (strncmp(header, "tree ", 5) == 0) *type = OBJ_TREE;
    else if (strncmp(header, "commit ", 7) == 0) *type = OBJ_COMMIT;
    else {
        free(dest);
        return NULL;
    }

    size_t data_len = original_size - header_len;
    *len = data_len;

    void *result = malloc(data_len + 1);
    if (!result) {
        free(dest);
        return NULL;
    }
    memcpy(result, (char *)dest + header_len, data_len);
    ((char *)result)[data_len] = 0;
    free(dest);
    return result;
}
