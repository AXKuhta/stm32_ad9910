#include <stdint.h>

#define M_216MHz 216000000.0

#define NS_TO_216MHZ_MU 0.216

// 2^32 ns = 4.294967296 s
typedef struct timing_t {
	uint32_t t1;
	uint32_t t2;
} timing_t;

void radar_emulator_start();

void timer2_init();
void timer2_stop();
void timer2_restart();

void timer8_init();
void timer8_stop();
void timer8_restart();

uint32_t timer_mu(uint32_t time_ns);
