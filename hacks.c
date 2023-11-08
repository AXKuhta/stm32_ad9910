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

int sscanf(const char *restrict str, const char *restrict format, ...) {
	return 0;
}

int snprintf(char* restrict str, size_t size, const char *restrict format, ...) {
	return 0;
}

int asprintf(char **restrict strp, const char *restrict fmt, ...) {
	return 0;
}
