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

	typedef UInt64 Register;

	typedef UInt16 InstructionOffset;
	typedef UInt8 RegisterOffset;
	typedef Size ChunkOffset;

	typedef std::byte StackUnit;
	typedef std::byte* StackData;

	enum class ErrorCode
	{
		OK = 0,

		CallStack_Overflow,
		CallStack_Empty,
		RuntimeStack_Overflow
	};

	struct CallInfo
	{
		InstructionOffset returnInstruction;
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
		bool pop(RuntimeState* state);
	};

	struct RuntimeStack
	{
		Size size;
		StackData base;

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
		InstructionOffset* inst;
		ErrorCode error;

		RuntimeState(RuntimeStack* rstack, CallStack* cstack, bin::Chunk** chunk, InstructionOffset* inst);

		void set(CallInfo* info);
	};

	void execute(RuntimeStack* rstack, CallStack* cstack, bin::Chunk* chunk);
}


