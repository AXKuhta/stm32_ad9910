#include <stdint.h>
#include <assert.h>

#include "ad9910/registers.h"
#include "ad9910/units.h"
#include "ad9910/drg.h"

//
// Digital ramp
//

// Включить Digital Ramp Generator в режиме частоты
// Частота в профилях перестанет иметь эффект (Но амплитуда всё ещё продолжит применяться в соответствии с профилем)
//
// Есть два варианта использования:
// - Работа в режиме остановки на границе
// - Работа в непрерывном осткакивающем режиме
//
// Сейчас используется остановка на границе
//
// Для начала свипирования требуется:
// - Снять static reset
// - Сделать фронт на DR_CTL
//
// DRG можно использовать в непрерывном отскакивающем режиме (выставлены биты No-dwell high + No-dwell low)
//
// В режиме отскока требуется только:
// - Снять static reset
//
// Кроме того:
// - Когда decrement == 0, режим отскока эквивалентен обычному dwell high
// - Когда decrement == 0x7FFFFFFF, режим отскока эквивалентен no-dwell high (FMCW)
//
void ad_enable_ramp() {
	r01[1] &= ~0b00111110; // Занулить все связанные с DRG биты
	r01[1] |=  0b00001000; // Включить + в режиме частоты

	r00[2] &= ~0b11010000; // Занулить все связанные с DRG биты
	r00[2] |=  0b10010000; // Автосброс downcounter-а шагов по смене профиля + static reset DRG
}

// Выключить Digital Ramp Generator
void ad_disable_ramp() {
	r01[1] &= ~0b00111110; // Занулить все связанные с DRG биты
}

// Установка начальной и конечной частоты
void ad_set_ramp_limits(uint32_t lower_ftw, uint32_t upper_ftw) {
	assert(lower_ftw < upper_ftw);

	uint8_t* view_u = (uint8_t*)&upper_ftw;
	uint8_t* view_l = (uint8_t*)&lower_ftw;

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
