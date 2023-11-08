
#include <stddef.h>

#include "FreeRTOS.h"

#define _heapStart _ebss
#define _heapEnd _estack

extern uint8_t _heapStart;  // located at the first byte of heap
extern uint8_t _heapEnd;    // located at the first byte after the heap

// Must be called before any memory allocations
void init_allocator() {
	size_t heap_size = (size_t)&_heapEnd - (size_t)&_heapStart;

	// FreeRTOS-Kernel/portable/MemMang/heap_5.c
	HeapRegion_t xHeapRegions[] = {
		{ ( uint8_t * ) &_heapStart, heap_size },
		{ NULL, 0 }
	};

	vPortDefineHeapRegions(xHeapRegions);
}

void* malloc(size_t size) {
	return pvPortMalloc(size);
}

void free(void* memory) {
	vPortFree(memory);
}
