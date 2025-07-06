#!/bin/bash

git clone https://github.com/FreeRTOS/FreeRTOS-Kernel --branch=V11.1.0 --depth=1 --no-checkout
cd "FreeRTOS-Kernel"

# https://css-tricks.com/git-pathspecs-and-how-to-use-them/
git checkout V11.1.0 ":(glob)*.c" include/ portable/GCC/ARM_CM7/
