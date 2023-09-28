# https://yuukidach.github.io/p/makefile-for-projects-with-subdirectories/

################################################################################
# SOURCE FILES
################################################################################
LDSCRIPT_SUBDIR = STM32CubeF7/Projects/STM32F746ZG-Nucleo/Templates/SW4STM32/STM32F746ZG_Nucleo_ITCM-FLASH
LDSCRIPT = $(LDSCRIPT_SUBDIR)/STM32F746ZGTx_FLASH.ld

FREERTOS_SUBDIR = FreeRTOS-Kernel
FREERTOS_SOURCE = $(wildcard $(FREERTOS_SUBDIR)/*.c) $(wildcard $(FREERTOS_SUBDIR)/portable/GCC/ARM_CM7/r0p1/*.c) $(wildcard freertos_extras/*.c)
FREERTOS_OBJS = $(FREERTOS_SOURCE:c=o)

FREERTOS_PLUS_TCP_SUBDIR = FreeRTOS-Plus-TCP
FREERTOS_PLUS_TCP_SOURCE = $(wildcard $(FREERTOS_PLUS_TCP_SUBDIR)/source/*.c) $(wildcard $(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/NetworkInterface/Common/*.c) $(wildcard $(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/NetworkInterface/STM32Fxx/*.c) $(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/BufferManagement/BufferAllocation_1.c
FREERTOS_PLUS_TCP_OBJS = $(FREERTOS_PLUS_TCP_SOURCE:c=o)

CMSIS_SUBDIR = STM32CubeF7/Drivers/CMSIS
CMSIS_DEVICE_SUBDIR = $(CMSIS_SUBDIR)/Device/ST/STM32F7xx
STARTUP_OBJ = $(CMSIS_DEVICE_SUBDIR)/Source/Templates/gcc/startup_stm32f746xx.o

HAL_SUBDIR = STM32CubeF7/Drivers/STM32F7xx_HAL_Driver
HAL_SOURCE = $(wildcard $(HAL_SUBDIR)/Src/*.c)
HAL_OBJS = $(HAL_SOURCE:c=o)

APP_SOURCE = $(wildcard *.c) $(wildcard ad9910/*.c)
APP_OBJS = $(APP_SOURCE:c=o)

OBJS = $(STARTUP_OBJ) $(HAL_OBJS) $(APP_OBJS) $(FREERTOS_OBJS) $(FREERTOS_PLUS_TCP_OBJS)
################################################################################


################################################################################
# COMPILER FLAGS
################################################################################
DEFINES = -D"USE_HAL_DRIVER" -D"STM32F746xx" -D"USE_STM32F7XX_NUCLEO_144" -D"STM32F7xx=1"
INCLUDE = -I"include/" -I"$(CMSIS_SUBDIR)/Core/Include" -I"$(CMSIS_DEVICE_SUBDIR)/Include" -I"$(HAL_SUBDIR)/Inc" -I"$(FREERTOS_SUBDIR)/include" -I"$(FREERTOS_SUBDIR)/portable/GCC/ARM_CM7/r0p1" -I"$(FREERTOS_PLUS_TCP_SUBDIR)/source/include" -I"$(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/Compiler/GCC/" -I"$(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/NetworkInterface/include/" -I"$(FREERTOS_PLUS_TCP_SUBDIR)/source/portable/NetworkInterface/STM32Fxx/"

# https://github.com/MayaPosch/Nodate/blob/master/arch/stm32/Makefile
MCU_FLAGS := -mcpu=cortex-m7 -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CC	 = arm-none-eabi-gcc
FLAGS	 = $(MCU_FLAGS) $(DEFINES) $(INCLUDE) -g -c -O2 -Wall -Wextra
LFLAGS	 = $(MCU_FLAGS) -T $(LDSCRIPT) -Wl,--print-memory-usage -Wl,--gc-sections -Wl,--wrap=malloc -Wl,--wrap=_malloc_r -Wl,-Map=firmware.map,--cref --specs=nosys.specs --specs=nano.specs -u _printf_float -u _scanf_float
################################################################################


################################################################################
# RULES
################################################################################
all: firmware.elf
	@echo "Done"

firmware.elf: $(OBJS)
	@echo " [LD] firmware.elf"
	@$(CC) -g $^ -o firmware.elf $(LFLAGS)

# All .c files
%.o: %.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -flto -o $@ $<

# Except syscalls without LTO
syscalls.o: syscalls.c
	@echo " [CC]" $<
	@$(CC) $(FLAGS) -o $@ $<

# Silence some warnings found in STM32 HAL code
%/stm32f7xx_hal_pwr.o: FLAGS += -Wno-unused-parameter
%/stm32f7xx_ll_utils.o: FLAGS += -Wno-unused-parameter

# Newlib malloc/free adapter needs a constant defined and a warning silenced
%/heap_useNewlib_ST.o: FLAGS += -D"configISR_STACK_SIZE_WORDS=128" -Wno-unused-parameter

# Startup
%.o: %.s
	@echo " [AS]" $<
	@$(CC) $(FLAGS) -o $@ $<

flash: firmware.elf
	openocd -f board/st_nucleo_f7.cfg -c "reset_config connect_assert_srst" -c "program firmware.elf verify reset exit"

debug:
	/mingw64.exe openocd -f board/st_nucleo_f7.cfg -c "reset_config connect_assert_srst"
	/mingw64.exe winpty gdb-multiarch ./firmware.elf -ex "target extended-remote :3333"

clean:
	@rm -f $(OBJS) firmware.elf firmware.map
################################################################################
