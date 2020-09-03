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

	union Registers;
	union Register
	{
		UInt8 u8;
		UInt16 u16;
		UInt32 u32;
		UInt64 u64;

		Int8 s8;
		Int16 s16;
		Int32 s32;
		Int64 s64;

		float f32;
		double f64;

		std::uintptr_t stack_offset;

		op::Opcode opcode;


		UInt8* addr_u8;
		UInt16* addr_u16;
		UInt32* addr_u32;
		UInt64* addr_u64;

		Int8* addr_s8;
		Int16* addr_s16;
		Int32* addr_s32;
		Int64* addr_s64;

		float* addr_f32;
		double* addr_f64;

		bin::Chunk* addr_chunk;

		StackUnit* addr_stack_offset;

		Registers* addr_regs;

		std::byte* addr_bytes;

		op::Opcode* addr_opcode;

		void* addr;


		UInt64 _value;
	};

	union Registers
	{
		struct
		{
			Register r0;
			Register r1;
			Register r2;
			Register r3;
			Register r4;
			Register r5;
			Register r6;
			Register r7;
			Register r8;
			union { Register sd, r9; };
			union { Register sb, r10; };
			union { Register sp, r11; };
			union { Register sr, r12; };
			union { Register ch, r13; };
			union { Register st, r14; };
			union { Register ip, r15; };
		};
		Register by_index[16];
	};

	/*struct CallInfo
	{
		op::Opcode* inst;
		bin::Chunk* chunk;

		std::ptrdiff_t top;
		std::ptrdiff_t regs;
		std::ptrdiff_t data;

		RegisterOffset returnRegisterOffset;

		UInt8 __padding[7];
	};*/

	struct Stack
	{
		StackUnit* roof;
		StackUnit* base;
	};

	struct RuntimeState
	{
		Registers regs;

		Stack* stack;
		Heap* heap;

		bool exit;

		ErrorCode error;

		RuntimeState(Stack* stack, Heap* heap);
	};

	void _build_stack(Stack* stack, Size size);
	void _resize_stack(Stack* stack, Size extra = 0);
	void _destroy_stack(Stack* stack);

	void execute(KramState* kstate, bin::Chunk* chunk, FunctionOffset function);
}


