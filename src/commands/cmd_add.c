#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static int add_file(Index *index, const char *path) {
    // 1. Check for existing version in index for delta compression
    char base_sha[65] = "";
    int low = 0, high = (int)index->count - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(index->entries[mid].path, path);
        if (cmp == 0) {
            for (int j = 0; j < 32; j++) {
                snprintf(base_sha + (j * 2), sizeof(base_sha) - (j * 2), "%02x", index->entries[mid].sha256[j]);
            }
            base_sha[64] = 0;
            break;
        }
        if (cmp < 0) low = mid + 1;
        else high = mid - 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }
    if (size > 0) {
        if (fread(buf, 1, size, f) != size) {
            free(buf);
            fclose(f);
            return -1;
        }
    }
    fclose(f);

    char *sha256_hex = write_object_ext(buf, size, OBJ_BLOB, strlen(base_sha) > 0 ? base_sha : NULL);
    if (!sha256_hex) {
        free(buf);
        return -1;
    }

    uint8_t sha256_bytes[32];
    for (int i = 0; i < 32; i++) {
        unsigned int val;
        sscanf(sha256_hex + (i * 2), "%02x", &val);
        sha256_bytes[i] = (uint8_t)val;
    }

    add_to_index(index, path, sha256_bytes, (uint32_t)size);
    
    free(buf);
    free(sha256_hex);
    return 0;
}

static int add_recursive(Index *index, const char *path) {
    // Skip the .cit directory itself and anything inside it
    const char *p = path;
    while (p) {
        if (strncmp(p, ".cit", 4) == 0 && (p[4] == '\0' || p[4] == '/')) {
            return 0;
        }
        p = strchr(p, '/');
        if (p) p++;
    }

    if (is_file(path)) {
        return add_file(index, path);
    } else if (dir_exists(path)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char subpath[1024];
            if (strcmp(path, ".") == 0) {
                snprintf(subpath, sizeof(subpath), "%s", entry->d_name);
            } else {
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            }
            add_recursive(index, subpath);
        }
        closedir(dir);
    } else {
        // Path does not exist on disk, check if it was in the index and remove it (stage deletion)
        remove_from_index(index, path);
    }
    return 0;
}

int cmd_add(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Usage: cit add <path>\n");
        return 1;
    }

    Index *index = read_index();
    for (int i = 0; i < argc; i++) {
        char normalized[1024];
        strncpy(normalized, argv[i], sizeof(normalized) - 1);
        normalized[sizeof(normalized) - 1] = '\0';
        normalize_path(normalized);
        add_recursive(index, normalized);
    }
    write_index(index);
    free_index(index);
    return 0;
}
