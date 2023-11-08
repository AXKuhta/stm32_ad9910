#!/bin/bash

git clone https://github.com/STMicroelectronics/STM32CubeF7 --branch=v1.17.1 --depth=1 --no-checkout
cd STM32CubeF7/
git checkout v1.17.1 Drivers/CMSIS/Core/Include/ Drivers/CMSIS/Device/ST/STM32F7xx/ Drivers/STM32F7xx_HAL_Driver/
rm Drivers/STM32F7xx_HAL_Driver/Src/*_template.c
cd ..
