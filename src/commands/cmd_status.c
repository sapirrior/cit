#include "commands.h"
#include "index.h"
#include "utils.h"
#include "portability.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void scan_working_dir(const char *path, Index *index, char ***untracked, int *untracked_count, char ***modified, int *modified_count) {
    if (strstr(path, ".cit")) return;

    if (is_file(path)) {
        const char *norm_path = path;
        if (strncmp(path, "./", 2) == 0) norm_path = path + 2;

        int found_in_index = 0;
        int low = 0, high = (int)index->count - 1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            int cmp = strcmp(index->entries[mid].path, norm_path);
            if (cmp == 0) {
                found_in_index = 1;
                struct stat st;
                if (stat(path, &st) == 0) {
                    if ((uint32_t)st.st_mtime != index->entries[mid].mtime || (uint32_t)st.st_size != index->entries[mid].size) {
                        char **new_modified = realloc(*modified, sizeof(char *) * (*modified_count + 1));
                        if (new_modified) {
                            *modified = new_modified;
                            (*modified)[*modified_count] = strdup(norm_path);
                            (*modified_count)++;
                        }
                    }
                }
                break;
            }
            if (cmp < 0) low = mid + 1;
            else high = mid - 1;
        }
        if (!found_in_index) {
            char **new_untracked = realloc(*untracked, sizeof(char *) * (*untracked_count + 1));
            if (!new_untracked) return;
            *untracked = new_untracked;
            (*untracked)[*untracked_count] = strdup(norm_path);
            (*untracked_count)++;
        }
    } else if (dir_exists(path)) {
        DIR *dir = opendir(path);
        if (!dir) return;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char subpath[1024];
            if (strcmp(path, ".") == 0) {
                snprintf(subpath, sizeof(subpath), "%s", entry->d_name);
            } else {
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            }
            scan_working_dir(subpath, index, untracked, untracked_count, modified, modified_count);
        }
        closedir(dir);
    }
}

int cmd_status(int argc, char *argv[]) {
    Index *index = read_index();
    char *branch = get_current_branch();
    
    printf(COLOR_BOLD "On branch " COLOR_FG_ACCENT "%s" COLOR_RESET "\n\n", branch ? branch : "main");

    char **untracked = NULL;
    int untracked_count = 0;
    char **modified = NULL;
    int modified_count = 0;
    char **deleted = NULL;
    int deleted_count = 0;

    scan_working_dir(".", index, &untracked, &untracked_count, &modified, &modified_count);

    for (uint32_t i = 0; i < index->count; i++) {
        if (!is_file(index->entries[i].path)) {
            char **new_deleted = realloc(deleted, sizeof(char *) * (deleted_count + 1));
            if (new_deleted) {
                deleted = new_deleted;
                deleted[deleted_count] = strdup(index->entries[i].path);
                deleted_count++;
            }
        }
    }

    int has_changes = 0;

    if (index->count > 0) {
        int staged_visible = 0;
        for (uint32_t i = 0; i < index->count; i++) {
            int is_deleted = 0;
            for (int j = 0; j < deleted_count; j++) {
                if (strcmp(index->entries[i].path, deleted[j]) == 0) {
                    is_deleted = 1;
                    break;
                }
            }
            if (!is_deleted) {
                if (!staged_visible) {
                    ui_header("Changes to be committed:");
                    ui_info("  (use \"cit commit\" to record these changes)");
                    staged_visible = 1;
                }
                printf("  " COLOR_FG_SUCCESS "new file:   %s" COLOR_RESET "\n", index->entries[i].path);
                has_changes = 1;
            }
        }
        if (staged_visible) printf("\n");
    }

    if (modified_count > 0 || deleted_count > 0) {
        ui_header("Changes not staged for commit:");
        ui_info("  (use \"cit add <file>...\" to update what will be committed)");
        for (int i = 0; i < modified_count; i++) {
            printf("  " COLOR_FG_DANGER "modified:   %s" COLOR_RESET "\n", modified[i]);
            free(modified[i]);
        }
        for (int i = 0; i < deleted_count; i++) {
            printf("  " COLOR_FG_DANGER "deleted:    %s" COLOR_RESET "\n", deleted[i]);
            free(deleted[i]);
        }
        printf("\n");
        free(modified);
        free(deleted);
        has_changes = 1;
    }

    if (untracked_count > 0) {
        ui_header("Untracked files:");
        ui_info("  (use \"cit add <file>...\" to include in what will be committed)");
        for (int i = 0; i < untracked_count; i++) {
            printf("  " COLOR_FG_DANGER "%s" COLOR_RESET "\n", untracked[i]);
            free(untracked[i]);
        }
        printf("\n");
        free(untracked);
        has_changes = 1;
    }

    if (!has_changes) {
        ui_info("nothing to commit, working tree clean");
    }

    free_index(index);
    return 0;
}
