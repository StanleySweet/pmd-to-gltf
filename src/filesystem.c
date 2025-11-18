#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portable_string.h"

// Cross-platform safe string duplication
#include "portable_string.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <fnmatch.h>
#endif

static FileList* create_file_list(void) {
    FileList *list = malloc(sizeof(FileList));
    if (!list) return NULL;
    
    list->capacity = 10;
    list->count = 0;
    list->paths = calloc(list->capacity, sizeof(char*));
    if (!list->paths) {
        free(list);
        return NULL;
    }
    
    return list;
}

static void add_file_to_list(FileList *list, const char *filepath) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->paths = realloc(list->paths, list->capacity * sizeof(char*));
    }
    
    list->paths[list->count++] = my_strdup(filepath);
}

FileList* find_files(const char *directory, const char *pattern) {
    FileList *list = create_file_list();
    if (!list) return NULL;

#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s\\%s", directory, pattern);
    
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "%s\\%s", directory, find_data.cFileName);
                add_file_to_list(list, filepath);
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(directory);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
                if (fnmatch(pattern, entry->d_name, 0) == 0) {
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
                    add_file_to_list(list, filepath);
                }
            }
        }
        closedir(dir);
    }
#endif

    return list;
}

void free_file_list(FileList *list) {
    if (!list) return;
    
    for (uint32_t i = 0; i < list->count; i++) {
        free(list->paths[i]);
    }
    free(list->paths);
    free(list);
}
