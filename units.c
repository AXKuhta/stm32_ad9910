#define _GNU_SOURCE // Нужно для появления прототипа asprintf()

#include <stdint.h>
#include <stdio.h>

char* time_unit(double time) {
	const char* unit;
	char* ret = NULL;
	int intv;

	if (time < 1.0/1000.0/1000.0) {
		intv = time*1000*1000*1000;
		unit = "ns";
	} else if (time < 1.0/1000.0) {
		intv = time*1000*1000;
		unit = "us";
	} else if (time < 1.0) {
		intv = time*1000;
		unit = "ms";
	} else {
		unit = "s";
	}

	asprintf(&ret, "%d %s", intv, unit);

	return ret;
}
