#include "test_framework.h"
#include "filesystem.h"
#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

// Helper function to create a temporary test file
static int create_test_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    if (content) {
        fputs(content, f);
    }
    fclose(f);
    return 1;
}

// Helper function to remove test file
static void remove_test_file(const char *path) {
    unlink(path);
}

static int test_create_and_free_file_list(void) {
    // Create empty directory for testing
    FileList *list = find_files(".", "nonexistent_*.test");
    
    TEST_ASSERT_NOT_NULL(list, "find_files should return a valid FileList even when no files match");
    TEST_ASSERT_EQ(0, list->count, "Empty search should return 0 files");
    
    free_file_list(list);
    return 1;
}

static int test_find_files_with_pattern(void) {
    // Create some test files
    TEST_ASSERT(create_test_file("test1.psa", "test content"), "Failed to create test file");
    TEST_ASSERT(create_test_file("test2.psa", "test content"), "Failed to create test file");
    TEST_ASSERT(create_test_file("other.txt", "test content"), "Failed to create test file");
    
    FileList *list = find_files(".", "*.psa");
    
    TEST_ASSERT_NOT_NULL(list, "find_files should return a valid FileList");
    TEST_ASSERT(list->count >= 2, "Should find at least 2 .psa files");
    
    // Verify file paths are correctly set
    for (uint32_t i = 0; i < list->count; i++) {
        TEST_ASSERT_NOT_NULL(list->paths[i], "File path should not be NULL");
    }
    
    free_file_list(list);
    
    // Cleanup
    remove_test_file("test1.psa");
    remove_test_file("test2.psa");
    remove_test_file("other.txt");
    
    return 1;
}

static int test_find_files_specific_pattern(void) {
    // Create test files with specific pattern
    TEST_ASSERT(create_test_file("horse_idle.psa", "animation data"), "Failed to create test file");
    TEST_ASSERT(create_test_file("horse_walk.psa", "animation data"), "Failed to create test file");
    TEST_ASSERT(create_test_file("cow_idle.psa", "animation data"), "Failed to create test file");
    
    FileList *list = find_files(".", "horse_*.psa");
    
    TEST_ASSERT_NOT_NULL(list, "find_files should return a valid FileList");
    TEST_ASSERT(list->count >= 2, "Should find at least 2 horse_*.psa files");
    
    // Verify that found files match the pattern
    int horse_files_found = 0;
    for (uint32_t i = 0; i < list->count; i++) {
        if (strstr(list->paths[i], "horse_") && strstr(list->paths[i], ".psa")) {
            horse_files_found++;
        }
    }
    TEST_ASSERT(horse_files_found >= 2, "Should find horse pattern files");
    
    free_file_list(list);
    
    // Cleanup
    remove_test_file("horse_idle.psa");
    remove_test_file("horse_walk.psa");
    remove_test_file("cow_idle.psa");
    
    return 1;
}

static int test_free_null_list(void) {
    // This should not crash
    free_file_list(NULL);
    return 1;
}

static int test_find_files_nonexistent_directory(void) {
    FileList *list = find_files("/nonexistent/directory/path", "*.psa");
    
    TEST_ASSERT_NOT_NULL(list, "find_files should return a valid FileList even for nonexistent directories");
    TEST_ASSERT_EQ(0, list->count, "Nonexistent directory should return 0 files");
    
    free_file_list(list);
    return 1;
}

int main(void) {
    const test_case_t tests[] = {
        {"create_and_free_file_list", test_create_and_free_file_list},
        {"find_files_with_pattern", test_find_files_with_pattern},
        {"find_files_specific_pattern", test_find_files_specific_pattern},
        {"free_null_list", test_free_null_list},
        {"find_files_nonexistent_directory", test_find_files_nonexistent_directory}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
