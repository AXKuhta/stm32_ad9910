#include <stdint.h>

#define M_216MHz 216000000.0

#define NS_TO_216MHZ_MU 0.216

// Разрешение таймеров
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

void timer8_init();
void timer8_stop();
void timer8_restart();

uint32_t timer_mu(uint32_t time_ns);
