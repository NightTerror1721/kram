#pragma once

#include "common.h"

namespace kram
{
	class Heap
	{
	public:
		struct Header
		{
			Header* next;
			Header* prev;
			Size size;
			UInt64 refs;
		};

	private:
		Header* _last;
		Size _size;

	public:
		Heap();
		~Heap();

		void* malloc(Size block_size, bool assign_ref = true);
		inline void free(void* ptr) { _free(reinterpret_cast<Header*>(ptr) - 1); }

		static inline Header& header(void* ptr) { return *(reinterpret_cast<Header*>(ptr) - 1); }
		static inline const Header& header(const void* ptr) { return *(reinterpret_cast<const Header*>(ptr) - 1); }

		static inline void increase_ref(void* ptr)
		{
			Header* header = reinterpret_cast<Header*>(ptr) - 1;
			if (header->refs < static_cast<decltype(header->refs)>(-1))
				header->refs++;
		}
		static inline void decrease_ref(void* ptr)
		{
			Header* header = reinterpret_cast<Header*>(ptr) - 1;
			if (header->refs > 0)
				header->refs--;
		}

		void garbage_collector();

	private:
		void _free(Header* header);
	};
}
