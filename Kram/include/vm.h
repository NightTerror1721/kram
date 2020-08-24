#pragma once

#include "common.h"
#include "heap.h"
#include "runtime.h"

namespace kram
{
	class KramState : public Heap
	{
	private:
		runtime::Stack _rstack;

	public:
		KramState();
		~KramState();

	public:
		friend void runtime::execute(KramState* state, bin::Chunk* chunk, FunctionOffset function);
	};
}
