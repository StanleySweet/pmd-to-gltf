#include "test_framework.h"
#include <string.h>
#include <stdlib.h>

// Copy of the extract_anim_name function from main.c for testing
static char* extract_anim_name(const char *psa_file, const char *basename) {
    // Find last path separator
    const char *filename = strrchr(psa_file, '/');
    if (!filename) filename = strrchr(psa_file, '\\');
    filename = filename ? filename + 1 : psa_file;

    // Find the extension
    const char *ext = strrchr(filename, '.');
    if (!ext) return NULL;

    // Build the prefix to skip: "basename_"
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%s_", basename);
    size_t prefix_len = strlen(prefix);

    // Check if filename starts with prefix
    if (strncmp(filename, prefix, prefix_len) == 0) {
        // Extract everything between prefix and extension
        const char *start = filename + prefix_len;
        size_t len = ext - start;
        char *name = malloc(len + 1);
        memcpy(name, start, len);
        name[len] = '\0';
        return name;
    }

    return NULL;
}

static int test_extract_anim_name_simple(void) {
    char *result = extract_anim_name("horse_idle.psa", "horse");
    
    TEST_ASSERT_NOT_NULL(result, "Should extract animation name from simple filename");
    TEST_ASSERT_STR_EQ("idle", result, "Should extract 'idle' from 'horse_idle.psa'");
    
    free(result);
    return 1;
}

static int test_extract_anim_name_complex(void) {
    char *result = extract_anim_name("horse_attack_a.psa", "horse");
    
    TEST_ASSERT_NOT_NULL(result, "Should extract animation name from complex filename");
    TEST_ASSERT_STR_EQ("attack_a", result, "Should extract 'attack_a' from 'horse_attack_a.psa'");
    
    free(result);
    return 1;
}

static int test_extract_anim_name_with_path(void) {
    char *result = extract_anim_name("/path/to/horse_walk.psa", "horse");
    
    TEST_ASSERT_NOT_NULL(result, "Should extract animation name from file with path");
    TEST_ASSERT_STR_EQ("walk", result, "Should extract 'walk' from '/path/to/horse_walk.psa'");
    
    free(result);
    return 1;
}

static int test_extract_anim_name_windows_path(void) {
    char *result = extract_anim_name("C:\\game\\assets\\horse_gallop.psa", "horse");
    
    TEST_ASSERT_NOT_NULL(result, "Should extract animation name from Windows path");
    TEST_ASSERT_STR_EQ("gallop", result, "Should extract 'gallop' from Windows path");
    
    free(result);
    return 1;
}

static int test_extract_anim_name_wrong_basename(void) {
    char *result = extract_anim_name("cow_idle.psa", "horse");
    
    TEST_ASSERT_NULL(result, "Should return NULL for wrong basename");
    
    return 1;
}

static int test_extract_anim_name_no_extension(void) {
    char *result = extract_anim_name("horse_idle", "horse");
    
    TEST_ASSERT_NULL(result, "Should return NULL for file without extension");
    
    return 1;
}

static int test_extract_anim_name_no_underscore(void) {
    char *result = extract_anim_name("horseidle.psa", "horse");
    
    TEST_ASSERT_NULL(result, "Should return NULL for file without underscore separator");
    
    return 1;
}

static int test_extract_anim_name_empty_animation(void) {
    char *result = extract_anim_name("horse_.psa", "horse");
    
    TEST_ASSERT_NOT_NULL(result, "Should handle empty animation name");
    TEST_ASSERT_STR_EQ("", result, "Should return empty string for 'horse_.psa'");
    
    free(result);
    return 1;
}

int main(void) {
    const test_case_t tests[] = {
        {"extract_anim_name_simple", test_extract_anim_name_simple},
        {"extract_anim_name_complex", test_extract_anim_name_complex},
        {"extract_anim_name_with_path", test_extract_anim_name_with_path},
        {"extract_anim_name_windows_path", test_extract_anim_name_windows_path},
        {"extract_anim_name_wrong_basename", test_extract_anim_name_wrong_basename},
        {"extract_anim_name_no_extension", test_extract_anim_name_no_extension},
        {"extract_anim_name_no_underscore", test_extract_anim_name_no_underscore},
        {"extract_anim_name_empty_animation", test_extract_anim_name_empty_animation}
    };
    
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
