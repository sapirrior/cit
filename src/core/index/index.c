#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int compare_entries(const void *a, const void *b) {
    return strcmp(((IndexEntry *)a)->path, ((IndexEntry *)b)->path);
}

Index *read_index() {
    Index *index = malloc(sizeof(Index));
    if (!index) return NULL;
    index->count = 0;
    index->entries = NULL;

    FILE *f = fopen(".cit/index", "rb");
    if (!f) return index;

    if (fread(&index->count, sizeof(uint32_t), 1, f) == 1 && index->count > 0) {
        index->entries = malloc(sizeof(IndexEntry) * index->count);
        if (index->entries) {
            fread(index->entries, sizeof(IndexEntry), index->count, f);
            // Index should already be sorted on disk, but we can verify if needed
        } else {
            index->count = 0;
        }
    }
    fclose(f);
    return index;
}

int write_index(Index *index) {
    FILE *f = fopen(".cit/index", "wb");
    if (!f) return -1;

    // Optional: Sort before writing to ensure disk integrity
    if (index->count > 1) {
        qsort(index->entries, index->count, sizeof(IndexEntry), compare_entries);
    }

    fwrite(&index->count, sizeof(uint32_t), 1, f);
    if (index->count > 0) {
        fwrite(index->entries, sizeof(IndexEntry), index->count, f);
    }
    fclose(f);
    return 0;
}

void free_index(Index *index) {
    if (index->entries) free(index->entries);
    free(index);
}

int add_to_index(Index *index, const char *path, const uint8_t sha256[32], uint32_t size) {
    int low = 0, high = (int)index->count - 1;
    int mid = 0, cmp;
    int found = 0;

    while (low <= high) {
        mid = low + (high - low) / 2;
        cmp = strcmp(index->entries[mid].path, path);
        if (cmp == 0) {
            low = mid;
            found = 1;
            break;
        }
        if (cmp < 0) low = mid + 1;
        else high = mid - 1;
    }

    if (found) {
        memcpy(index->entries[low].sha256, sha256, 32);
        index->entries[low].size = size;
        struct stat st;
        stat(path, &st);
        index->entries[low].mtime = (uint32_t)st.st_mtime;
        return 0;
    }

    index->count++;
    IndexEntry *new_entries = realloc(index->entries, sizeof(IndexEntry) * index->count);
    if (!new_entries) {
        index->count--;
        return -1;
    }
    index->entries = new_entries;

    if ((uint32_t)low < index->count - 1) {
        memmove(&index->entries[low + 1], &index->entries[low], (index->count - 1 - low) * sizeof(IndexEntry));
    }

    strncpy(index->entries[low].path, path, sizeof(index->entries[low].path) - 1);
    memcpy(index->entries[low].sha256, sha256, 32);
    index->entries[low].size = size;
    
    struct stat st;
    stat(path, &st);
    index->entries[low].mtime = (uint32_t)st.st_mtime;
    
    return 0;
}
