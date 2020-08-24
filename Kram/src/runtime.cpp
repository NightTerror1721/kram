#include "runtime.h"

#include "vm.h"

using namespace kram::bin;
using kram::op::Opcode;

#define SELF_CHUNK static_cast<ChunkOffset>(-1)

namespace kram::runtime
{
	RuntimeState::RuntimeState(Stack* stack, Heap* heap) :
		stack{ stack },
		heap{ heap },
		chunk{ nullptr },
		inst{ nullptr },
		exit{ false },
		error{ ErrorCode::OK }
	{}

	static forceinline bool need_resize_stack(Stack* stack, Size extra = 0) { return (stack->roof - stack->top + static_cast<Int64>(extra)) < ((stack->roof - stack->base) / 2); }

	static void call_chunk(RuntimeState* state, ChunkOffset chunkOffset, FunctionOffset functionOffset, RegisterOffset retRegOffset)
	{
		Stack* stack = state->stack;
		Chunk* chunk = chunkOffset == SELF_CHUNK ? state->chunk : state->chunk->childChunks + chunkOffset;
		Function* function = chunk->functions + functionOffset;
		CallInfo* info = rcast(CallInfo*, stack->top);

		info->inst = state->inst;
		info->chunk = state->chunk;

		info->top = stack->top - stack->base;
		info->regs = rcast(StackUnit*, stack->regs) - stack->base;
		info->data = stack->data - stack->base;

		info->returnRegisterOffset = retRegOffset;

		if (need_resize_stack(stack, function->stackSize))
			_resize_stack(stack, function->stackSize);

		stack->data = rcast(StackUnit*, info + 1);
		stack->regs = rcast(Register*, stack->regs + function->dataCount);
		stack->top = rcast(StackUnit*, stack->data + function->registerCount);

		state->chunk = chunk;
		state->inst = rcast(Opcode*, function->code);
	}

	static void finish_call(RuntimeState* state, RegisterOffset retRegOffset)
	{
		Stack* stack = state->stack;
		CallInfo* info = stack->cinfo;
		auto rr = stack->regs[retRegOffset].reg;
		
		stack->top = stack->base + info->top;
		stack->regs = rcast(Register*, stack->base + info->regs);
		stack->data = stack->base + info->data;
		stack->cinfo = rcast(CallInfo*, stack->data) - 1;

		state->chunk = info->chunk;
		state->inst = info->inst;

		if(!(state->exit = !state->inst))
			stack->regs[info->returnRegisterOffset].reg = rr;
	}

	void _build_stack(Stack* stack, Size size)
	{
		stack->base = new StackUnit[size];
		stack->roof = stack->base + size;

		stack->top = stack->data = stack->base;
		stack->regs = rcast(Register*, stack->base);
		stack->cinfo = rcast(CallInfo*, stack->base);
	}
	void _resize_stack(Stack* stack, Size extra)
	{
		Size size = static_cast<Size>(stack->roof - stack->base);

		StackUnit* old = stack->base;
		std::ptrdiff_t top = stack->top - old;
		std::ptrdiff_t regs = rcast(StackUnit*, stack->regs) - old;
		std::ptrdiff_t data = stack->data - old;
		std::ptrdiff_t cinfo = rcast(StackUnit*, stack->cinfo) - old;

		stack->base = new StackUnit[size * 2 + extra];
		stack->roof = stack->base + (size * 2 + extra);

		std::memcpy(stack->base, old, size);

		stack->top = stack->base + top;
		stack->regs = rcast(Register*, stack->base + regs);
		stack->data = stack->base + data;
		stack->cinfo = rcast(CallInfo*, stack->base + cinfo);

		delete old;
	}
	void _destroy_stack(Stack* stack)
	{
		delete stack->base;
		std::memset(stack, 0, sizeof(*stack));
	}
}

#define move_pc(_Amount) (state.inst += (_Amount))
#define opcode(_Inst) case _Inst : {
#define end_opcode() } goto instruction_begin
#define break_endop() goto instruction_begin
#define movpc_endop(_Amount) move_pc(_Amount); goto instruction_begin

#define register(_Idx) state.stack->regs[_Idx]

#define register_data_a(_Part, _Idx) register(_Idx).CONCAT_MACROS(addr_, _Part)
#define register_data_ad(_Type, _Idx, _Delta) rcast(_Type*, register(_Idx).addr_u8 + (_Delta))
#define register_data(_Part, _Idx) *register_data_a(_Part, _Idx)
#define register_data_d(_Type, _Idx, _Delta) *register_data_ad(_Type, _Idx, _Delta)

#define stack_data_a(_Type, _Idx) rcast(_Type*, state.stack->data + (_Idx))
#define stack_data_ad(_Type, _Idx, _Delta) rcast(_Type*, state.stack->data + ((_Idx) + (_Delta)))
#define stack_data(_Type, _Idx) *stack_data_a(_Type, _Idx)
#define stack_data_d(_Type, _Idx, _Delta) *stack_data_ad(_Type, _Idx, _Delta)

#define static_data_a(_Type, _Idx) rcast(_Type*, chunk->statics[_Idx].data)
#define static_data_ad(_Type, _Idx, _Delta) rcast(_Type*, rcast(UInt8*, chunk->statics[_Idx].data) + (_Delta))
#define static_data(_Type, _Idx) *static_data_a(_Type, _Idx)
#define static_data_d(_Type, _Idx, _Delta) *static_data_ad(_Type, _Idx, _Delta)

#define stack_arg_data(_Type, _Idx) *rcast(_Type*, state.stack->top + (sizeof(CallInfo) + (_Idx)))

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


#define mcpy(_Dst, _Src, _Size) std::memcpy((_Dst), (_Src), static_cast<kram::Size>(_Size))

#define op_cast_type(_Reg, _RegPart) \
	switch (arg_4bits(0, 4)) { \
		case 0: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->u8); movpc_endop(3); \
		case 1: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->u16); movpc_endop(3); \
		case 2: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->u32); movpc_endop(3); \
		case 3: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->u64); movpc_endop(3); \
		case 4: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->s8); movpc_endop(3); \
		case 5: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->s16); movpc_endop(3); \
		case 6: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->s32); movpc_endop(3); \
		case 7: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->s64); movpc_endop(3); \
		case 8: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->f32); movpc_endop(3); \
		case 9: reg->_RegPart = static_cast<decltype(reg->_RegPart)>(reg->f64); movpc_endop(3); \
	} break

#define op_binary(_Op, _DstRegId, _SrcRegId) \
	switch (arg_4bits(0, 0)) { \
		case 0: register(_DstRegId).u8 _Op ## = register(_SrcRegId).u8; movpc_endop(3); \
		case 1: register(_DstRegId).u16 _Op ## = register(_SrcRegId).u16; movpc_endop(3); \
		case 2: register(_DstRegId).u32 _Op ## = register(_SrcRegId).u32; movpc_endop(3); \
		case 3: register(_DstRegId).u64 _Op ## = register(_SrcRegId).u64; movpc_endop(3); \
		case 4: register(_DstRegId).s8 _Op ## = register(_SrcRegId).s8; movpc_endop(3); \
		case 5: register(_DstRegId).s16 _Op ## = register(_SrcRegId).s16; movpc_endop(3); \
		case 6: register(_DstRegId).s32 _Op ## = register(_SrcRegId).s32; movpc_endop(3); \
		case 7: register(_DstRegId).s64 _Op ## = register(_SrcRegId).s64; movpc_endop(3); \
		case 8: register(_DstRegId).f32 _Op ## = register(_SrcRegId).f32; movpc_endop(3); \
		case 9: register(_DstRegId).f64 _Op ## = register(_SrcRegId).f64; movpc_endop(3); \
	} break


namespace kram::runtime::support
{
	template<typename _Ty>
	forceinline _Ty* reg(RuntimeState& state, UInt8 idx) { return rcast(_Ty*, &state.stack->regs[idx].reg); }

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* a_data(RuntimeState& state, _ArgType idx) { return rcast(_Ty*, state.stack->data + idx); }

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* a_data(RuntimeState& state, _ArgType idx, _ArgType delta)
	{
		return rcast(_Ty*, state.stack->data + (scast(Size, idx) + delta));
	}

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* s_data(RuntimeState& state, _ArgType idx) { return rcast(_Ty*, state.chunk->statics[idx].data); }

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* s_data(RuntimeState& state, _ArgType idx, _ArgType delta)
	{
		return rcast(_Ty*, rcast(char*, state.chunk->statics[idx].data) + delta);
	}

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* r_data(RuntimeState& state, UInt8 idx) { return rcast(_Ty*, state.stack->regs[idx].addr); }

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* r_data(RuntimeState& state, UInt8 idx, _ArgType delta)
	{
		return rcast(_Ty*, rcast(char*, state.stack->regs[idx].addr) + delta);
	}

	template<typename _Ty, typename _ArgType>
	forceinline _Ty* aa_data(RuntimeState& state, _ArgType idx) { return rcast(_Ty*, state.stack->top + sizeof(CallInfo) + idx); }

	template<typename _Ty, unsigned int _Idx>
	forceinline _Ty arg(RuntimeState& state) { return *rcast(_Ty*, state.inst + _Idx); }

	template<typename _Ty, unsigned int _Idx>
	forceinline _Ty& ref_arg(RuntimeState& state) { return *rcast(_Ty*, state.inst + _Idx); }

	template<typename _Ty, unsigned int _Idx, typename _ShiftType, unsigned int _ShiftAmount>
	forceinline _Ty sarg(RuntimeState& state) { return *rcast(_Ty*, state.inst + _Idx + (sizeof(_ShiftType) * _ShiftAmount - _ShiftAmount)); }

	template<typename _ArgMode, typename _DataSize, unsigned int _ArgOffset, bool _ValidImm>
	forceinline int mov_access(RuntimeState& state, UInt8 mode, _DataSize** out)
	{
		switch (mode)
		{
			case 0: return (*out = reg<_DataSize>(state, arg<UInt8, _ArgOffset>(state))), _ArgOffset + 1;
			case 1:
				if constexpr (_ValidImm)
				{
					return (*out = &ref_arg<_DataSize, _ArgOffset>(state)), _ArgOffset + sizeof(_DataSize);
				}
				else
				{
					return 0;
				}
			case 2: return (*out = a_data<_DataSize, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 3: return (*out = a_data<_DataSize, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
			case 4: return (*out = s_data<_DataSize, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 5: return (*out = s_data<_DataSize, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
			case 6: return (*out = r_data<_DataSize, _ArgMode>(state, arg<UInt8, _ArgOffset>(state))), _ArgOffset + 1;
			case 7: return (*out = r_data<_DataSize, _ArgMode>(state, arg<UInt8, _ArgOffset>(state), arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + (1 + sizeof(_ArgMode));
		}
		return 0;
	}

	template<typename _ArgMode, unsigned int _ArgOffset>
	forceinline int mmb_access(RuntimeState& state, UInt8 mode, void** out)
	{
		switch (mode)
		{
			case 0: return (*out = a_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 1: return (*out = a_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
			case 2: return (*out = s_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 3: return (*out = s_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
			case 4: return (*out = r_data<void, _ArgMode>(state, arg<UInt8, _ArgOffset>(state))), _ArgOffset + 1;
			case 5: return (*out = r_data<void, _ArgMode>(state, arg<UInt8, _ArgOffset>(state), arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + (1 + sizeof(_ArgMode));
		}
		return 0;
	}

	template<typename _ArgMode, unsigned int _ArgOffset>
	forceinline int lea_access(RuntimeState& state, UInt8 mode, void** out)
	{
		switch (mode)
		{
			case 0: return (*out = a_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 1: return (*out = a_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
			case 2: return (*out = s_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state))), _ArgOffset + sizeof(_ArgMode);
			case 3: return (*out = s_data<void, _ArgMode>(state, arg<_ArgMode, _ArgOffset>(state), sarg<_ArgMode, _ArgOffset + 1, _ArgMode, 1>(state))), _ArgOffset + (sizeof(_ArgMode) * 2);
		}
		return 0;
	}

	template<typename _ArgMode>
	forceinline void mov(RuntimeState& state)
	{

		switch (arg_2bits(0, 0))
		{
			case 0: {
				UInt8* src;
				UInt8* dst;
				move_pc((mov_access<_ArgMode, UInt8, 1U, false>(state, arg_3bits(0, 2), &dst)) + 1LL);
				move_pc((mov_access<_ArgMode, UInt8, 0U, true>(state, arg_3bits(0, 5), &src)));
				*dst = *src;
			} return;
			case 1: {
				UInt16* src;
				UInt16* dst;
				move_pc((mov_access<_ArgMode, UInt16, 1U, false>(state, arg_3bits(0, 2), &dst)) + 1LL);
				move_pc((mov_access<_ArgMode, UInt16, 0U, true>(state, arg_3bits(0, 5), &src)));
				*dst = *src;
			} return;
			case 2: {
				UInt32* src;
				UInt32* dst;
				move_pc((mov_access<_ArgMode, UInt32, 1U, false>(state, arg_3bits(0, 2), &dst)) + 1LL);
				move_pc((mov_access<_ArgMode, UInt32, 0U, true>(state, arg_3bits(0, 5), &src)));
				*dst = *src;
			} return;
			case 3: {
				UInt64* src;
				UInt64* dst;
				move_pc((mov_access<_ArgMode, UInt64, 1U, false>(state, arg_3bits(0, 2), &dst)) + 1LL);
				move_pc((mov_access<_ArgMode, UInt64, 0U, true>(state, arg_3bits(0, 5), &src)));
				*dst = *src;
			} return;
		}
	}

	template<typename _ArgMode>
	forceinline void mmb(RuntimeState& state)
	{
		void *src, *dst;
		move_pc((mmb_access<_ArgMode, sizeof(_ArgMode) + 1U>(state, arg_3bits(0, 2), &dst)) + (1LL + sizeof(_ArgMode)));
		move_pc((mmb_access<_ArgMode, 0U>(state, arg_3bits(0, 5), &src)));
		std::memcpy(dst, src, arg<_ArgMode, 1U>(state));
	}

	template<typename _ArgMode>
	forceinline void las(RuntimeState& state)
	{

		switch (arg_2bits(0, 2))
		{
			case 0: {
				UInt8* src;
				UInt8* dst;
				dst = aa_data<UInt8, _ArgMode>(state, arg<_ArgMode, 1U>(state));
				move_pc((mov_access<_ArgMode, UInt8, 1U + sizeof(_ArgMode), true>(state, arg_3bits(0, 4), &src)) + (1LL + sizeof(_ArgMode)));
				*dst = *src;
			} return;
			case 1: {
				UInt16* src;
				UInt16* dst;
				dst = aa_data<UInt16, _ArgMode>(state, arg<_ArgMode, 1U>(state));
				move_pc((mov_access<_ArgMode, UInt16, 1U + sizeof(_ArgMode), true>(state, arg_3bits(0, 4), &src)) + (1LL + sizeof(_ArgMode)));
				*dst = *src;
			} return;
			case 2: {
				UInt32* src;
				UInt32* dst;
				dst = aa_data<UInt32, _ArgMode>(state, arg<_ArgMode, 1U>(state));
				move_pc((mov_access<_ArgMode, UInt32, 1U + sizeof(_ArgMode), true>(state, arg_3bits(0, 4), &src)) + (1LL + sizeof(_ArgMode)));
				*dst = *src;
			} return;
			case 3: {
				UInt64* src;
				UInt64* dst;
				dst = aa_data<UInt64, _ArgMode>(state, arg<_ArgMode, 1U>(state));
				move_pc((mov_access<_ArgMode, UInt64, 1U + sizeof(_ArgMode), true>(state, arg_3bits(0, 4), &src)) + (1LL + sizeof(_ArgMode)));
				*dst = *src;
			} return;
		}
	}

	template<typename _ArgMode>
	forceinline void lea(RuntimeState& state)
	{
		move_pc((lea_access<_ArgMode, 2U>(state, arg_3bits(0, 4), &(state.stack->regs[arg<UInt8, 1>(state)].addr))) + 2LL);
	}

	template<typename _ArgMode>
	forceinline void new_(RuntimeState& state)
	{
		void** dst;
		bool add_reg = arg_1bit(0, 5);
		move_pc((mov_access<_ArgMode, void*, 1U, false>(state, arg_3bits(0, 2), &dst)) + 1LL);
		*dst = state.heap->malloc(arg<_ArgMode, 0U>(state), add_reg);
		move_pc(sizeof(_ArgMode));
	}

	template<typename _ArgMode>
	forceinline void del(RuntimeState& state)
	{
		void** src;
		move_pc((mov_access<_ArgMode, void*, 1U, false>(state, arg_3bits(0, 2), &src)) + 1LL);
		state.heap->free(*src);
	}

	template<typename _ArgMode>
	forceinline void mhr(RuntimeState& state)
	{
		void** target;
		bool is_inc = arg_1bit(0, 5);
		move_pc((mov_access<_ArgMode, void*, 1U, false>(state, arg_3bits(0, 2), &target)) + 1LL);
		if (is_inc)
			Heap::increase_ref(*target);
		else Heap::decrease_ref(*target);
	}
}


void kram::runtime::execute(KramState* kstate, Chunk* chunk, FunctionOffset function)
{
	RuntimeState state{ &kstate->_rstack, kstate };
	call_chunk(&state, SELF_CHUNK, function, 0);

	for (;;)
	{
		instruction_begin:
		switch (*state.inst)
		{
			opcode(Opcode::NOP)
				move_pc(1);
			end_opcode();

			opcode(Opcode::MOV)
				support::mov<UInt8>(state);
			end_opcode();

			opcode(Opcode::MOVX)
				support::mov<UInt16>(state);
			end_opcode();

			opcode(Opcode::MOVEX)
				support::mov<UInt32>(state);
			end_opcode();

			opcode(Opcode::MOVRX)
				support::mov<UInt64>(state);
			end_opcode();

			opcode(Opcode::MMB)
				switch (arg_2bits(0, 0))
				{
					case 0: support::mmb<UInt8>(state); break_endop();
					case 1: support::mmb<UInt16>(state); break_endop();
					case 2: support::mmb<UInt32>(state); break_endop();
					case 3: support::mmb<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::LAS)
				switch (arg_2bits(0, 0))
				{
					case 0: support::las<UInt8>(state); break_endop();
					case 1: support::las<UInt16>(state); break_endop();
					case 2: support::las<UInt32>(state); break_endop();
					case 3: support::las<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::LEA)
				switch (arg_2bits(0, 0))
				{
					case 0: support::lea<UInt8>(state); break_endop();
					case 1: support::lea<UInt16>(state); break_endop();
					case 2: support::lea<UInt32>(state); break_endop();
					case 3: support::lea<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::NEW)
				switch (arg_2bits(0, 0))
				{
					case 0: support::new_<UInt8>(state); break_endop();
					case 1: support::new_<UInt16>(state); break_endop();
					case 2: support::new_<UInt32>(state); break_endop();
					case 3: support::new_<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::DEL)
				switch (arg_2bits(0, 0))
				{
					case 0: support::del<UInt8>(state); break_endop();
					case 1: support::del<UInt16>(state); break_endop();
					case 2: support::del<UInt32>(state); break_endop();
					case 3: support::del<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::MHR)
				switch (arg_2bits(0, 0))
				{
					case 0: support::mhr<UInt8>(state); break_endop();
					case 1: support::mhr<UInt16>(state); break_endop();
					case 2: support::mhr<UInt32>(state); break_endop();
					case 3: support::mhr<UInt64>(state); break_endop();
				}
			end_opcode();

			opcode(Opcode::CST)
				Register* reg = &register(arg_byte(1));
				switch (arg_4bits(0, 0))
				{
					case 0: op_cast_type(reg, u8);
					case 1: op_cast_type(reg, u16);
					case 2: op_cast_type(reg, u32);
					case 3: op_cast_type(reg, u64);
					case 4: op_cast_type(reg, s8);
					case 5: op_cast_type(reg, s16);
					case 6: op_cast_type(reg, s32);
					case 7: op_cast_type(reg, s64);
					case 8: op_cast_type(reg, f32);
					case 9: op_cast_type(reg, f64);
				}
			end_opcode();

			opcode(Opcode::ADD)
				op_binary(+, arg_byte(1), arg_byte(2));
			end_opcode();
		}
	}
}

