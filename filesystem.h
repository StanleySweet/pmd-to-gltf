#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

// Structure to hold a list of file paths
typedef struct {
    char **paths;
    uint32_t count;
    uint32_t capacity;
} FileList;

// Find all files matching a pattern in a directory
// Pattern examples: "*.psa", "horse_*.psa"
FileList* find_files(const char *directory, const char *pattern);

// Free a FileList structure
void free_file_list(FileList *list);

#endif // FILESYSTEM_H
