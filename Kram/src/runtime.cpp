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
#define end_opcode() } goto instruction_begin
#define movpc_endop(_Amount) move_pc(_Amount); goto instruction_begin

#define register(_Idx) state.stack->regs[_Idx]

#define register_data(_Part, _Idx) *register(_Idx).addr_ ## _Part
#define register_data_d(_Type, _Idx, _Delta) *rcast(_Type*, register(_Idx).addr_u8 + (_Delta))

#define stack_data(_Type, _Idx) *rcast(_Type*, state.stack->data + (_Idx))
#define stack_data_d(_Type, _Idx, _Delta) *rcast(_Type*, state.stack->data + ((_Idx) + (_Delta)))

#define static_data(_Type, _Idx) *rcast(_Type*, chunk->statics[_Idx].data)
#define static_data_d(_Type, _Idx, _Delta) *rcast(_Type*, rcast(UInt8*, chunk->statics[_Idx].data) + (_Delta))

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
		instruction_begin:
		switch (*state.inst)
		{
			opcode(Instruction::NOP)
				move_pc(1);
			end_opcode();

			opcode(Instruction::MOV)
				switch (arg_2bits(0, 0)) /* size */
				{
					case 0: switch (arg_3bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register(arg_byte(1)).u8 = register(arg_byte(2)).u8; movpc_endop(4);
							case 1: register(arg_byte(1)).u8 = arg_byte(2); movpc_endop(4);
							case 2: register(arg_byte(1)).u8 = stack_data(UInt8, arg_qword(2)); movpc_endop(11);
							case 3: register(arg_byte(1)).u8 = stack_data_d(UInt8, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register(arg_byte(1)).u8 = static_data(UInt8, arg_qword(2)); movpc_endop(11);
							case 5: register(arg_byte(1)).u8 = static_data_d(UInt8, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register(arg_byte(1)).u8 = register_data(u8, arg_byte(2)); movpc_endop(4);
							case 7: register(arg_byte(1)).u8 = register_data_d(UInt8, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 1: movpc_endop(1);
						case 2: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data(UInt8, arg_qword(1)) = register(arg_byte(9)).u8; movpc_endop(11);
							case 1: stack_data(UInt8, arg_qword(1)) = arg_byte(9); movpc_endop(11);
							case 2: stack_data(UInt8, arg_qword(1)) = stack_data(UInt8, arg_qword(9)); movpc_endop(18);
							case 3: stack_data(UInt8, arg_qword(1)) = stack_data_d(UInt8, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: stack_data(UInt8, arg_qword(1)) = static_data(UInt8, arg_qword(9)); movpc_endop(18);
							case 5: stack_data(UInt8, arg_qword(1)) = static_data_d(UInt8, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: stack_data(UInt8, arg_qword(1)) = register_data(u8, arg_byte(9)); movpc_endop(11);
							case 7: stack_data(UInt8, arg_qword(1)) = register_data_d(UInt8, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 3: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u8; movpc_endop(15);
							case 1: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = arg_byte(13); movpc_endop(15);
							case 2: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = stack_data(UInt8, arg_qword(13)); movpc_endop(22);
							case 3: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = stack_data_d(UInt8, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 4: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = static_data(UInt8, arg_qword(13)); movpc_endop(22);
							case 5: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = static_data_d(UInt8, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 6: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = register_data(u8, arg_byte(13)); movpc_endop(15);
							case 7: stack_data_d(UInt8, arg_qword(1), arg_dword(9)) = register_data_d(UInt8, arg_byte(13), arg_dword(14)); movpc_endop(19);
						} break;
						case 4: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data(UInt8, arg_qword(1)) = register(arg_byte(9)).u8; movpc_endop(11);
							case 1: static_data(UInt8, arg_qword(1)) = arg_byte(9); movpc_endop(11);
							case 2: static_data(UInt8, arg_qword(1)) = stack_data(UInt8, arg_qword(9)); movpc_endop(18);
							case 3: static_data(UInt8, arg_qword(1)) = stack_data_d(UInt8, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: static_data(UInt8, arg_qword(1)) = static_data(UInt8, arg_qword(9)); movpc_endop(18);
							case 5: static_data(UInt8, arg_qword(1)) = static_data_d(UInt8, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: static_data(UInt8, arg_qword(1)) = register_data(u8, arg_byte(9)); movpc_endop(11);
							case 7: static_data(UInt8, arg_qword(1)) = register_data_d(UInt8, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 5: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u8; movpc_endop(15);
							case 1: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = arg_byte(13); movpc_endop(15);
							case 2: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = stack_data(UInt8, arg_qword(13)); movpc_endop(22);
							case 3: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = stack_data_d(UInt8, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 4: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = static_data(UInt8, arg_qword(13)); movpc_endop(22);
							case 5: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = static_data_d(UInt8, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 6: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = register_data(u8, arg_byte(13)); movpc_endop(15);
							case 7: static_data_d(UInt8, arg_qword(1), arg_dword(9)) = register_data_d(UInt8, arg_byte(13), arg_dword(10)); movpc_endop(19);
						} break;
						case 6: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data(u8, arg_byte(1)) = register(arg_byte(2)).u8; movpc_endop(4);
							case 1: register_data(u8, arg_byte(1)) = arg_byte(2); movpc_endop(4);
							case 2: register_data(u8, arg_byte(1)) = stack_data(UInt8, arg_qword(2)); movpc_endop(11);
							case 3: register_data(u8, arg_byte(1)) = stack_data_d(UInt8, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register_data(u8, arg_byte(1)) = static_data(UInt8, arg_qword(2)); movpc_endop(11);
							case 5: register_data(u8, arg_byte(1)) = static_data_d(UInt8, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register_data(u8, arg_byte(1)) = register_data(u8, arg_byte(2)); movpc_endop(4);
							case 7: register_data(u8, arg_byte(1)) = register_data_d(UInt8, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 7: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = register(arg_byte(6)).u8; movpc_endop(8);
							case 1: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = arg_byte(6); movpc_endop(8);
							case 2: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = stack_data(UInt8, arg_qword(6)); movpc_endop(15);
							case 3: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = stack_data_d(UInt8, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 4: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = static_data(UInt8, arg_qword(6)); movpc_endop(15);
							case 5: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = static_data_d(UInt8, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 6: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = register_data(u8, arg_byte(6)); movpc_endop(8);
							case 7: register_data_d(UInt8, arg_byte(1), arg_dword(2)) = register_data_d(UInt8, arg_byte(6), arg_dword(7)); movpc_endop(12);
						} break;
					} break;
					case 1: switch (arg_3bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register(arg_byte(1)).u16 = register(arg_byte(2)).u16; movpc_endop(4);
							case 1: register(arg_byte(1)).u16 = arg_word(2); movpc_endop(5);
							case 2: register(arg_byte(1)).u16 = stack_data(UInt16, arg_qword(2)); movpc_endop(11);
							case 3: register(arg_byte(1)).u16 = stack_data_d(UInt16, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register(arg_byte(1)).u16 = static_data(UInt16, arg_qword(2)); movpc_endop(11);
							case 5: register(arg_byte(1)).u16 = static_data_d(UInt16, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register(arg_byte(1)).u16 = register_data(u16, arg_byte(2)); movpc_endop(4);
							case 7: register(arg_byte(1)).u16 = register_data_d(UInt16, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 1: movpc_endop(1);
						case 2: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data(UInt16, arg_qword(1)) = register(arg_byte(9)).u16; movpc_endop(11);
							case 1: stack_data(UInt16, arg_qword(1)) = arg_word(9); movpc_endop(12);
							case 2: stack_data(UInt16, arg_qword(1)) = stack_data(UInt16, arg_qword(9)); movpc_endop(18);
							case 3: stack_data(UInt16, arg_qword(1)) = stack_data_d(UInt16, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: stack_data(UInt16, arg_qword(1)) = static_data(UInt16, arg_qword(9)); movpc_endop(18);
							case 5: stack_data(UInt16, arg_qword(1)) = static_data_d(UInt16, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: stack_data(UInt16, arg_qword(1)) = register_data(u16, arg_byte(9)); movpc_endop(11);
							case 7: stack_data(UInt16, arg_qword(1)) = register_data_d(UInt16, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 3: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u16; movpc_endop(15);
							case 1: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(16);
							case 2: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = stack_data(UInt16, arg_qword(13)); movpc_endop(22);
							case 3: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = stack_data_d(UInt16, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 4: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = static_data(UInt16, arg_qword(13)); movpc_endop(22);
							case 5: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = static_data_d(UInt16, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 6: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = register_data(u16, arg_byte(13)); movpc_endop(15);
							case 7: stack_data_d(UInt16, arg_qword(1), arg_dword(9)) = register_data_d(UInt16, arg_byte(13), arg_dword(14)); movpc_endop(19);
						} break;
						case 4: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data(UInt16, arg_qword(1)) = register(arg_byte(9)).u16; movpc_endop(11);
							case 1: static_data(UInt16, arg_qword(1)) = arg_word(9); movpc_endop(12);
							case 2: static_data(UInt16, arg_qword(1)) = stack_data(UInt16, arg_qword(9)); movpc_endop(18);
							case 3: static_data(UInt16, arg_qword(1)) = stack_data_d(UInt16, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: static_data(UInt16, arg_qword(1)) = static_data(UInt16, arg_qword(9)); movpc_endop(18);
							case 5: static_data(UInt16, arg_qword(1)) = static_data_d(UInt16, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: static_data(UInt16, arg_qword(1)) = register_data(u16, arg_byte(9)); movpc_endop(11);
							case 7: static_data(UInt16, arg_qword(1)) = register_data_d(UInt16, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 5: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u16; movpc_endop(15);
							case 1: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(16);
							case 2: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = stack_data(UInt16, arg_qword(13)); movpc_endop(22);
							case 3: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = stack_data_d(UInt16, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 4: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = static_data(UInt16, arg_qword(13)); movpc_endop(22);
							case 5: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = static_data_d(UInt16, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 6: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = register_data(u16, arg_byte(13)); movpc_endop(15);
							case 7: static_data_d(UInt16, arg_qword(1), arg_dword(9)) = register_data_d(UInt16, arg_byte(13), arg_dword(10)); movpc_endop(19);
						} break;
						case 6: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data(u16, arg_byte(1)) = register(arg_byte(2)).u16; movpc_endop(4);
							case 1: register_data(u16, arg_byte(1)) = arg_word(2); movpc_endop(5);
							case 2: register_data(u16, arg_byte(1)) = stack_data(UInt16, arg_qword(2)); movpc_endop(11);
							case 3: register_data(u16, arg_byte(1)) = stack_data_d(UInt16, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register_data(u16, arg_byte(1)) = static_data(UInt16, arg_qword(2)); movpc_endop(11);
							case 5: register_data(u16, arg_byte(1)) = static_data_d(UInt16, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register_data(u16, arg_byte(1)) = register_data(u16, arg_byte(2)); movpc_endop(4);
							case 7: register_data(u16, arg_byte(1)) = register_data_d(UInt16, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 7: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = register(arg_byte(6)).u16; movpc_endop(8);
							case 1: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = arg_word(6); movpc_endop(9);
							case 2: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = stack_data(UInt16, arg_qword(6)); movpc_endop(15);
							case 3: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = stack_data_d(UInt16, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 4: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = static_data(UInt16, arg_qword(6)); movpc_endop(15);
							case 5: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = static_data_d(UInt16, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 6: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = register_data(u16, arg_byte(6)); movpc_endop(8);
							case 7: register_data_d(UInt16, arg_byte(1), arg_dword(2)) = register_data_d(UInt16, arg_byte(6), arg_dword(7)); movpc_endop(12);
						} break;
					} break;
					case 2: switch (arg_3bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register(arg_byte(1)).u32 = register(arg_byte(2)).u32; movpc_endop(4);
							case 1: register(arg_byte(1)).u32 = arg_word(2); movpc_endop(7);
							case 2: register(arg_byte(1)).u32 = stack_data(UInt32, arg_qword(2)); movpc_endop(11);
							case 3: register(arg_byte(1)).u32 = stack_data_d(UInt32, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register(arg_byte(1)).u32 = static_data(UInt32, arg_qword(2)); movpc_endop(11);
							case 5: register(arg_byte(1)).u32 = static_data_d(UInt32, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register(arg_byte(1)).u32 = register_data(u32, arg_byte(2)); movpc_endop(4);
							case 7: register(arg_byte(1)).u32 = register_data_d(UInt32, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 1: movpc_endop(1);
						case 2: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data(UInt32, arg_qword(1)) = register(arg_byte(9)).u32; movpc_endop(11);
							case 1: stack_data(UInt32, arg_qword(1)) = arg_word(9); movpc_endop(14);
							case 2: stack_data(UInt32, arg_qword(1)) = stack_data(UInt32, arg_qword(9)); movpc_endop(18);
							case 3: stack_data(UInt32, arg_qword(1)) = stack_data_d(UInt32, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: stack_data(UInt32, arg_qword(1)) = static_data(UInt32, arg_qword(9)); movpc_endop(18);
							case 5: stack_data(UInt32, arg_qword(1)) = static_data_d(UInt32, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: stack_data(UInt32, arg_qword(1)) = register_data(u32, arg_byte(9)); movpc_endop(11);
							case 7: stack_data(UInt32, arg_qword(1)) = register_data_d(UInt32, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 3: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u32; movpc_endop(15);
							case 1: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(18);
							case 2: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = stack_data(UInt32, arg_qword(13)); movpc_endop(22);
							case 3: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = stack_data_d(UInt32, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 4: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = static_data(UInt32, arg_qword(13)); movpc_endop(22);
							case 5: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = static_data_d(UInt32, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 6: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = register_data(u32, arg_byte(13)); movpc_endop(15);
							case 7: stack_data_d(UInt32, arg_qword(1), arg_dword(9)) = register_data_d(UInt32, arg_byte(13), arg_dword(14)); movpc_endop(19);
						} break;
						case 4: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data(UInt32, arg_qword(1)) = register(arg_byte(9)).u32; movpc_endop(11);
							case 1: static_data(UInt32, arg_qword(1)) = arg_word(9); movpc_endop(14);
							case 2: static_data(UInt32, arg_qword(1)) = stack_data(UInt32, arg_qword(9)); movpc_endop(18);
							case 3: static_data(UInt32, arg_qword(1)) = stack_data_d(UInt32, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: static_data(UInt32, arg_qword(1)) = static_data(UInt32, arg_qword(9)); movpc_endop(18);
							case 5: static_data(UInt32, arg_qword(1)) = static_data_d(UInt32, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: static_data(UInt32, arg_qword(1)) = register_data(u32, arg_byte(9)); movpc_endop(11);
							case 7: static_data(UInt32, arg_qword(1)) = register_data_d(UInt32, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 5: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u32; movpc_endop(15);
							case 1: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(18);
							case 2: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = stack_data(UInt32, arg_qword(13)); movpc_endop(22);
							case 3: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = stack_data_d(UInt32, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 4: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = static_data(UInt32, arg_qword(13)); movpc_endop(22);
							case 5: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = static_data_d(UInt32, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 6: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = register_data(u32, arg_byte(13)); movpc_endop(15);
							case 7: static_data_d(UInt32, arg_qword(1), arg_dword(9)) = register_data_d(UInt32, arg_byte(13), arg_dword(10)); movpc_endop(19);
						} break;
						case 6: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data(u32, arg_byte(1)) = register(arg_byte(2)).u32; movpc_endop(4);
							case 1: register_data(u32, arg_byte(1)) = arg_word(2); movpc_endop(7);
							case 2: register_data(u32, arg_byte(1)) = stack_data(UInt32, arg_qword(2)); movpc_endop(11);
							case 3: register_data(u32, arg_byte(1)) = stack_data_d(UInt32, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register_data(u32, arg_byte(1)) = static_data(UInt32, arg_qword(2)); movpc_endop(11);
							case 5: register_data(u32, arg_byte(1)) = static_data_d(UInt32, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register_data(u32, arg_byte(1)) = register_data(u32, arg_byte(2)); movpc_endop(4);
							case 7: register_data(u32, arg_byte(1)) = register_data_d(UInt32, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 7: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = register(arg_byte(6)).u32; movpc_endop(8);
							case 1: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = arg_word(6); movpc_endop(11);
							case 2: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = stack_data(UInt32, arg_qword(6)); movpc_endop(15);
							case 3: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = stack_data_d(UInt32, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 4: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = static_data(UInt32, arg_qword(6)); movpc_endop(15);
							case 5: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = static_data_d(UInt32, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 6: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = register_data(u32, arg_byte(6)); movpc_endop(8);
							case 7: register_data_d(UInt32, arg_byte(1), arg_dword(2)) = register_data_d(UInt32, arg_byte(6), arg_dword(7)); movpc_endop(12);
						} break;
					} break;
					case 3: switch (arg_3bits(0, 2)) /* dest mode */
					{
						case 0: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register(arg_byte(1)).u64 = register(arg_byte(2)).u64; movpc_endop(4);
							case 1: register(arg_byte(1)).u64 = arg_word(2); movpc_endop(11);
							case 2: register(arg_byte(1)).u64 = stack_data(UInt64, arg_qword(2)); movpc_endop(11);
							case 3: register(arg_byte(1)).u64 = stack_data_d(UInt64, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register(arg_byte(1)).u64 = static_data(UInt64, arg_qword(2)); movpc_endop(11);
							case 5: register(arg_byte(1)).u64 = static_data_d(UInt64, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register(arg_byte(1)).u64 = register_data(u64, arg_byte(2)); movpc_endop(4);
							case 7: register(arg_byte(1)).u64 = register_data_d(UInt64, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 1: movpc_endop(1);
						case 2: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data(UInt64, arg_qword(1)) = register(arg_byte(9)).u64; movpc_endop(11);
							case 1: stack_data(UInt64, arg_qword(1)) = arg_word(9); movpc_endop(18);
							case 2: stack_data(UInt64, arg_qword(1)) = stack_data(UInt64, arg_qword(9)); movpc_endop(18);
							case 3: stack_data(UInt64, arg_qword(1)) = stack_data_d(UInt64, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: stack_data(UInt64, arg_qword(1)) = static_data(UInt64, arg_qword(9)); movpc_endop(18);
							case 5: stack_data(UInt64, arg_qword(1)) = static_data_d(UInt64, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: stack_data(UInt64, arg_qword(1)) = register_data(u64, arg_byte(9)); movpc_endop(11);
							case 7: stack_data(UInt64, arg_qword(1)) = register_data_d(UInt64, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 3: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u64; movpc_endop(15);
							case 1: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(22);
							case 2: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = stack_data(UInt64, arg_qword(13)); movpc_endop(22);
							case 3: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = stack_data_d(UInt64, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 4: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = static_data(UInt64, arg_qword(13)); movpc_endop(22);
							case 5: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = static_data_d(UInt64, arg_qword(13), arg_dword(21)); movpc_endop(26);
							case 6: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = register_data(u64, arg_byte(13)); movpc_endop(15);
							case 7: stack_data_d(UInt64, arg_qword(1), arg_dword(9)) = register_data_d(UInt64, arg_byte(13), arg_dword(14)); movpc_endop(19);
						} break;
						case 4: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data(UInt64, arg_qword(1)) = register(arg_byte(9)).u64; movpc_endop(11);
							case 1: static_data(UInt64, arg_qword(1)) = arg_word(9); movpc_endop(18);
							case 2: static_data(UInt64, arg_qword(1)) = stack_data(UInt64, arg_qword(9)); movpc_endop(18);
							case 3: static_data(UInt64, arg_qword(1)) = stack_data_d(UInt64, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 4: static_data(UInt64, arg_qword(1)) = static_data(UInt64, arg_qword(9)); movpc_endop(18);
							case 5: static_data(UInt64, arg_qword(1)) = static_data_d(UInt64, arg_qword(9), arg_dword(17)); movpc_endop(22);
							case 6: static_data(UInt64, arg_qword(1)) = register_data(u64, arg_byte(9)); movpc_endop(11);
							case 7: static_data(UInt64, arg_qword(1)) = register_data_d(UInt64, arg_byte(9), arg_dword(10)); movpc_endop(15);
						} break;
						case 5: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = register(arg_byte(13)).u64; movpc_endop(15);
							case 1: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = arg_word(13); movpc_endop(22);
							case 2: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = stack_data(UInt64, arg_qword(13)); movpc_endop(22);
							case 3: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = stack_data_d(UInt64, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 4: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = static_data(UInt64, arg_qword(13)); movpc_endop(22);
							case 5: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = static_data_d(UInt64, arg_qword(13), arg_dword(17)); movpc_endop(26);
							case 6: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = register_data(u64, arg_byte(13)); movpc_endop(15);
							case 7: static_data_d(UInt64, arg_qword(1), arg_dword(9)) = register_data_d(UInt64, arg_byte(13), arg_dword(10)); movpc_endop(19);
						} break;
						case 6: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data(u64, arg_byte(1)) = register(arg_byte(2)).u64; movpc_endop(4);
							case 1: register_data(u64, arg_byte(1)) = arg_word(2); movpc_endop(11);
							case 2: register_data(u64, arg_byte(1)) = stack_data(UInt64, arg_qword(2)); movpc_endop(11);
							case 3: register_data(u64, arg_byte(1)) = stack_data_d(UInt64, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 4: register_data(u64, arg_byte(1)) = static_data(UInt64, arg_qword(2)); movpc_endop(11);
							case 5: register_data(u64, arg_byte(1)) = static_data_d(UInt64, arg_qword(2), arg_dword(10)); movpc_endop(15);
							case 6: register_data(u64, arg_byte(1)) = register_data(u64, arg_byte(2)); movpc_endop(4);
							case 7: register_data(u64, arg_byte(1)) = register_data_d(UInt64, arg_byte(2), arg_dword(3)); movpc_endop(8);
						} break;
						case 7: switch (arg_3bits(0, 5)) /* src mode */
						{
							case 0: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = register(arg_byte(6)).u64; movpc_endop(8);
							case 1: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = arg_word(6); movpc_endop(15);
							case 2: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = stack_data(UInt64, arg_qword(6)); movpc_endop(15);
							case 3: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = stack_data_d(UInt64, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 4: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = static_data(UInt64, arg_qword(6)); movpc_endop(15);
							case 5: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = static_data_d(UInt64, arg_qword(6), arg_dword(14)); movpc_endop(19);
							case 6: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = register_data(u64, arg_byte(6)); movpc_endop(8);
							case 7: register_data_d(UInt64, arg_byte(1), arg_dword(2)) = register_data_d(UInt64, arg_byte(6), arg_dword(7)); movpc_endop(12);
						} break;
					} break;
				}
			end_opcode();
		}
	}
}

