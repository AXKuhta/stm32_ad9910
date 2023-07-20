
#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"

extern uint32_t perf_wakeups;

void vApplicationIdleHook() {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    perf_wakeups++;
}
