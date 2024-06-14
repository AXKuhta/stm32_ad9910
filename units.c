#define _GNU_SOURCE // Нужно для появления прототипа asprintf()

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t parse_freq(double freq, const char* unit) {
	double multiplier;

	if (strcasecmp(unit, "mhz") == 0) {
		multiplier = 1000*1000;
	} else if (strcasecmp(unit, "khz") == 0) {
		multiplier = 1000;
	} else if (strcasecmp(unit, "hz") == 0) {
		multiplier = 1;
	} else {
		printf("Invalid frequency unit: %s\n", unit);
		return 0;
	}

	return freq * multiplier;
}

uint32_t parse_time(double time, const char* unit) {
	double multiplier;

	if (strcasecmp(unit, "s") == 0) {
		multiplier = 1000*1000*1000;
	} else if (strcasecmp(unit, "ms") == 0) {
		multiplier = 1000*1000;
	} else if (strcasecmp(unit, "us") == 0) {
		multiplier = 1000;
	} else if (strcasecmp(unit, "ns") == 0) {
		multiplier = 1;
	} else {
		printf("Invalid time unit: %s\n", unit);
		return 0;
	}

	return time * multiplier;
}

double parse_volts(double voltage, const char* unit) {
	double multiplier;

	if (strcasecmp(unit, "uv") == 0) {
		multiplier = 0.001 * 0.001;
	} else if (strcasecmp(unit, "mv") == 0) {
		multiplier = 0.001;
	} else if (strcasecmp(unit, "v") == 0) {
		multiplier = 1;
	} else {
		printf("Invalid voltage unit: %s\n", unit);
		return 0;
	}

	return voltage * multiplier;
}

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

char* freq_unit(double freq) {
	const char* unit;
	char* ret = NULL;

	if (freq > 1000*1000*1000) {
		freq = freq/1000/1000/1000;
		unit = "GHz";
	} else if (freq > 1000*1000) {
		freq = freq/1000/1000;
		unit = "MHz";
	} else if (freq > 1000) {
		freq = freq/1000;
		unit = "KHz";
	} else {
		unit = "Hz";
	}

	asprintf(&ret, "%.3lf %s", freq, unit);

	return ret;
}

