#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"
#include "tasks.h"

int main(void) {
	system_init();

	/* Infinite loop */
	while (1) {
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		isr_recorder_sync();
		run_all_tasks();
	}
}
