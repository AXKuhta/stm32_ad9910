#include <stdio.h>
#include <assert.h>

#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "ad9910.h"
#include "vt100.h"
#include "spi.h"

// Профили
#define P_0 	GPIOD, GPIO_PIN_13
#define P_1 	GPIOD, GPIO_PIN_12
#define P_2 	GPIOD, GPIO_PIN_11

// Управляющие сигналы
#define IO_UPDATE 	GPIOB, GPIO_PIN_0
#define IO_RESET 	GPIOG, GPIO_PIN_9
#define DR_CTL 		GPIOB, GPIO_PIN_11
#define DR_HOLD 	GPIOB, GPIO_PIN_10

// Текущая тактовая частота AD9910
// Используется при вычислении FTW
static uint32_t ad_system_clock = 0;

// Регистры AD9910
// Состояние после сброса
static uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
static uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2
static uint8_t r02[4] = {0x1F, 0x3F, 0x40, 0x00}; // CFR3
static uint8_t r03[4] = {0x00, 0x00, 0x7F, 0x7F}; // Aux DAC control
static uint8_t r04[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // I/O Update Rate
static uint8_t r05[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
static uint8_t r06[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
static uint8_t r07[4] = {0x00, 0x00, 0x00, 0x00}; // FTW
static uint8_t r08[2] = {0x00, 0x00}; // POW
static uint8_t r09[4] = {0x00, 0x00, 0x00, 0x00}; // ASF
static uint8_t r0A[4] = {0x00, 0x00, 0x00, 0x00}; // Multichip Sync
static uint8_t r0B[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Limit
static uint8_t r0C[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Step
static uint8_t r0D[4] = {0x00, 0x00, 0x00, 0x00}; // Digital Ramp Rate
static uint8_t r0E[8] = {0x08, 0xB5, 0x00, 0x00, 0x0C, 0xCC, 0xCC, 0xCD}; // Profile 0
static uint8_t r0F[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 1
static uint8_t r10[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 2
static uint8_t r11[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 3
static uint8_t r12[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 4
static uint8_t r13[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 5
static uint8_t r14[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 6
static uint8_t r15[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 7
static uint8_t r16[4] = {0x00, 0x00, 0x00, 0x00}; // RAM

static uint8_t* regmap[23] = {
	r00,
	r01,
	r02,
	r03,
	r04,
	r05,
	r06,
	r07,
	r08,
	r09,
	r0A,
	r0B,
	r0C,
	r0D,
	r0E,
	r0F,
	r10,
	r11,
	r12,
	r13,
	r14,
	r15,
	r16
};

static void init_profile_gpio() {
	PIN_Init(P_0);
	PIN_Init(P_1);
	PIN_Init(P_2);
	
	HAL_GPIO_WritePin(P_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_2, GPIO_PIN_RESET);
}

static void init_control_gpio() {
	PIN_Init(IO_UPDATE);
	PIN_Init(IO_RESET);
	
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_RESET);
}

void drctl_software_controlled() {
	HAL_GPIO_DeInit(DR_CTL);
	PIN_Init(DR_CTL);
}

void drhold_software_controlled() {
	HAL_GPIO_DeInit(DR_HOLD);
	PIN_Init(DR_HOLD);
}

void drctl_timer_controlled() {
	HAL_GPIO_DeInit(DR_CTL);
	PIN_AF_Init(DR_CTL, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH4
}

void drhold_timer_controlled() {
	HAL_GPIO_DeInit(DR_HOLD);
	PIN_AF_Init(DR_HOLD, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH3
}

// Перевести номер профиля в значение, пригодное для записи в регистр ODR
uint16_t profile_to_gpio_states(uint8_t profile_id) {
	return 	(profile_id & 0b001 ? GPIO_PIN_13 : 0) +
			(profile_id & 0b010 ? GPIO_PIN_12 : 0) +
			(profile_id & 0b100 ? GPIO_PIN_11 : 0);
}

// Установить профиль
void set_profile(uint8_t profile_id) {
	assert(profile_id < 8);

	LL_GPIO_WriteOutputPort(GPIOD, profile_to_gpio_states(profile_id));
}

// Установить направление хода Digital Ramp генератора
// 1 = отсчитывать вверх
// 0 = отсчитывать вниз
// Вероятно, будет использоваться только режим отсчёта вверх
void set_ramp_direction(uint8_t direction) {
	HAL_GPIO_WritePin(DR_CTL, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void my_delay(uint32_t delay) {
	for (uint32_t i=0; i<delay; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
}

// Хоть какая-то задержка должна присутствовать
// Без неё импульса вообще не возникнет
void ad_pulse_io_reset() {
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_RESET);
	HAL_Delay(1);
}	

void ad_pulse_io_update() {
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_RESET);
}

void ad_write(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = reg_addr & 0x1F;
	
	spi_send(&instruction, 1);

	for (uint16_t i = 0; i < size; i++) {
		spi_send(&buffer[i], 1);
	}
}

//
// Чтение регистров AD9910
//
void ad_readback(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = 0x80 | (reg_addr & 0x1F);
	uint8_t read[8] = {0};
	
	spi_send(&instruction, 1);
	
	for (int i = 0; i < size; i++) {
		spi_recv(&read[i], 1);
	}
	
	printf("ad9910.c: register 0x%02X: ", reg_addr);
	
	for (int i = 0; i < size; i++) {
		printf(COLOR_GREEN "%s0x%02X ", buffer[i] == read[i] ? COLOR_GREEN : COLOR_RED, read[i]);
	}
	
	printf(COLOR_RESET "\n");
}

// Включить блок масштабирования амплитуды
// По умолчанию выключен в целях энергосбережения
// Необходимо для ad_set_profile_amplitude()
void ad_enable_amplitude_scaler() {
	r01[0] |= 0b0000001;
}

// Прочитать и сверить содержимое всех регистров с ожидаемыми значениями
// Не трогать недокументированные регистры 0x05 и 0x06
// Не пытаться сверять оперативную память
void ad_readback_all() {
	printf("Full readback\n");

	ad_pulse_io_reset();
	
	ad_readback(0x00, r00, 4);
	ad_readback(0x01, r01, 4);
	ad_readback(0x02, r02, 4);
	ad_readback(0x03, r03, 4);
	ad_readback(0x04, r04, 4);
	//ad_readback(0x05, r05, 6);
	//ad_readback(0x06, r06, 6);
	ad_readback(0x07, r07, 4);
	ad_readback(0x08, r08, 2);
	ad_readback(0x09, r09, 4);
	ad_readback(0x0A, r0A, 4);
	ad_readback(0x0B, r0B, 8);
	ad_readback(0x0C, r0C, 8);
	ad_readback(0x0D, r0D, 4);
	
	ad_readback(0x0E, r0E, 8);
	ad_readback(0x0F, r0F, 8);
	ad_readback(0x10, r10, 8);
	ad_readback(0x11, r11, 8);
	ad_readback(0x12, r12, 8);
	ad_readback(0x13, r13, 8);
	ad_readback(0x14, r14, 8);
	ad_readback(0x15, r15, 8);
	
	//ad_readback(0x16, r16, 4);

	printf("Full readback complete\n");
}

// Записать все регистры
// Не трогать недокументированные регистры 0x05 и 0x06
// Не трогать пытаться записывать в оперативную память
void ad_write_all() {	
	ad_pulse_io_reset();

	ad_write(0x00, r00, 4);
	ad_write(0x01, r01, 4);
	ad_write(0x02, r02, 4);
	ad_write(0x03, r03, 4);
	ad_write(0x04, r04, 4);
	//ad_write(0x05, r05, 6);
	//ad_write(0x06, r06, 6);
	ad_write(0x07, r07, 4);
	ad_write(0x08, r08, 2);
	ad_write(0x09, r09, 4);
	ad_write(0x0A, r0A, 4);
	ad_write(0x0B, r0B, 8);
	ad_write(0x0C, r0C, 8);
	ad_write(0x0D, r0D, 4);
	
	ad_write(0x0E, r0E, 8);
	ad_write(0x0F, r0F, 8);
	ad_write(0x10, r10, 8);
	ad_write(0x11, r11, 8);
	ad_write(0x12, r12, 8);
	ad_write(0x13, r13, 8);
	ad_write(0x14, r14, 8);
	ad_write(0x15, r15, 8);
	
	//ad_write(0x16, r16, 4);
}

// Тест с переключением SYNC_CLK
// Полезно для тестирования задержек
void ad_toggle_sync_clk() {	
	static uint8_t cnt = 1;
	
	if (cnt % 2) {
		r01[1] &= 0x00;
	} else {
		r01[1] |= 0xBF;
	}
	cnt++;
	
	ad_write(0x01, r01, 4);
	
	my_delay(1024);
	ad_pulse_io_update();
}

// Вычислить значение FTW для указанной частоты
// Если частота превышает SYSCLK/2, то на выходе получится противоположная частота, т.е. f_настоящая = SYSCLK - f_указанная
uint32_t ad_calc_ftw(uint32_t freq_hz) {
	double ratio = (double)freq_hz / (double)ad_system_clock;
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);
	
	return ftw;
}

// Вычислить размер шага, необходимый, чтобы пройти с частоты f1 до f2 за указанное время
// Используется минимальная возможная задержка между шагами
//
// Вычисления имеют следующий вид:
//
// sysclk = 1 GHz
// fstep = sysclk / 2^31
// tstep = 1s / 1s / (sysclk / 4)
//
// f1 = 10 MHz
// f2 = 20 MHz
// f_delta = f2 - f1
// t_delta = 1 ms
//
// coverage = f_delta / (fstep * (t_delta / tstep))
// req_tstep = coverage < 1.0 ? tstep * round(1/coverage) : tstep
// req_fstep = f_delta / (t_delta / req_tstep)
// ftw = round(2^31 * (req_fstep/sysclk))
//
ad_ramp_t ad_calc_ramp(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns) {
	double fstep_hz = ad_system_clock / 4294967296.0;
	double tstep_ns = 1*1000*1000*1000 / (ad_system_clock / 4);
	double f_delta = (double)f2_hz - (double)f1_hz;
	double t_delta = (double)time_ns;

	if (f_delta < 0.0)
		f_delta = -f_delta;

	double coverage = f_delta / (fstep_hz * (t_delta / tstep_ns));
	double req_tstep = coverage < 1.0 ? tstep_ns * (uint32_t)(1.0 / coverage + 0.5) : tstep_ns;
	double req_fstep = f_delta / (t_delta / req_tstep);

	double ratio = req_fstep / (double)ad_system_clock;	
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);

	printf("Req fstep: %lf\n", req_fstep);
	printf("Req tstep: %lf\n", req_tstep);
	
	return (ad_ramp_t){
		.fstep_ftw = ftw,
		.tstep_mul = (uint32_t)(req_tstep / tstep_ns)
	};
}

// Установить частоту в указанном профиле
void ad_set_profile_freq(int profile_id, uint32_t freq_hz) {
	uint8_t* profile = regmap[14 + profile_id];
	uint32_t ftw = ad_calc_ftw(freq_hz);
	uint8_t* view = (uint8_t*)&ftw;
	
	profile[7] = view[0];
	profile[6] = view[1];
	profile[5] = view[2];
	profile[4] = view[3];
}

// Установить амплитуду в указанном профиле
void ad_set_profile_amplitude(int profile_id, uint16_t amplitude) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&amplitude;
	
	profile[1] = view[0];
	profile[0] = view[1] & 0x3F;
}

// Установить сдвиг фазы в указанном профиле
void ad_set_profile_phase(int profile_id, uint16_t phase) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&phase;
	
	profile[3] = view[0];
	profile[2] = view[1];
}

//
// Digital ramp
//

// Включить Digital Ramp Generator в режиме частоты
// Частота в профилях перестанет иметь эффект (Но амплитуда всё ещё продолжит применяться в соответствии с профилем)
void ad_enable_ramp() {
	r01[1] &= ~0b00111110; // Занулить все связанные с DRG биты
	r01[1] |=  0b00001000; // Включить в режиме частоты

	r00[2] &= ~0b11000100; // Занулить все связанные с DRG биты
	r00[2] |=  0b01000000; // Автосброс таймера (На нижний лимит)
}

// Выключить Digital Ramp Generator
void ad_disable_ramp() {
	r01[1] &= ~0b00111110; // Занулить все связанные с DRG биты
}

// Установка начальной и конечной частоты
void ad_set_ramp_limits(uint32_t f1_hz, uint32_t f2_hz) {
	uint32_t lower;
	uint32_t upper;

	// Если конечная частота выше начальной частоты, то всё нормально
	// Если же конечная частота НИЖЕ начальной частоты, то нужно использовать "зеркальные" значения частоты
	// В даташите это называется "aliased image"
	if (f2_hz > f1_hz) {
		lower = ad_calc_ftw(f1_hz);
		upper = ad_calc_ftw(f2_hz);
	} else {
		lower = ad_calc_ftw(1000*1000*1000 - f1_hz);
		upper = ad_calc_ftw(1000*1000*1000 - f2_hz);
	}

	uint8_t* view_u = (uint8_t*)&upper;
	uint8_t* view_l = (uint8_t*)&lower;

	r0B[0] = view_u[3];
	r0B[1] = view_u[2];
	r0B[2] = view_u[1];
	r0B[3] = view_u[0];

	r0B[4] = view_l[3];
	r0B[5] = view_l[2];
	r0B[6] = view_l[1];
	r0B[7] = view_l[0];
}

// Установка шага
// Подойдут значения от ad_calc_ftw(), либо 1 1 для максимальной плавности
void ad_set_ramp_step(uint32_t decrement, uint32_t increment) {
	uint8_t* view_d = (uint8_t*)&decrement;
	uint8_t* view_i = (uint8_t*)&increment;

	r0C[0] = view_d[3];
	r0C[1] = view_d[2];
	r0C[2] = view_d[1];
	r0C[3] = view_d[0];

	r0C[4] = view_i[3];
	r0C[5] = view_i[2];
	r0C[6] = view_i[1];
	r0C[7] = view_i[0];
}

// Установка задержки между шагами
// Формула для вычисления разрешения: 1s / SYSCLK/4
// Если выставить 0 0, то задержка между шагами будет нулевой и счётчик... никуда не пойдёт
// Для получения максимально быстрого шага нужно выставить значения 1 1
void ad_set_ramp_rate(uint16_t down_rate, uint16_t up_rate) {
	uint8_t* view_d = (uint8_t*)&down_rate;
	uint8_t* view_u = (uint8_t*)&up_rate;

	r0D[0] = view_d[1];
	r0D[1] = view_d[0];

	r0D[2] = view_u[1];
	r0D[3] = view_u[0];
}

// Выключить static reset фазы
// Не изменяет снимок регистров; предполагается, что там static reset всегда должен быть включен
// Бит будет применён при первом переключении профиля
void ad_drop_phase_static_reset() {
	uint8_t altered_r00[4] = {
		r00[0],
		r00[1],
		r00[2] & ~0b00001000,
		r00[3]
	};

	ad_write(0x00, altered_r00, 4);
}

void ad_init() {
	init_profile_gpio();
	init_control_gpio();
	set_profile(0);
	ad_enable_amplitude_scaler();

	// Ramp Generator
	drctl_software_controlled();
	drhold_software_controlled();
	set_ramp_direction(1);

	// SDIO Input Only
	r00[3] = 0x02;

	// PLL
	r02[0] = 0x0D; // VCO + XTAL out enable/disable
	r02[1] = 0x3F; // Charge pump current
	r02[2] = 0xC1; // Divider disable + PLL enable
	r02[3] = 0x64; // x50 multiplier

	//r02[2] = 0xC0; // Divider disable + PLL disable
	
	ad_system_clock = 1000*1000*1000;
	
	r00[2] = 0b00001000; // Phase static reset
	r01[3] = 0b10000000; // Matched latency
	r00[1] = 0b00000001; // Sine output


	// Записать измененные регистры
	ad_write(0x00, r00, 4);
	ad_write(0x01, r01, 4);
	ad_write(0x02, r02, 4);
	
	ad_pulse_io_update();
}
