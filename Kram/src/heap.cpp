#include "heap.h"

namespace kram
{
	static inline void free_block(Heap::Header* node)
	{
		delete reinterpret_cast<std::byte*>(node);
	}

	Heap::Heap() :
		_last{ nullptr },
		_size{ 0 }
	{}
	Heap::~Heap()
	{
		for (Header* node = _last, *prev; node; node = prev)
		{
			prev = node->prev;
			free_block(node);
		}
		_last = nullptr;
		_size = 0;
	}

	void* Heap::malloc(Size block_size, bool assign_ref)
	{
		Header* header = reinterpret_cast<Heap::Header*>(new std::byte[block_size + sizeof(Heap::Header)]);
		void* block = reinterpret_cast<void*>(header + 1);
		
		header->next = nullptr;
		header->prev = _last;
		header->refs = assign_ref & 0x1U;
		header->size = block_size;

		if (!_last)
			_last = header;
		else
		{
			_last->next = header;
			_last = header;
		}

		_size++;

		return block;
	}
	void Heap::_free(Header* node)
	{
		if (node == _last)
		{
			_last = node->prev;
			if (_last)
				_last->next = nullptr;
			free_block(node);
		}
		else
		{
			node->next->prev = node->prev;
			node->prev->next = node->next;
			free_block(node);
		}
		_size--;
	}

	void Heap::garbage_collector()
	{
		Header* header = _last, *prev = nullptr;
		while (header)
		{
			prev = header->prev;
			if (header->refs == 0)
				_free(header);
			header = prev;
		}
	}
}
