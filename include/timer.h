#include <stdint.h>

// Разрешение таймеров
// т.е. 1 s / 108 MHz
#define NANOSEC_54MHZ 18.51851851851852
#define NANOSEC_108MHZ 9.25925925925926
#define NANOSEC_216MHZ 4.629629629629629

// 2^32 ns = 4.294967296 s
typedef struct timing_t {
	uint32_t t1;
	uint32_t t2;
} timing_t;

void radar_emulator_start();

void timer2_init();
void timer2_stop();
void timer2_restart();

void timer5_init();
void timer5_stop();
void timer5_restart();

uint32_t timer_mu(uint32_t time_ns);
