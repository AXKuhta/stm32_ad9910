#include <stddef.h>

void __libc_init_array(void) {
}

int puts(const char* str) {
	return 0;
}

int printf(const char *restrict format, ...) {
	return 0;
}

void __assert_func (const char * file, int line, const char * func, const char * msg) {
	while (1) {}
}

void* realloc(void* ptr, size_t size) {
	return 0;
}

void* memset(void* ptr, int pattern, size_t size) {
	return 0;
}

int memcmp(const void* s1, const void* s2, size_t n) {
	return 0;
}

void* memmove(void* dst, const void* src, size_t n) {
	return 0;
}

void* memcpy(void* restrict dst, const void* restrict src, size_t n) {
	return 0;
}

int strcasecmp(const char *s1, const char *s2) {
	return 0;
}

int strcmp(const char *s1, const char *s2) {
	return 0;
}

int sscanf(const char *restrict str, const char *restrict format, ...) {
	return 0;
}

int snprintf(char* restrict str, size_t size, const char *restrict format, ...) {
	return 0;
}

int asprintf(char **restrict strp, const char *restrict fmt, ...) {
	return 0;
}
