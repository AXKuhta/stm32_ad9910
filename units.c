#include <stdint.h>

const char* time_unit_str(double time) {
	if (time < 1.0/1000.0/1000.0) return "ns";
	if (time < 1.0/1000.0) return "us";
	if (time < 1.0) return "ms";

	return "s";
}

uint32_t time_unit_int(double time) {
	if (time < 1.0/1000.0/1000.0) return time*1000.0*1000.0*1000.0;
	if (time < 1.0/1000.0) return time*1000.0*1000.0;
	if (time < 1.0) return time*1000.0;

	return time;
}
