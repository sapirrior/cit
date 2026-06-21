#include "commands.h"
#include "object.h"
#include "index.h"
#include "utils.h"
#include "refs.h"
#include "portability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checkout_tree(const char *tree_sha, const char *base_path, Index *index) {
    size_t obj_len;
    obj_type type;
    char *tree_content = read_object(tree_sha, &obj_len, &type);
    if (!tree_content || type != OBJ_TREE) {
        free(tree_content);
        return -1;
    }

    if (strlen(base_path) > 0 && !dir_exists(base_path)) {
        mkdir_p(base_path);
    }

    char *saveptr;
    char *line = strtok_r(tree_content, "\n", &saveptr);
    while (line) {
        char type_str[10], sha[65], name[256];
        if (sscanf(line, "%9s %64s %255s", type_str, sha, name) == 3) {
            char sub_path[1024];
            if (strlen(base_path) == 0) {
                strncpy(sub_path, name, 1023);
            } else {
                snprintf(sub_path, sizeof(sub_path), "%s/%s", base_path, name);
            }
            sub_path[1023] = '\0';

            if (strcmp(type_str, "blob") == 0) {
                size_t blob_len;
                obj_type btype;
                void *blob_data = read_object(sha, &blob_len, &btype);
                if (blob_data && btype == OBJ_BLOB) {
                    FILE *fw = fopen(sub_path, "wb");
                    if (fw) {
                        fwrite(blob_data, 1, blob_len, fw);
                        fclose(fw);
                        printf("Restored %s\n", sub_path);

                        uint8_t sha_bytes[32];
                        for (int j = 0; j < 32; j++) {
                            unsigned int val;
                            sscanf(sha + (j * 2), "%02x", &val);
                            sha_bytes[j] = (uint8_t)val;
                        }
                        add_to_index(index, sub_path, sha_bytes, (uint32_t)blob_len);
                    }
                    free(blob_data);
                }
            } else if (strcmp(type_str, "tree") == 0) {
                checkout_tree(sha, sub_path, index);
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(tree_content);
    return 0;
}

static void remove_empty_parents(const char *path) {
    char dir[1024];
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *last_slash = strrchr(dir, '/');
    while (last_slash) {
        *last_slash = '\0';
        if (strlen(dir) == 0) break;
        if (rmdir(dir) != 0) {
            break;
        }
        last_slash = strrchr(dir, '/');
    }
}

int cmd_checkout(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: cit checkout <commit-sha|branch-name>\n");
        return 1;
    }

    char *target = argv[1];

    // 1. Resolve target to a SHA
    char *resolved_sha = get_ref_sha(target);
    if (!resolved_sha) {
        fprintf(stderr, "Error: Could not resolve %s to a commit or branch.\n", target);
        return 1;
    }
    char target_sha[65];
    strncpy(target_sha, resolved_sha, 64);
    target_sha[64] = 0;

    // 2. Update HEAD (distinguish branch vs raw SHA by checking branch file existence)
    char head_content[256];
    char branch_path[512];
    snprintf(branch_path, sizeof(branch_path), ".cit/refs/heads/%s", target);
    if (is_file(branch_path)) {
        snprintf(head_content, sizeof(head_content), "ref: refs/heads/%s\n", target);
    } else {
        snprintf(head_content, sizeof(head_content), "%s\n", target_sha);
    }
    write_string_to_file(".cit/HEAD", head_content);

    // 3. Get Tree SHA from Commit
    size_t obj_len;
    obj_type type;
    char *commit_content = read_object(target_sha, &obj_len, &type);
    if (!commit_content || type != OBJ_COMMIT) {
        fprintf(stderr, "Error: %s is not a commit.\n", target_sha);
        free(commit_content);
        return 1;
    }

    char tree_sha[65] = "";
    char *tree_line = strstr(commit_content, "tree ");
    if (tree_line) {
        strncpy(tree_sha, tree_line + 5, 64);
        tree_sha[64] = 0;
    }
    free(commit_content);

    if (strlen(tree_sha) == 0) {
        fprintf(stderr, "Error: Could not find tree in commit %s\n", target_sha);
        return 1;
    }

    // 4. Recursive checkout into a fresh new index
    Index *old_index = read_index();
    Index *new_index = malloc(sizeof(Index));
    if (!new_index) {
        free_index(old_index);
        return 1;
    }
    new_index->count = 0;
    new_index->entries = NULL;

    checkout_tree(tree_sha, "", new_index);

    // Remove files from working directory that are in the old index but not in the new tree
    for (uint32_t i = 0; i < old_index->count; i++) {
        const char *old_path = old_index->entries[i].path;
        int found_in_new = 0;
        // Search in new_index (binary search since it's sorted or we can just scan)
        int low = 0, high = (int)new_index->count - 1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            int cmp = strcmp(new_index->entries[mid].path, old_path);
            if (cmp == 0) {
                found_in_new = 1;
                break;
            }
            if (cmp < 0) low = mid + 1;
            else high = mid - 1;
        }

        if (!found_in_new) {
            if (is_file(old_path)) {
                unlink(old_path);
                remove_empty_parents(old_path);
            }
        }
    }

    write_index(new_index);
    free_index(old_index);
    free_index(new_index);

    printf("Switched to %s\n", target);
    return 0;
}
