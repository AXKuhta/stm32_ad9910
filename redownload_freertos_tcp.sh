#!/bin/bash

git clone https://github.com/FreeRTOS/FreeRTOS-Plus-TCP --branch=V4.0.0 --depth=1 --no-checkout
cd FreeRTOS-Plus-TCP

# :(glob)source/*.c означает восстановление файлов в source/, но не глубже, т.е. не уходит в поддиректории
git checkout origin/V4.0.0 ":(glob)source/*.c" source/include source/portable/NetworkInterface/STM32Fxx/ source/portable/NetworkInterface/include/

cd ..

# FreeRTOS Plus TCP поставляет свои версии этих файлов, и оригиналы должны быть удалены 
rm -vf STM32CubeF7/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_eth.h STM32CubeF7/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c
