#ifndef PORTABLE_STRING_H
#define PORTABLE_STRING_H

#include <string.h>
#include <stdlib.h>

// Portable strncpy macro
#if defined(HAVE_STRNCPY_S)
#define my_strncpy(dest, src, n) strncpy_s(dest, n, src, _TRUNCATE)
#else
#define my_strncpy(dest, src, n) strncpy(dest, src, n)
#endif

// Portable strdup macro
#if defined(HAVE__STRDUP)
#define my_strdup _strdup
#elif defined(HAVE_STRDUP)
#define my_strdup strdup
#else
static inline char* my_strdup(const char *str) {
	if (!str) return NULL;
	size_t len = strlen(str);
	char *copy = malloc(len + 1);
	if (copy) {
		memcpy(copy, str, len);
		copy[len] = '\0';
	}
	return copy;
}
#endif

#endif // PORTABLE_STRING_H
