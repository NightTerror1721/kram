#include "runtime.h"

#define rcast(_Type, _Value) reinterpret_cast<_Type>(_Value)

using namespace kram::bin;
using kram::op::inst::Instruction;

namespace kram::runtime
{
	RuntimeState::RuntimeState(Stack* stack) :
		stack{ stack },
		chunk{ nullptr },
		inst{ nullptr },
		lastInst{ nullptr },
		exit{ false },
		error{ ErrorCode::OK }
	{}

	static void main_chunk_call(RuntimeState* state, Chunk* chunk)
	{
		Stack* stack = state->stack;
		CallInfo* info = rcast(CallInfo*, stack->top);

		info->inst = nullptr;
		info->lastInst = nullptr;
		info->chunk = nullptr;

		info->top = stack->top;
		info->regs = stack->regs;
		info->data = stack->data;

		info->returnRegisterOffset = 0;


		stack->data = rcast(StackUnit*, info + 1);
		stack->regs = rcast(Register*, stack->regs + chunk->stackSize);
		stack->top = rcast(StackUnit*, stack->data + chunk->registerCount);

		state->chunk = chunk;
		state->inst = rcast(Instruction*, chunk->code);
		state->lastInst = rcast(Instruction*, chunk->code) + (chunk->codeSize - 1);
	}

	static void call_chunk(RuntimeState* state, ChunkOffset chunkOffset, DataOffset dataOffset, Size dataSize, RegisterOffset retRegOffset)
	{
		Stack* stack = state->stack;
		Chunk* chunk = state->chunk->childChunks + chunkOffset;
		CallInfo* info = rcast(CallInfo*, stack->top);

		info->inst = state->inst;
		info->lastInst = state->lastInst;
		info->chunk = state->chunk;

		info->top = stack->top;
		info->regs = stack->regs;
		info->data = stack->data;

		info->returnRegisterOffset = retRegOffset;


		stack->data = rcast(StackUnit*, info + 1);
		stack->regs = rcast(Register*, stack->regs + chunk->stackSize);
		stack->top = rcast(StackUnit*, stack->data + chunk->registerCount);

		state->chunk = chunk;
		state->inst = rcast(Instruction*, chunk->code);
		state->lastInst = rcast(Instruction*, chunk->code) + (chunk->codeSize - 1);


		std::memcpy(stack->data, info->data + dataOffset, dataSize);
	}

	static void finish_call(RuntimeState* state, RegisterOffset retRegOffset)
	{
		Stack* stack = state->stack;
		CallInfo* info = stack->cinfo;
		auto rr = stack->regs[retRegOffset].reg;
		
		stack->top = info->top;
		stack->regs = info->regs;
		stack->data = info->data;
		stack->cinfo = rcast(CallInfo*, stack->data) - 1;

		state->chunk = info->chunk;
		state->inst = info->inst;
		state->lastInst = info->lastInst;

		if(!(state->exit = !state->inst))
			stack->regs[info->returnRegisterOffset].reg = rr;
	}
}

#define move_pc(_Amount) (state.inst += (_Amount))
#define opcode(_Inst) case _Inst : {
#define end_opcode() } break
#define move_break(_Amount) move_pc(_Amount); break

#define register(_Idx) state.stack->regs[_Idx]

#define scast(_Type, _Value) static_cast<_Type>(_Value)

#define arg_byte(_Idx) (rcast(UInt8*, state.inst)[_Idx])
#define arg_word(_Idx) (*rcast(UInt16*, state.inst + (_Idx)))
#define arg_dword(_Idx) (*rcast(UInt32*, state.inst + (_Idx)))
#define arg_qword(_Idx) (*rcast(UInt64*, state.inst + (_Idx)))

#define arg_sbyte(_Idx) (*rcast(Int8*, state.inst + (_Idx)))
#define arg_sword(_Idx) (*rcast(Int16*, state.inst + (_Idx)))
#define arg_sdword(_Idx) (*rcast(Int32*, state.inst + (_Idx)))
#define arg_sqword(_Idx) (*rcast(Int64*, state.inst + (_Idx)))

#define arg_1bit(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x01U)
#define arg_2bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x03U)
#define arg_3bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x07U)
#define arg_4bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x0FU)
#define arg_5bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x1FU)
#define arg_6bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x3FU)
#define arg_7bits(_Idx, _BitIdx) ((arg_byte(_Idx) >> (_BitIdx)) & 0x7FU)


void kram::runtime::execute(Stack* stack, Chunk* chunk)
{
	RuntimeState state{ stack };
	main_chunk_call(&state, chunk);

	for (;state.inst <= state.lastInst;)
	{
		switch (*state.inst)
		{
			opcode(Instruction::NOP)
				move_pc(1);
			end_opcode();

			opcode(Instruction::MOV)
				switch (arg_2bits(0, 0)) /* size */
				{
					case 0: switch (arg_2bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: register(arg_byte(1)).u8 = register(arg_byte(2)).u8; move_break(4);
							case 1: register(arg_byte(1)).u8 = *rcast(UInt8*, arg_qword(2)); move_break(11);
							case 2: register(arg_byte(1)).u8 = *rcast(UInt8*, chunk->statics[arg_qword(2)].data); move_break(18);
							case 3: register(arg_byte(1)).u8 = arg_byte(2); move_break(4);
						} break;
						case 1: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt8*, arg_qword(1)) = register(arg_byte(9)).u8; move_break(11);
							case 1: *rcast(UInt8*, arg_qword(1)) = *rcast(UInt8*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt8*, arg_qword(1)) = *rcast(UInt8*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt8*, arg_qword(1)) = arg_byte(9); move_break(11);
						} break;
						case 2: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt8*, chunk->statics[arg_qword(1)].data) = register(arg_byte(9)).u8; move_break(11);
							case 1: *rcast(UInt8*, chunk->statics[arg_qword(1)].data) = *rcast(UInt8*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt8*, chunk->statics[arg_qword(1)].data) = *rcast(UInt8*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt8*, chunk->statics[arg_qword(1)].data) = arg_byte(9); move_break(11);
						} break;
					} break;
					case 1: switch (arg_2bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: register(arg_byte(1)).u16 = register(arg_byte(2)).u16; move_break(4);
							case 1: register(arg_byte(1)).u16 = *rcast(UInt16*, arg_qword(2)); move_break(11);
							case 2: register(arg_byte(1)).u16 = *rcast(UInt16*, chunk->statics[arg_qword(2)].data); move_break(18);
							case 3: register(arg_byte(1)).u16 = arg_word(2); move_break(5);
						} break;
						case 1: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt16*, arg_qword(1)) = register(arg_byte(9)).u16; move_break(11);
							case 1: *rcast(UInt16*, arg_qword(1)) = *rcast(UInt16*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt16*, arg_qword(1)) = *rcast(UInt16*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt16*, arg_qword(1)) = arg_word(9); move_break(12);
						} break;
						case 2: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt16*, chunk->statics[arg_qword(1)].data) = register(arg_byte(9)).u16; move_break(11);
							case 1: *rcast(UInt16*, chunk->statics[arg_qword(1)].data) = *rcast(UInt16*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt16*, chunk->statics[arg_qword(1)].data) = *rcast(UInt16*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt16*, chunk->statics[arg_qword(1)].data) = arg_word(9); move_break(12);
						} break;
					} break;
					case 2: switch (arg_2bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: register(arg_byte(1)).u32 = register(arg_byte(2)).u32; move_break(4);
							case 1: register(arg_byte(1)).u32 = *rcast(UInt32*, arg_qword(2)); move_break(11);
							case 2: register(arg_byte(1)).u32 = *rcast(UInt32*, chunk->statics[arg_qword(2)].data); move_break(18);
							case 3: register(arg_byte(1)).u32 = arg_dword(2); move_break(7);
						} break;
						case 1: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt32*, arg_qword(1)) = register(arg_byte(9)).u32; move_break(11);
							case 1: *rcast(UInt32*, arg_qword(1)) = *rcast(UInt32*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt32*, arg_qword(1)) = *rcast(UInt32*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt32*, arg_qword(1)) = arg_dword(9); move_break(14);
						} break;
						case 2: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt32*, chunk->statics[arg_qword(1)].data) = register(arg_byte(9)).u32; move_break(11);
							case 1: *rcast(UInt32*, chunk->statics[arg_qword(1)].data) = *rcast(UInt32*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt32*, chunk->statics[arg_qword(1)].data) = *rcast(UInt32*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt32*, chunk->statics[arg_qword(1)].data) = arg_dword(9); move_break(14);
						} break;
					} break;
					case 3: switch (arg_2bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: register(arg_byte(1)).u64 = register(arg_byte(2)).u64; move_break(4);
							case 1: register(arg_byte(1)).u64 = *rcast(UInt64*, arg_qword(2)); move_break(11);
							case 2: register(arg_byte(1)).u64 = *rcast(UInt64*, chunk->statics[arg_qword(2)].data); move_break(18);
							case 3: register(arg_byte(1)).u64 = arg_qword(2); move_break(11);
						} break;
						case 1: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt64*, arg_qword(1)) = register(arg_byte(9)).u64; move_break(11);
							case 1: *rcast(UInt64*, arg_qword(1)) = *rcast(UInt64*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt64*, arg_qword(1)) = *rcast(UInt64*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt64*, arg_qword(1)) = arg_qword(9); move_break(18);
						} break;
						case 2: switch (arg_2bits(0, 4)) /* src mode */
						{
							case 0: *rcast(UInt64*, chunk->statics[arg_qword(1)].data) = register(arg_byte(9)).u64; move_break(11);
							case 1: *rcast(UInt64*, chunk->statics[arg_qword(1)].data) = *rcast(UInt64*, arg_qword(9)); move_break(18);
							case 2: *rcast(UInt64*, chunk->statics[arg_qword(1)].data) = *rcast(UInt64*, chunk->statics[arg_qword(9)].data); move_break(18);
							case 3: *rcast(UInt64*, chunk->statics[arg_qword(1)].data) = arg_qword(9); move_break(18);
						} break;
					} break;
				}
			end_opcode();
		}
	}
}

