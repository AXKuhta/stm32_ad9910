#define _GNU_SOURCE // Нужно для появления прототипа vasprintf()

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscalls.h"
#include "server.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "task.h"

// Перенаправить printf в сокет
// Нулевая позиция в Thread Local Storage зарезервирована под указатель на сокет
void printf_redirect(void* socket) {
	vTaskSetThreadLocalStoragePointer(NULL, 0, socket);
}

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

	BaseType_t size = strlen(buf);

	// Достать указатель на сокет (или NULL)
	Socket_t sock = pvTaskGetThreadLocalStoragePointer(NULL, 0);

	// NULL = выводим в последовательный порт
	// Не NULL = отправляем в сокет
	if (sock) {
		BaseType_t result = FreeRTOS_send(sock, buf, size, FREERTOS_MSG_DONTWAIT);

		// Не получилось отправить? Предупредить вызывающего
		if (result != size) {
			return -1;
		}
	} else {
		_write(1, buf, size);
	}

	free(buf);

	return i;
}

// Вызов puts() вставляется компилятором вместо вызовов printf() в тех случаях, когда печатается строка без подстановок (Но с \n в конце!)
int puts(const char* str) {
	Socket_t sock = pvTaskGetThreadLocalStoragePointer(NULL, 0);
	BaseType_t size = strlen(str);

	if (sock) {
		BaseType_t result = FreeRTOS_send(sock, str, size, FREERTOS_MSG_DONTWAIT);

		if (result < 0)
			return EOF;

		result = FreeRTOS_send(sock, "\n", 1, FREERTOS_MSG_DONTWAIT);

		if (result < 0)
			return EOF;
	} else {
		_write(1, str, size);
		_write(1, "\n", 1);
	}

	return size;
}

// Реализация assert()
void __assert_func(const char * file, int line, const char * func, const char * msg) {
	printf("%s:%d %s(): %s\n", file, line, func, msg);
	while (1) {}
}
