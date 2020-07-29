#pragma once

#include "common.h"
#include "native_mem.h"

namespace kram
{
	class Heap
	{
	public:
		typedef __kram_heap_header Header;

	private:
		__kram_heap _heap;

	public:
		Heap(Size size);
		Heap(Heap&& heap) noexcept;
		~Heap();

		Heap& operator= (Heap&& right) noexcept;

		types::Pointer malloc(Size size);
		void free(types::Pointer ptr);

		Header& header(types::Pointer ptr);
		const Header& header(types::Pointer ptr) const;

	public:
		Heap(const Heap&) = delete;
		Heap& operator= (const Heap&) = delete;
	};

	class KramState : public Heap
	{

	};
}
