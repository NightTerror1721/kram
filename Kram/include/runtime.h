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

		StackUnit* top;
		Register* regs;
		StackUnit* data;

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

		bin::Chunk* chunk;
		op::Opcode* inst;

		bool exit;

		ErrorCode error;

		RuntimeState(Stack* stack);
	};

	void execute(Stack* stack, bin::Chunk* chunk, FunctionOffset function);
}


