#include "vm.h"

namespace kram
{
	Heap::Heap(Size size) :
		_heap{}
	{
		kramnm_CreateHeap(&_heap, size, false);
	}
	Heap::Heap(Heap&& heap) noexcept :
		_heap{ std::move(heap._heap) }
	{
		std::memset(&heap._heap, 0, sizeof(heap._heap));
	}
	Heap::~Heap()
	{
		kramnm_DestroyHeap(&_heap);
	}

	Heap& Heap::operator= (Heap&& right) noexcept
	{
		kramnm_DestroyHeap(&_heap);
		_heap = std::move(right._heap);
		std::memset(&right._heap, 0, sizeof(right._heap));
		return *this;
	}

	types::Pointer Heap::malloc(Size size)
	{
		types::Pointer ptr;
		return kramnm_Malloc(&_heap, size, &ptr) != HS_OK ? nullptr : ptr;
	}
	void Heap::free(types::Pointer ptr)
	{
		kramnm_Free(&_heap, ptr);
	}

	Heap::Header& Heap::header(types::Pointer ptr)
	{
		Header* header;
		kramnm_GetHeader(ptr, &header);
		return *header;
	}
	const Heap::Header& Heap::header(types::Pointer ptr) const
	{
		Header* header;
		kramnm_GetHeader(ptr, &header);
		return *header;
	}
}
