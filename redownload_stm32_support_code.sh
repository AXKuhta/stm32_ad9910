#!/bin/bash

git clone https://github.com/STMicroelectronics/STM32CubeF7/ --branch=v1.17.3 --depth=1
rm -r STM32CubeF7/*
cd STM32CubeF7/
git checkout Drivers/CMSIS/Core
git submodule update --init Drivers/STM32F7xx_HAL_Driver Drivers/CMSIS/Device/ST/STM32F7xx

# Убрать несобирающиеся файлы-шаблоны
rm -vf Drivers/STM32F7xx_HAL_Driver/Src/*_template.c
