#include "runtime.h"

#include "vm.h"

using namespace kram::bin;
using kram::op::Opcode;

#define SELF_CHUNK static_cast<ChunkOffset>(-1)

namespace kram::runtime
{
	RuntimeState::RuntimeState(Stack* stack, Heap* heap) :
		regs{},
		stack{ stack },
		heap{ heap },
		exit{ false },
		error{ ErrorCode::OK }
	{}

	static forceinline bool need_resize_stack(RuntimeState* state)
	{
		Stack& stack = *state->stack;
		return (stack.roof - (stack.base + state->regs.st.stack_offset)) < ((stack.roof - stack.base) / 2);
	}

	static void init_runtime(RuntimeState* state, Chunk* chunk, FunctionOffset functionOffset)
	{
		Function* function = chunk->functions + functionOffset;

		state->regs.sb.stack_offset = 0;
		state->regs.sp.stack_offset = state->regs.sb.stack_offset + function->stackCount + sizeof(Registers);
		state->regs.st.stack_offset = state->regs.sp.stack_offset + function->parameterCount;
		state->regs.ch.addr_chunk = chunk;
		state->regs.ip.addr_bytes = chunk->code + function->codeOffset;
		state->regs.sd.addr_bytes = chunk->statics;

		if (need_resize_stack(state))
			_resize_stack(state->stack, state->regs.st.stack_offset - state->regs.sb.stack_offset);
	}

	static void call_chunk(RuntimeState* state, ChunkOffset chunkOffset, FunctionOffset functionOffset)
	{
		Chunk* chunk = chunkOffset == SELF_CHUNK ? state->regs.ch.addr_chunk : state->regs.ch.addr_chunk->connections + chunkOffset;
		Function* function = chunk->functions + functionOffset;

		Registers* oldregs = rcast(Registers*, state->stack->base + state->regs.sp.stack_offset) - 1;
		*oldregs = state->regs;

		state->regs.sb = oldregs->sp;
		state->regs.sp.stack_offset = state->regs.sb.stack_offset + function->stackCount + sizeof(Registers);
		state->regs.st.stack_offset = state->regs.sp.stack_offset + function->parameterCount;
		state->regs.ch.addr_chunk = chunk;
		state->regs.ip.addr_bytes = chunk->code + function->codeOffset;
		state->regs.sd.addr_bytes = chunk->statics;

		if (need_resize_stack(state))
			_resize_stack(state->stack, state->regs.st.stack_offset - state->regs.sb.stack_offset);
	}

	static void finish_call(RuntimeState* state)
	{
		if (state->regs.sb.stack_offset == 0)
			state->exit = true;

		Registers* oldregs = rcast(Registers*, state->stack->base + state->regs.sb.stack_offset) - 1;
		oldregs->sr = state->regs.sr;
		state->regs = *oldregs;
		state->exit = false;
	}

	void _build_stack(Stack* stack, Size size)
	{
		stack->base = _kram_malloc(StackUnit, size);
		stack->roof = stack->base + size;
	}
	void _resize_stack(Stack* stack, Size min)
	{
		Size size = static_cast<Size>(stack->roof - stack->base);
		Size newsize = size * 2 + (size < min ? min - size : 0);
		StackUnit* old = stack->base;

		stack->base = _kram_malloc(StackUnit, newsize);
		stack->roof = stack->base + newsize;

		std::memcpy(stack->base, old, size);

		_kram_free(old);
	}
	void _destroy_stack(Stack* stack)
	{
		_kram_free(stack->base);
		std::memset(stack, 0, sizeof(*stack));
	}
}




#define do_opcode(_Inst) case _Inst : {
#define end_opcode() } goto instruction_begin
#define move_ip(_Amount) (state.regs.ip.addr_bytes += (_Amount))

namespace kram::runtime
{
	namespace ru
	{
		template<std::integral _Ty>
		forceinline _Ty pop_arg(RuntimeState& state)
		{
			if constexpr (sizeof(_Ty) == 1)
			{
				if constexpr (std::signed_integral<_Ty>)
					return *(state.regs.ip.addr_s8++);
				else return *(state.regs.ip.addr_u8++);
			}
			else if constexpr (sizeof(_Ty) == 2)
			{
				if constexpr (std::signed_integral<_Ty>)
					return *(state.regs.ip.addr_s16++);
				else return *(state.regs.ip.addr_u16++);
			}
			else if constexpr (sizeof(_Ty) == 4)
			{
				if constexpr (std::signed_integral<_Ty>)
					return *(state.regs.ip.addr_s32++);
				else return *(state.regs.ip.addr_u32++);
			}
			else
			{
				if constexpr (std::signed_integral<_Ty>)
					return *(state.regs.ip.addr_s64++);
				else return *(state.regs.ip.addr_u64++);
			}
		}

		template<std::integral _Ty, unsigned int _ArgIdx>
		forceinline _Ty get_arg(RuntimeState& state)
		{
			if constexpr (sizeof(_Ty) == 1)
			{
				if constexpr (std::signed_integral<_Ty>)
					return *(state.regs.ip.addr_s8 + _ArgIdx);
				else return *(state.regs.ip.addr_u8 + _ArgIdx);
			}
			else
			{
				return *reinterpret_cast<_Ty*>(state.regs.ip.addr_bytes + _ArgIdx);
			}
		}

		template<unsigned int _BitIdx, unsigned int _BitCount>
		forceinline UInt8 bits(UInt8 value)
		{
			return static_cast<UInt8>((value >> _BitIdx) & ((0x1 << _BitCount) - 1));
		}

		template<unsigned int _BitIdx>
		forceinline bool test(UInt8 value)
		{
			return static_cast<bool>((value >> _BitIdx) & 0x1);
		}


		template<unsigned int _BitIdx, unsigned int _BitCount>
		forceinline UInt8 pop_arg_bits(RuntimeState& state)
		{
			return bits<_BitIdx, _BitCount>(pop_arg<UInt8>(state));
		}

		template<unsigned int _ArgIdx, unsigned int _BitIdx, unsigned int _BitCount>
		forceinline UInt8 get_arg_bits(RuntimeState& state)
		{
			return bits<_BitIdx, _BitCount>(get_arg<UInt8, _ArgIdx>(state));
		}

		template<typename _Ty>
		forceinline _Ty& reg(RuntimeState& state, unsigned int index)
		{
			if constexpr (std::integral<_Ty>)
			{
				if constexpr (sizeof(_Ty) == 1)
				{
					if constexpr (std::signed_integral<_Ty>)
						return state.regs.by_index[index].s8;
					else return state.regs.by_index[index].u8;
				}
				else if constexpr (sizeof(_Ty) == 2)
				{
					if constexpr (std::signed_integral<_Ty>)
						return state.regs.by_index[index].s16;
					else return state.regs.by_index[index].u16;
				}
				else if constexpr (sizeof(_Ty) == 4)
				{
					if constexpr (std::signed_integral<_Ty>)
						return state.regs.by_index[index].s32;
					else return state.regs.by_index[index].u32;
				}
				else
				{
					if constexpr (std::signed_integral<_Ty>)
						return state.regs.by_index[index].s64;
					else return state.regs.by_index[index].u64;
				}
			}
			else if constexpr (std::floating_point<_Ty>)
			{
				if constexpr (std::same_as<_Ty, float>)
					return state.regs.by_index[index].f32;
				else state.regs.by_index[index].f64;
			}
			else
			{
				return *rcast(_Ty*, rcast(void*, &state.regs.by_index[index]._value));
			}
		}

		template<typename _Ty, bool _FromStack>
		forceinline _Ty& from_mem(RuntimeState& state, std::uintptr_t offset)
		{
			if constexpr (_FromStack)
			{
				return *rcast(_Ty*, (state.stack->base + state.regs.sb.stack_offset + offset));
			}
			else
			{
				return *rcast(_Ty*, (state.regs.sd.addr_stack_offset + offset));
			}
		}

		template<typename _Ty>
		forceinline _Ty& from_mem(RuntimeState& state, std::uintptr_t offset, bool from_stack)
		{
			return from_stack
				? *rcast(_Ty*, (state.stack->base + state.regs.sb.stack_offset + offset))
				: *rcast(_Ty*, (state.regs.sd.addr_stack_offset + offset));
		}

		template<typename _SizeType>
		forceinline _SizeType& pop_memloc(RuntimeState& state)
		{
			UInt8 pars = pop_arg<UInt8>(state);
			UInt8 regs = pop_arg<UInt8>(state);
			std::uintptr_t addr = 0;

			if (test<2>(pars))
			{
				switch (bits<3, 2>(pars))
				{
					case 0: addr += state.regs.by_index[bits<4, 4>(regs)].u64; break;
					case 1: addr += state.regs.by_index[bits<4, 4>(regs)].u64 * 2; break;
					case 2: addr += state.regs.by_index[bits<4, 4>(regs)].u64 * 4; break;
					case 3: addr += state.regs.by_index[bits<4, 4>(regs)].u64 * 8; break;
				}
			}
			if (test<5>(pars))
			{
				switch (bits<6, 2>(pars))
				{
					case 0: addr += pop_arg<UInt8>(state); break;
					case 1: addr += pop_arg<UInt16>(state); break;
					case 2: addr += pop_arg<UInt32>(state); break;
					case 3: addr += pop_arg<UInt64>(state); break;
				}
			}
			switch (bits<0, 2>(pars))
			{
				case 0: return *rcast(_SizeType*, (addr != 0 ? addr : scast(std::uintptr_t, -1)));
				case 1: return from_mem<_SizeType, true>(state, addr);
				case 2: return from_mem<_SizeType, false>(state, addr);
				case 3: return *rcast(_SizeType*, (state.regs.by_index[bits<0, 4>(regs)].addr_stack_offset + addr));
			}

			return *rcast(_SizeType*, scast(std::uintptr_t, -1));
		}

		template<typename _DestType, typename _SrcType>
		forceinline void raw_cast_to(void* dst, _SrcType value)
		{
			*rcast(_DestType*, dst) = scast(_DestType, value);
		}

		template<typename _FromType>
		forceinline void cast_to(UInt8 type, void* dst, _FromType value)
		{
			switch (type)
			{
				case 0: raw_cast_to<UInt8>(dst, value); return;
				case 1: raw_cast_to<UInt16>(dst, value); return;
				case 2: raw_cast_to<UInt32>(dst, value); return;
				case 3: raw_cast_to<UInt64>(dst, value); return;
				case 4: raw_cast_to<Int8>(dst, value); return;
				case 5: raw_cast_to<Int16>(dst, value); return;
				case 6: raw_cast_to<Int32>(dst, value); return;
				case 7: raw_cast_to<Int64>(dst, value); return;
				case 8: raw_cast_to<float>(dst, value); return;
				case 9: raw_cast_to<double>(dst, value); return;
			}
		}

		forceinline void cast_from_to(UInt8 dst_type, void* dst, UInt8 src_type, void* src)
		{
			switch (src_type)
			{
				case 0: cast_to(dst_type, dst, *rcast(UInt8*, src)); return;
				case 1: cast_to(dst_type, dst, *rcast(UInt16*, src)); return;
				case 2: cast_to(dst_type, dst, *rcast(UInt32*, src)); return;
				case 3: cast_to(dst_type, dst, *rcast(UInt64*, src)); return;
				case 4: cast_to(dst_type, dst, *rcast(Int8*, src)); return;
				case 5: cast_to(dst_type, dst, *rcast(Int16*, src)); return;
				case 6: cast_to(dst_type, dst, *rcast(Int32*, src)); return;
				case 7: cast_to(dst_type, dst, *rcast(Int64*, src)); return;
				case 8: cast_to(dst_type, dst, *rcast(float*, src)); return;
				case 9: cast_to(dst_type, dst, *rcast(double*, src)); return;
			}
		}



		template<std::unsigned_integral _SizeType>
		forceinline void mov_r_r(RuntimeState& state)
		{
			UInt8 regs = pop_arg<UInt8>(state);

			if constexpr (sizeof(_SizeType) == 1)
				state.regs.by_index[bits<0, 4>(regs)].u8 = state.regs.by_index[bits<4, 4>(regs)].u8;
			else if constexpr (sizeof(_SizeType) == 2)
				state.regs.by_index[bits<0, 4>(regs)].u16 = state.regs.by_index[bits<4, 4>(regs)].u16;
			else if constexpr (sizeof(_SizeType) == 4)
				state.regs.by_index[bits<0, 4>(regs)].u32 = state.regs.by_index[bits<4, 4>(regs)].u32;
			else
				state.regs.by_index[bits<0, 4>(regs)].u64 = state.regs.by_index[bits<4, 4>(regs)].u64;
		}

		template<std::unsigned_integral _SizeType, bool _Swap>
		forceinline void mov_rm_rm(RuntimeState& state)
		{
			UInt8 regidx = pop_arg_bits<0, 4>(state);
			_SizeType& loc = pop_memloc<_SizeType>(state);

			if constexpr (_Swap)
				loc = reg<_SizeType>(state, regidx);
			else reg<_SizeType>(state, regidx) = loc;
		}

		template<std::unsigned_integral _SizeType>
		forceinline void mov_r_imm(RuntimeState& state)
		{
			UInt8 reg = pop_arg_bits<0, 4>(state);

			if constexpr (sizeof(_SizeType) == 1)
				state.regs.by_index[reg].u8 = pop_arg<UInt8>(state);
			else if constexpr (sizeof(_SizeType) == 2)
				state.regs.by_index[reg].u16 = pop_arg<UInt16>(state);
			else if constexpr (sizeof(_SizeType) == 4)
				state.regs.by_index[reg].u32 = pop_arg<UInt32>(state);
			else
				state.regs.by_index[reg].u64 = pop_arg<UInt64>(state);
		}

		template<std::unsigned_integral _SizeType>
		forceinline void mov_m_imm(RuntimeState& state)
		{
			_SizeType imm = pop_arg<_SizeType>(state);
			pop_memloc<_SizeType>(state) = imm;
		}

		forceinline void lea(RuntimeState& state)
		{
			UInt8 reg = pop_arg_bits<0, 4>(state);
			state.regs.by_index[reg].addr = &pop_memloc<void*>(state);
		}

		template<std::unsigned_integral _SizeType>
		forceinline void mmb(RuntimeState& state)
		{
			UInt8 regs = pop_arg<UInt8>(state);
			Size size = static_cast<Size>(pop_arg<_SizeType>(state));
			std::memcpy(state.regs.by_index[bits<0, 4>(regs)].addr, state.regs.by_index[bits<4, 4>(regs)].addr, size);
		}

		forceinline void new_r_s(RuntimeState& state)
		{
			UInt8 pars = pop_arg<UInt8>(state);
			Size size;
			
			switch (bits<4, 2>(pars))
			{
				case 0: size = pop_arg<UInt8>(state); break;
				case 1: size = pop_arg<UInt16>(state); break;
				case 2: size = pop_arg<UInt32>(state); break;
				case 3: size = pop_arg<UInt64>(state); break;
				default: return;
			}

			state.regs.by_index[bits<0, 4>(pars)].addr = state.heap->malloc(size, test<6>(pars));
		}

		forceinline void new_m_s(RuntimeState& state)
		{
			UInt8 pars = pop_arg<UInt8>(state);
			Size size;
			switch (bits<0, 2>(pars))
			{
				case 0: size = pop_arg<UInt8>(state); break;
				case 1: size = pop_arg<UInt16>(state); break;
				case 2: size = pop_arg<UInt32>(state); break;
				case 3: size = pop_arg<UInt64>(state); break;
				default: size = 0;
			}

			pop_memloc<void*>(state) = state.heap->malloc(size, test<2>(pars));
		}

		forceinline void del_r(RuntimeState& state)
		{
			state.heap->free(state.regs.by_index[pop_arg_bits<0, 4>(state)].addr);
		}

		forceinline void del_m(RuntimeState& state)
		{
			state.heap->free(pop_memloc<void*>(state));
		}

		forceinline void mhr_r(RuntimeState& state)
		{
			UInt8 pars = pop_arg<UInt8>(state);
			if (test<4>(pars))
				state.heap->increase_ref(state.regs.by_index[bits<0, 4>(pars)].addr);
			else state.heap->decrease_ref(state.regs.by_index[bits<0, 4>(pars)].addr);
		}

		forceinline void mhr_m(RuntimeState& state)
		{
			if (pop_arg_bits<0, 1>(state))
				state.heap->increase_ref(pop_memloc<void*>(state));
			else state.heap->decrease_ref(pop_memloc<void*>(state));
		}

		forceinline void cst_r(RuntimeState& state)
		{
			void* reg = &state.regs.by_index[pop_arg_bits<0, 4>(state)]._value;
			UInt8 types = pop_arg<UInt8>(state);

			cast_from_to(bits<0, 4>(types), reg, bits<4, 4>(types), reg);
		}

		forceinline void cst_m(RuntimeState& state)
		{
			UInt8 types = pop_arg<UInt8>(state);
			void* ptr = pop_memloc<void*>(state);

			cast_from_to(bits<0, 4>(types), ptr, bits<4, 4>(types), ptr);
		}
	}

	void execute(KramState* kstate, bin::Chunk* chunk, FunctionOffset function)
	{
		RuntimeState state{ &kstate->_rstack, kstate };
		init_runtime(&state, chunk, function);

		int x = ru::pop_arg<char>(state);

	instruction_begin:
		switch (*(state.regs.ip.addr_opcode++))
		{
			do_opcode(op::Opcode::NOP)
			end_opcode();


			do_opcode(op::Opcode::MOV_r8_r8)
				ru::mov_r_r<UInt8>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r16_r16)
				ru::mov_r_r<UInt16>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r32_r32)
				ru::mov_r_r<UInt32>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r64_r64)
				ru::mov_r_r<UInt64>(state);
			end_opcode();


			do_opcode(op::Opcode::MOV_r8_m8)
				ru::mov_rm_rm<UInt8, false>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r16_m16)
				ru::mov_rm_rm<UInt16, false>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r32_m32)
				ru::mov_rm_rm<UInt32, false>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r64_m64)
				ru::mov_rm_rm<UInt64, false>(state);
			end_opcode();


			do_opcode(op::Opcode::MOV_m8_r8)
				ru::mov_rm_rm<UInt8, true>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m16_r16)
				ru::mov_rm_rm<UInt16, true>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m32_r32)
				ru::mov_rm_rm<UInt32, true>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m64_r64)
				ru::mov_rm_rm<UInt64, true>(state);
			end_opcode();


			do_opcode(op::Opcode::MOV_r8_imm8)
				ru::mov_r_imm<UInt8>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r16_imm16)
				ru::mov_r_imm<UInt16>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r32_imm32)
				ru::mov_r_imm<UInt32>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_r64_imm64)
				ru::mov_r_imm<UInt64>(state);
			end_opcode();


			do_opcode(op::Opcode::MOV_m8_imm8)
				ru::mov_m_imm<UInt8>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m16_imm16)
				ru::mov_m_imm<UInt16>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m32_imm32)
				ru::mov_m_imm<UInt32>(state);
			end_opcode();

			do_opcode(op::Opcode::MOV_m64_imm64)
				ru::mov_m_imm<UInt64>(state);
			end_opcode();


			do_opcode(op::Opcode::LEA)
				ru::lea(state);
			end_opcode();


			do_opcode(op::Opcode::MMB_sb)
				ru::mmb<UInt8>(state);
			end_opcode();

			do_opcode(op::Opcode::MMB_sw)
				ru::mmb<UInt16>(state);
			end_opcode();

			do_opcode(op::Opcode::MMB_sd)
				ru::mmb<UInt32>(state);
			end_opcode();

			do_opcode(op::Opcode::MMB_sq)
				ru::mmb<UInt64>(state);
			end_opcode();


			do_opcode(op::Opcode::NEW_r_s)
				ru::new_r_s(state);
			end_opcode();


			do_opcode(op::Opcode::NEW_m_s)
				ru::new_m_s(state);
			end_opcode();


			do_opcode(op::Opcode::DEL_r)
				ru::del_r(state);
			end_opcode();


			do_opcode(op::Opcode::DEL_m)
				ru::del_m(state);
			end_opcode();


			do_opcode(op::Opcode::MHR_r)
				ru::mhr_r(state);
			end_opcode();


			do_opcode(op::Opcode::MHR_m)
				ru::mhr_m(state);
			end_opcode();


			do_opcode(op::Opcode::CST_r)
				ru::cst_r(state);
			end_opcode();


			do_opcode(op::Opcode::CST_m)
				ru::cst_m(state);
			end_opcode();
		}
	}
}



