#include "commands.h"
#include "utils.h"
#include "config.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static void list_branches_recursive(const char *base_path, const char *rel_path, const char *current) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, rel_path);
    
    DIR *dir = opendir(full_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char next_rel[1024];
        if (strlen(rel_path) == 0) {
            snprintf(next_rel, sizeof(next_rel), "%s", entry->d_name);
        } else {
            snprintf(next_rel, sizeof(next_rel), "%s/%s", rel_path, entry->d_name);
        }

        char next_full[1024];
        snprintf(next_full, sizeof(next_full), "%s/%s", base_path, next_rel);

        if (dir_exists(next_full)) {
            list_branches_recursive(base_path, next_rel, current);
        } else {
            if (current && strcmp(next_rel, current) == 0) {
                printf("* " COLOR_FG_SUCCESS "%s" COLOR_RESET "\n", next_rel);
            } else {
                printf("  %s\n", next_rel);
            }
        }
    }
    closedir(dir);
}

int cmd_branch(int argc, char *argv[]) {
    if (argc > 1 && !check_config()) {
        fprintf(stderr, "Error: User configuration (username and email) not found.\n");
        fprintf(stderr, "Use 'cit config -u <name>' and 'cit config -e <email>' to set them.\n");
        return 1;
    }

    if (argc == 1) {
        // List branches
        char *current = get_current_branch();
        list_branches_recursive(".cit/refs/heads", "", current);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "-m") != 0) {
        // Create branch
        char *new_branch = argv[1];
        char current_sha[65] = "";
        char *current = get_current_branch();
        if (current) {
            char full_ref_path[512];
            snprintf(full_ref_path, sizeof(full_ref_path), ".cit/refs/heads/%s", current);
            FILE *fref = fopen(full_ref_path, "r");
            if (fref) {
                if (!fgets(current_sha, 65, fref)) {
                    current_sha[0] = 0;
                }
                fclose(fref);
            }
        }

        if (strlen(current_sha) == 0) {
            fprintf(stderr, "Error: No commits yet. Cannot create branch.\n");
            return 1;
        }

        char rel_ref_path[512];
        snprintf(rel_ref_path, sizeof(rel_ref_path), "refs/heads/%s", new_branch);
        
        if (write_ref(rel_ref_path, current_sha) != 0) {
            fprintf(stderr, "Error: Could not create branch %s.\n", new_branch);
            return 1;
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        // Delete branch
        char *target = argv[2];
        char *current = get_current_branch();
        if (current && strcmp(target, current) == 0) {
            fprintf(stderr, "Error: Cannot delete the branch you are currently on.\n");
            return 1;
        }
        char path[512];
        snprintf(path, sizeof(path), ".cit/refs/heads/%s", target);
        if (remove(path) != 0) {
            fprintf(stderr, "Error: Could not delete branch %s.\n", target);
            return 1;
        }
        printf("Deleted branch %s.\n", target);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "-m") == 0) {
        // Rename branch
        char *old_name = argv[2];
        char *new_name = argv[3];
        char old_path[512], new_path[512];
        snprintf(old_path, sizeof(old_path), ".cit/refs/heads/%s", old_name);
        snprintf(new_path, sizeof(new_path), ".cit/refs/heads/%s", new_name);
        
        if (rename(old_path, new_path) != 0) {
            fprintf(stderr, "Error: Could not rename branch %s to %s.\n", old_name, new_name);
            return 1;
        }
        
        // If we renamed the current branch, update HEAD
        char *current = get_current_branch();
        if (current && strcmp(old_name, current) == 0) {
            char head_content[256];
            snprintf(head_content, sizeof(head_content), "ref: refs/heads/%s\n", new_name);
            write_string_to_file(".cit/HEAD", head_content);
        }
        return 0;
    }

    printf("Usage: cit branch [<name> | -d <name> | -m <old> <new>]\n");
    return 1;
}
