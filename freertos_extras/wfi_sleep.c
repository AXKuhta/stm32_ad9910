
#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"

void vApplicationIdleHook() {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}
