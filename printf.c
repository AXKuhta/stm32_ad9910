#define _GNU_SOURCE // Нужно для появления прототипа vasprintf()

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscalls.h"
#include "server.h"

#include "FreeRTOS.h"
#include "task.h"

// Перенаправляемый printf
int printf(const char *restrict fmt, ...) {
	va_list ap;
	int i;

	char* buf = NULL;

	// Для небольших строк vasprintf() будет делать два вызова realloc():
	// realloc(NULL, 32) для начального буфера
	// realloc(ptr, 6) для уменьшения буфера, когда строка готова
	//
	// Для больших строк, vasprintf() будет удваивать буфер с 32 по мере необходимости
	//

	va_start(ap, fmt);
	i = vasprintf(&buf, fmt, ap);
	va_end(ap);

	// Достать указатель на сокет (или NULL)
	void* sock = pvTaskGetThreadLocalStoragePointer(NULL, 0);

	// NULL = выводим в последовательный порт
	// Не NULL = отправляем в сокет
	if (sock) {
		send_all(sock, buf, strlen(buf));
	} else {
		_write(1, buf, strlen(buf));
	}

	free(buf);

	return i;
}

// Вызов puts() вставляется компилятором вместо вызовов printf() в тех случаях, когда печатается строка без подстановок (Но с \n в конце!)
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

// Реализация assert()
void __assert_func(const char * file, int line, const char * func, const char * msg) {
	printf("%s:%d %s(): %s\n", file, line, func, msg);
	while (1) {}
}
