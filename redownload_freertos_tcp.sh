#!/bin/bash

git clone https://github.com/evpopov/FreeRTOS-Plus-TCP --branch=MCast_PR --depth=1 --no-checkout
cd FreeRTOS-Plus-TCP

# :(glob)source/*.c означает восстановление файлов в source/, но не глубже, т.е. не уходит в поддиректории
git checkout origin/MCast_PR ":(glob)source/*.c" source/include source/portable/NetworkInterface/Common/ source/portable/NetworkInterface/STM32/NetworkInterface.c source/portable/NetworkInterface/STM32/Drivers/F7 source/portable/NetworkInterface/include/ source/portable/BufferManagement/BufferAllocation_1.c source/portable/Compiler/GCC/

cd ..

# FreeRTOS Plus TCP поставляет свои версии этих файлов, и оригиналы должны быть удалены 
rm -vf STM32CubeF7/Drivers/STM32F7xx_HAL_Driver/Inc/stm32f7xx_hal_eth.h STM32CubeF7/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c
