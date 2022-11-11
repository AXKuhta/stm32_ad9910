#include <stdint.h>

// Разрешение таймеров
// т.е. 1 s / 108 MHz
#define NANOSEC_54MHZ 18.51851851851852
#define NANOSEC_108MHZ 9.25925925925926

// Для контроля отступа и продолжительности импульсов
// 2^32 ns = 4.294967296 s
typedef struct pulse_t {
	uint32_t t1;
	uint32_t t2;
} pulse_t;

void radar_emulator_start();
