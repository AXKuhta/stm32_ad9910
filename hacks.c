#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscalls.h"

#include "FreeRTOS.h"
#include "task.h"

void __assert_func (const char * file, int line, const char * func, const char * msg) {
	printf("%s:%d %s(): %s\n", file, line, func, msg);
	while (1) {}
}

int puts(const char* str) {
	void* sock = pvTaskGetThreadLocalStoragePointer(NULL, 0);

	if (sock) {
		send_all(sock, str, strlen(str));
		send_all(sock, "\n", 1);
	} else {
		_write(1, str, strlen(str));
		_write(1, "\n", 1);
	}

	return 1;
}

int printf(const char *restrict fmt, ...) {
	va_list ap;
	int i;

	char* buf = malloc(128);

	va_start(ap, fmt);
	i = vsnprintf(buf, 128, fmt, ap);
	va_end(ap);

	void* sock = pvTaskGetThreadLocalStoragePointer(NULL, 0);

	if (sock) {
		send_all(sock, buf, strlen(buf));
	} else {
		_write(1, buf, strlen(buf));
	}

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

