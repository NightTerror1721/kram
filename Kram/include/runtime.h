#pragma once

#include "common.h"
#include "opcodes.h"
#include "bindata.h"

namespace kram::runtime
{
	struct CallInfo;
	struct CallStack;
	struct RegisterStack;
	struct AutodataStack;
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
		bin::Chunk* chunk;

		StackUnit* regsTop;
		StackUnit* regsBottom;

		StackUnit* dataTop;
		StackUnit* dataBottom;
	};

	struct CallStack
	{
		CallInfo* roof;
		CallInfo* base;

		CallInfo* top;

		bool push(RuntimeState* state);
		bool pop(RuntimeState* state);
	};

	struct RegisterStack
	{
		StackUnit* roof;
		StackUnit* base;

		StackUnit* top;
		union
		{
			Register* registers;
			StackUnit* bottom;
		};
	};

	struct AutodataStack
	{
		StackUnit* roof;
		StackUnit* base;

		StackUnit* top;
		StackUnit* bottom;
	};

	struct RuntimeState
	{
		RegisterStack* rstack;
		AutodataStack* dstack;
		CallStack* cstack;
		bin::Chunk** chunk;
		op::inst::Instruction** inst;
		op::inst::Instruction** lastInst;
		ErrorCode error;

		RuntimeState(RegisterStack* rstack, AutodataStack* dstack, CallStack* cstack,
			bin::Chunk** chunk, op::inst::Instruction** inst, op::inst::Instruction** lastInst);

		void set(CallInfo* info);
		void set(bin::Chunk* chunk, RegisterOffset firstRegister);
	};

	void execute(RegisterStack* rstack, AutodataStack* dstack, CallStack* cstack, bin::Chunk* chunk);
}


