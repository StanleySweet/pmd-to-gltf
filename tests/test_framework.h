#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return 0; \
        } \
    } while ((void)0, 0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s - %s (expected: %d, got: %d)\n", __func__, message, (int)(expected), (int)(actual)); \
            return 0; \
        } \
    } while ((void)0, 0)

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("FAIL: %s - %s (expected: \"%s\", got: \"%s\")\n", __func__, message, expected, actual); \
            return 0; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != NULL) { \
            printf("FAIL: %s - %s (expected NULL, got %p)\n", __func__, message, (void*)(ptr)); \
            return 0; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            printf("FAIL: %s - %s (expected non-NULL, got NULL)\n", __func__, message); \
            return 0; \
        } \
    } while(0)

// Test runner
typedef int (*test_function_t)(void);

typedef struct {
    const char *name;
    test_function_t func;
} test_case_t;

static int run_tests(const test_case_t *tests, int count) {
    int passed = 0;
    int failed = 0;
    
    printf("Running %d tests...\n\n", count);
    
    for (int i = 0; i < count; i++) {
        printf("Test %d/%d: %s... ", i + 1, count, tests[i].name);
        fflush(stdout);
        
        if (tests[i].func()) {
            printf("PASS\n");
            passed++;
        } else {
            failed++;
        }
    }
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", count);
    
    return failed == 0 ? 0 : 1;
}

#endif // TEST_FRAMEWORK_H
