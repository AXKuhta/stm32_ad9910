
#include <stddef.h>
#include <stdio.h>

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

void print_mem() {
	HeapStats_t stats = {0};

	vPortGetHeapStats(&stats);

	printf("%10d bytes free total\n", stats.xAvailableHeapSpaceInBytes);
	printf("%10d bytes low water mark\n", stats.xMinimumEverFreeBytesRemaining);
	printf("%10d fragments\n", stats.xNumberOfFreeBlocks);
	printf("%10d malloc calls\n", stats.xNumberOfSuccessfulAllocations);
	printf("%10d free calls\n", stats.xNumberOfSuccessfulFrees);
	printf("%10d bytes largest available block\n", stats.xSizeOfLargestFreeBlockInBytes);
	printf("%10d bytes smallest available block\n", stats.xSizeOfSmallestFreeBlockInBytes);
}
