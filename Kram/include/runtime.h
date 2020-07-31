#pragma once

#include "common.h"
#include "opcodes.h"
#include "bindata.h"

namespace kram::runtime
{
	struct CallInfo;
	struct CallStack;
	struct RuntimeStack;
	struct RuntimeState;

	typedef UInt16 InstructionOffset;
	typedef UInt8 RegisterOffset;
	typedef Size ChunkOffset;

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
		op::inst::Instruction* inst;
		op::inst::Instruction* lastInst;
		RegisterOffset returnRegister;
		bin::Chunk* chunk;

		StackUnit* top;
		StackUnit* data;
		StackUnit* bottom;
	};

	struct CallStack
	{
		CallInfo* roof;
		CallInfo* base;

		CallInfo* top;

		bool push(RuntimeState* state, RegisterOffset registerToReturn);
		bool pushFirst(RuntimeState* state);
		bool pop(RuntimeState* state);
	};

	struct RuntimeStack
	{
		Size size;
		StackUnit* roof;
		StackUnit* base;

		StackUnit* top;
		StackUnit* data;
		union
		{
			Register* registers;
			StackUnit* bottom;
		};
	};

	struct RuntimeState
	{
		RuntimeStack* rstack;
		CallStack* cstack;
		bin::Chunk** chunk;
		op::inst::Instruction** inst;
		op::inst::Instruction** lastInst;
		ErrorCode error;

		RuntimeState(RuntimeStack* rstack, CallStack* cstack, bin::Chunk** chunk, op::inst::Instruction** inst, op::inst::Instruction** lastInst);

		void set(CallInfo* info);
		void set(bin::Chunk* chunk);
	};

	void execute(RuntimeStack* rstack, CallStack* cstack, bin::Chunk* chunk);
}


