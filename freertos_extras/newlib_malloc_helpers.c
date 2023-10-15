// newlib_malloc_helpers.c

#include <stdint.h>
#include <reent.h>
#include <malloc.h>
#include <errno.h>

#include "FreeRTOS.h"
#include "task.h"

//      These values come from the linker file.
//

#define _heapStart _ebss
#define _heapEnd _estack

extern uint8_t _heapStart;  // located at the first byte of heap
extern uint8_t _heapEnd;    // located at the first byte after the heap

static uint8_t* currentHeapPos = &_heapStart;

size_t xPortGetFreeHeapSize( void )
{
   struct mallinfo info = mallinfo();
   return ( info.fordblks + (size_t)(&_heapEnd - currentHeapPos) );
}

//      The application may choose to override this function.  Note that
// after this function runs, the malloc-failed hook in FreeRTOS is likely
// to be called too.  It won't be called, however, if we're here due to a
// "direct" call to malloc(), not via pvPortMalloc(), such as a call made
// from inside newlib.
//
__attribute__((weak)) void sbrkFailedHook( ptrdiff_t size )
{
   configASSERT(0);
}

//      The implementation of _sbrk_r() included in some builds of newlib
// doesn't enforce a limit in heap size.  This implementation does.
//
void* _sbrk_r(struct _reent *reent, ptrdiff_t size)
{
   void* returnValue;

   if (currentHeapPos + size > &_heapEnd)
   {
      sbrkFailedHook(size);

      reent->_errno = ENOMEM;
      returnValue = (void*)-1;
   }
   else
   {
      returnValue = (void*)currentHeapPos;
      currentHeapPos += size;
   }

   return (returnValue);
}

//      In the pre-emptive multitasking environment provided by FreeRTOS,
// it's possible (though unlikely) that a call from newlib code to the
// malloc family might be preempted by a higher priority task that then
// makes a call to the malloc family.  These implementations of newlib's
// hook functions protect against that.  Most calls to this function come
// via pvPortMalloc(), which has already suspended the scheduler, so the
// call here is redundant most of the time.  That's OK.
//
void __malloc_lock(struct _reent *r)
{
   vTaskSuspendAll();
}

void __malloc_unlock(struct _reent *r)
{
   xTaskResumeAll();
}
