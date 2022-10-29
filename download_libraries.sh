#!/bin/bash

git clone https://github.com/STMicroelectronics/STM32CubeF7 --depth=1 --no-checkout
cd STM32CubeF7/
git checkout master Drivers/CMSIS/ Drivers/STM32F7xx_HAL_Driver/ Projects/STM32F746ZG-Nucleo/Templates/SW4STM32/STM32F746ZG_Nucleo_ITCM-FLASH/
cd ..
