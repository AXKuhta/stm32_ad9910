#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscalls.h"

void __assert_func (const char * file, int line, const char * func, const char * msg) {
	printf("%s:%d %s(): %s\n", file, line, func, msg);
	while (1) {}
}

int puts(const char* str) {
	return _write(1, str, strlen(str));
}

int printf(const char *restrict fmt, ...) {
	va_list ap;
	int i;

	char* buf = malloc(128);

	va_start(ap, fmt);
	i = vsnprintf(buf, 128, fmt, ap);
	va_end(ap);

	puts(buf);
	free(buf);

	return i;
}

/*
void __libc_init_array(void) {
}

int sscanf(const char *restrict str, const char *restrict format, ...) {
	return 0;
}
*/

