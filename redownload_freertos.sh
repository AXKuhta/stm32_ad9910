#!/bin/bash

git clone https://github.com/FreeRTOS/FreeRTOS-Kernel --depth=1 --no-checkout
cd "FreeRTOS-Kernel"

# https://css-tricks.com/git-pathspecs-and-how-to-use-them/
git checkout main ":(glob)*.c" include/ portable/GCC/ARM_CM7/
