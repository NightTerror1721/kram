#ifndef __KP_KRAM_NATIVE_MEM_H__
#define __KP_KRAM_NATIVE_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

	#include <stdlib.h>

	typedef struct __kram_heap_header
	{
		struct __kram_heap_header* next;
		struct __kram_heap_header* prev;
		size_t size;
		unsigned int refs;

	} __kram_heap_header;

	typedef struct
	{
		int is_static;
		size_t capacity;
		size_t used;
		__kram_heap_header* last;
		void* data;

	} __kram_heap;

	enum heap_status
	{
		HS_OK = 0,
		HS_CANNOT_CREATE = -1,
		HS_HEAP_OVERFLOW = -2
	};

	int kramnm_CreateHeap(__kram_heap* const heap, const size_t size, const int is_static);
	int kramnm_DestroyHeap(__kram_heap* const heap);

	int kramnm_Malloc(__kram_heap* const heap, const size_t size, void** const ptr);
	int kramnm_Free(__kram_heap* const heap, void* const ptr);

	int kramnm_GetHeader(const void* const ptr, __kram_heap_header** const header);
	int kramnm_IncreaseReferenceCounter(void* const ptr);
	int kramnm_DecreaseReferenceCounter(void* const ptr);

	int klangh_RunGarbageCollector(__kram_heap* const heap);

#ifdef __cplusplus
}
#endif

#endif //__KP_KRAM_NATIVE_MEM_H__
