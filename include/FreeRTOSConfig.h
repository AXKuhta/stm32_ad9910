#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
extern uint32_t SystemCoreClock;

#define configUSE_PREEMPTION					1
#define configIDLE_SHOULD_YIELD					1
#define configUSE_TICKLESS_IDLE					0
#define configSUPPORT_DYNAMIC_ALLOCATION		1
#define configUSE_IDLE_HOOK						1
#define configUSE_TICK_HOOK						0
#define configCPU_CLOCK_HZ						( SystemCoreClock )
#define configTICK_RATE_HZ						( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES					( 3 ) // 0 = idle task, 1 = normal task, 2 = software timer
#define configMINIMAL_STACK_SIZE				( ( uint16_t ) 128 )
#define configMAX_TASK_NAME_LEN					( 16 )
#define configUSE_TRACE_FACILITY				0
#define configUSE_16_BIT_TICKS					0
#define configUSE_MUTEXES						1
#define configQUEUE_REGISTRY_SIZE				8
#define configUSE_RECURSIVE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES			1
#define configUSE_MALLOC_FAILED_HOOK			1
#define configCHECK_FOR_STACK_OVERFLOW			2
#define configUSE_NEWLIB_REENTRANT				0
#define configRECORD_STACK_HIGH_ADDRESS			1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 1

/* Defaults to size_t for backward compatibility, but can be changed
 * if lengths will always be less than the number of bytes in a size_t. */
#define configMESSAGE_BUFFER_LENGTH_TYPE		size_t
/* USER CODE END MESSAGE_BUFFER_LENGTH_TYPE */

/* Software timer definitions. */
#define configUSE_TIMERS						0
#define configTIMER_TASK_PRIORITY				( 2 )
#define configTIMER_QUEUE_LENGTH				10
#define configTIMER_TASK_STACK_DEPTH			256

/* Set the following definitions to 1 to include the API function, or zero
 * to exclude the API function. */
#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskCleanUpResources			0
#define INCLUDE_vTaskSuspend					0
#define INCLUDE_vTaskDelayUntil					0
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTaskGetSchedulerState			1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
	/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
	#define configPRIO_BITS						__NVIC_PRIO_BITS
#else
	#define configPRIO_BITS						4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
 * function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY	15

/* The highest interrupt priority that can be used by any interrupt service
 * routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT
 * CALL INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A
 * HIGHER PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
 * to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY				( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY		( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Normal assert() semantics without relying on the provision of an assert.h
 * header file. */
#define configASSERT( x )							if ( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

#endif
