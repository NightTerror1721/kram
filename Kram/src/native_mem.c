#include "native_mem.h"

#include <string.h>

#define HEADER_SIZE sizeof(__kram_heap_header)

int kramnm_CreateHeap(__kram_heap* const heap, const size_t size, const int is_static)
{
	void* heap_data = malloc(size);
	if (!heap_data)
		return HS_CANNOT_CREATE;

	heap->is_static = is_static ? 1 : 0;
	heap->capacity = size;
	heap->used = 0;
	heap->last = NULL;
	heap->data = heap_data;

	return HS_OK;
}

int kramnm_DestroyHeap(__kram_heap* const heap)
{
	free(heap->data);
	heap->is_static = 0;
	heap->capacity = heap->used = 0;
	heap->last = NULL;
	heap->data = NULL;

	return HS_OK;
}

int kramnm_Malloc(__kram_heap* const heap, const size_t size, void** const ptr)
{
	if (heap->used + size + HEADER_SIZE > heap->capacity)
		return HS_HEAP_OVERFLOW;

	void* data = (void*)(((char*)heap->data) + heap->used);

	__kram_heap_header* header = (__kram_heap_header*)data;
	header->size = size + HEADER_SIZE;
	header->refs = 0;

	if (!heap->last)
	{
		heap->last = header;
		header->next = header->prev = NULL;
	}
	else
	{
		heap->last->next = header;
		header->prev = heap->last;
		heap->last = header;
	}

	heap->used += header->size;
	*ptr = (void*)(header + 1);

	return HS_OK;
}

int kramnm_Free(__kram_heap* const heap, void* const ptr)
{
	if (heap->is_static)
		return HS_OK;

	__kram_heap_header* header = ((__kram_heap_header*)ptr) - 1;
	if (header == heap->last)
	{
		if (heap->last->prev)
			heap->last->prev->next = NULL;
		heap->last = heap->last->prev;

		heap->capacity -= header->size;
	}
	else
	{
		if (header->prev)
			header->prev->next = header->next;
		if (header->next)
			header->next->prev = header->prev;
	}

	return HS_OK;
}

int kramnm_GetHeader(const void* const ptr, __kram_heap_header** const header)
{
	*header = ((__kram_heap_header*)ptr) - 1;
	return HS_OK;
}

int kramnm_IncreaseReferenceCounter(void* const ptr)
{
	(((__kram_heap_header*)ptr) - 1)->refs++;
	return HS_OK;
}

int kramnm_DecreaseReferenceCounter(void* const ptr)
{
	(((__kram_heap_header*)ptr) - 1)->refs--;
	return HS_OK;
}

int klangh_RunGarbageCollector(__kram_heap* const heap)
{
	if (!heap->last || heap->is_static)
		return HS_OK;

	__kram_heap_header* header = (__kram_heap_header*)heap->data, *prev = NULL;
	char* const base_ptr = (char*)heap->data;
	size_t count = 0, used = 0;
	while (header)
	{
		if (!header->refs)
		{
			if (prev)
				prev->next = header->next;
			if (header->next)
				header->next->prev = prev;
		}
		else
		{
			char* data_ptr = (char*)((void*)header);
			if (!prev)
			{
				if (data_ptr - base_ptr > 0)
				{
					memcpy_s(base_ptr, header->size, data_ptr, header->size);
					header = (__kram_heap_header*)base_ptr;
				}
			}
			else
			{
				char* last_ptr = (char*)((void*)prev) + prev->size;
				if (data_ptr - last_ptr > 0)
				{
					memcpy_s(last_ptr, header->size, data_ptr, header->size);
					header = (__kram_heap_header*)base_ptr;
				}
			}
			count++;
			used = header->size;
		}

		prev = header;
		header = header->next;
	}

	if (count == 0)
	{
		heap->last = NULL;
		heap->used = 0;
	}
	else
	{
		heap->last = prev;
		heap->used = used;
	}

	return HS_OK;
}
