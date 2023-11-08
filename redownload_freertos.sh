#!/bin/bash

git clone https://github.com/FreeRTOS/FreeRTOS-Kernel --branch=V10.6.0 --depth=1 --no-checkout
cd "FreeRTOS-Kernel"

# https://css-tricks.com/git-pathspecs-and-how-to-use-them/
git checkout V10.6.0 ":(glob)*.c" include/ portable/GCC/ARM_CM7/ portable/MemMang/heap_5.c
