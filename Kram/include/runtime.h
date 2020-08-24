#pragma once

#include "common.h"
#include "opcodes.h"
#include "bindata.h"

namespace kram::runtime
{
	struct CallInfo;
	struct Stack;
	struct RuntimeState;

	typedef UInt16 InstructionOffset;
	typedef UInt8 RegisterOffset;
	typedef Size DataOffset;
	typedef Size ChunkOffset;
	typedef Size FunctionOffset;

	typedef std::byte StackUnit;

	enum class ErrorCode
	{
		OK = 0,

		CallStack_Overflow,
		CallStack_Empty,
		RuntimeStack_Overflow
	};

	struct CallInfo
	{
		op::Opcode* inst;
		bin::Chunk* chunk;

		std::ptrdiff_t top;
		std::ptrdiff_t regs;
		std::ptrdiff_t data;

		RegisterOffset returnRegisterOffset;

		UInt8 __padding[7];
	};

	struct Stack
	{
		StackUnit* roof;
		StackUnit* base;

		StackUnit* top;
		Register* regs;
		StackUnit* data;
		CallInfo* cinfo;
	};

	struct RuntimeState
	{
		Stack* stack;
		Heap* heap;

		bin::Chunk* chunk;
		op::Opcode* inst;

		bool exit;

		ErrorCode error;

		RuntimeState(Stack* stack, Heap* heap);
	};

	void _build_stack(Stack* stack, Size size);
	void _resize_stack(Stack* stack, Size extra = 0);
	void _destroy_stack(Stack* stack);

	void execute(KramState* kstate, bin::Chunk* chunk, FunctionOffset function);
}


