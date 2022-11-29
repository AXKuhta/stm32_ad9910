#include <stdint.h>

// Разрешение таймеров
// т.е. 1 s / 108 MHz
#define NANOSEC_54MHZ 18.51851851851852
#define NANOSEC_108MHZ 9.25925925925926


// 2^32 ns = 4.294967296 s

void radar_emulator_start();

void timer2_init();
void timer2_stop();
void timer2_restart();
void pulse_set_timing(pulse_t* pulse, uint32_t delay_ns, uint32_t length_ns);
