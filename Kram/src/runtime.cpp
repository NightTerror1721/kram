#include "runtime.h"

using namespace kram::bin;
using kram::op::inst::Instruction;

namespace kram::runtime
{
	bool CallStack::push(RuntimeState* state)
	{
		RegisterStack* rstack = state->rstack;

		CallInfo* info = top + 1;
		if (info >= roof)
		{
			state->error = ErrorCode::CallStack_Overflow;
			return false;
		}

		info->inst = *state->inst;
		info->lastInst = *state->lastInst;
		info->chunk = *state->chunk;

		info->regsTop = rstack->top;
		info->regsBottom = rstack->bottom;

		info->dataTop = state->dstack->top;
		info->dataBottom = state->dstack->bottom;

		return top = info, true;
	}

	bool CallStack::pop(RuntimeState* state)
	{
		if (top < base)
		{
			state->error = ErrorCode::CallStack_Empty;
			return false;
		}

		return --top, true;
	}


	RuntimeState::RuntimeState(RegisterStack* rstack, AutodataStack* dstack, CallStack* cstack, Chunk** chunk, Instruction** inst, op::inst::Instruction** lastInst) :
		rstack{ rstack },
		dstack{ dstack },
		cstack{ cstack },
		chunk{ chunk },
		inst{ inst },
		lastInst{ lastInst },
		error{ ErrorCode::OK }
	{}

	void RuntimeState::set(CallInfo* info)
	{
		rstack->top = info->regsTop;
		rstack->bottom = info->regsBottom;

		dstack->top = info->dataTop;
		dstack->bottom = info->dataBottom;

		*chunk = info->chunk;
		*inst = info->inst;
		*lastInst = info->lastInst;
	}

	void RuntimeState::set(Chunk* newChunk, RegisterOffset firstRegister)
	{
		rstack->bottom = reinterpret_cast<StackUnit*>(rstack->registers + firstRegister);
		rstack->top = rstack->bottom + (static_cast<Size>(newChunk->parameterCount) + newChunk->variableCount);

		dstack->bottom = dstack->top;
		dstack->top += newChunk->autodataCount;

		*chunk = newChunk;
		*inst = reinterpret_cast<Instruction*>(newChunk->code);
		*lastInst = reinterpret_cast<Instruction*>(newChunk->code) + (newChunk->codeSize - 1);
	}

	static void init_call(RuntimeState* state, ChunkOffset chunkOffset, RegisterOffset firstRegister)
	{
		state->cstack->push(state);
		state->set((*state->chunk)->childChunks + chunkOffset, firstRegister);
	}

	static void finish_call(RuntimeState* state, Register returnedValue)
	{
		state->rstack->registers[0] = returnedValue;

		CallInfo* info = state->cstack->top;
		state->set(info);
		state->cstack->pop(state);
	}
}

#define move_pc(_Amount) inst += (_Amount)
#define opcode(_Inst) case _Inst : {
#define end_opcode(_Amount) move_pc(_Amount); } break

#define __reg(_Inst, _Part) (rstack->registers[(_Inst)-> ## _Part])
#define __stc(_Inst, _Part) (chunk->statics[(_Inst)-> ## _Part].value)
#define __is_static(_Inst, _Part) ((_Inst)->features.static_ ## _Part)

#define reg_stc(_Inst, _Part) (__is_static(_Inst, _Part) ? __reg(_Inst, _Part) : __stc(_Inst, _Part))
#define get_in_addr_reg_stc(_Inst, _Part, _Delta) \
	(*reinterpret_cast<Register*>(reinterpret_cast<char*>(reg_stc(_Inst, _Part)) + (_Delta)))
#define set_in_addr_reg_stc(_Inst, _Part, _Delta, _Value) (get_in_addr_reg_stc(_Inst, _Part, _Delta) = (_Value))

#define bx(_Inst) INSTRUCTION_BX_GET((_Inst)->Bx)

#define SBYTE(_Value) static_cast<kram::Int8>(_Value)
#define SWORD(_Value) static_cast<kram::Int16>(_Value)
#define SLONG(_Value) static_cast<kram::Int32>(_Value)
#define SQUAD(_Value) static_cast<kram::Int64>(_Value)
#define UBYTE(_Value) static_cast<kram::UInt8>(_Value)
#define UWORD(_Value) static_cast<kram::UInt16>(_Value)
#define ULONG(_Value) static_cast<kram::UInt32>(_Value)
#define UQUAD(_Value) static_cast<kram::UInt64>(_Value)

#define as_float(_Value) *reinterpret_cast<float*>(&(_Value))
#define as_double(_Value) *reinterpret_cast<double*>(&(_Value))

#define get_double_reg_stc(_Inst, _Part) (__is_static(_Inst, _Part) ? as_double(__reg(_Inst, _Part)) : as_double(__stc(_Inst, _Part)))
#define get_float_reg_stc(_Inst, _Part) static_cast<float>(get_double_reg_stc(_Inst, _Part))


#define set_float_reg_stc(_Inst, _Part, _Value) (get_double_reg_stc(_Inst, _Part) = static_cast<double>(_Value))
#define set_double_reg_stc(_Inst, _Part, _Value) (get_double_reg_stc(_Inst, _Part) = (_Value))

#define u_binary_op(_Op, _Type) reg_stc(fabc, A) = static_cast<_Type>(reg_stc(fabc, B) _Op reg_stc(fabc, C))
#define s_binary_op(_Op, _Type) reg_stc(fabc, A) = static_cast<_Type>(UQUAD(reg_stc(fabc, B)) _Op UQUAD(reg_stc(fabc, C)))
#define binary_op(_Op) switch (fabc->features.data_size) { \
	case op::data::DataSize::UByte: u_binary_op(_Op, UInt8); break; \
	case op::data::DataSize::UWord: u_binary_op(_Op, UInt16); break; \
	case op::data::DataSize::ULong: u_binary_op(_Op, UInt32); break; \
	case op::data::DataSize::UQuad: u_binary_op(_Op, UInt64); break; \
	case op::data::DataSize::SByte: u_binary_op(_Op, Int8); break; \
	case op::data::DataSize::SWord: u_binary_op(_Op, Int16); break; \
	case op::data::DataSize::SLong: u_binary_op(_Op, Int32); break; \
	case op::data::DataSize::SQuad: u_binary_op(_Op, Int64); break; \
}

#define __pre_unary_op(_Op, _Type) reg_stc(fabc, A) = static_cast<_Type>(_Op(reg_stc(fabc, B)))
#define pre_unary_op(_Op) switch (fabc->features.data_size) { \
	case op::data::DataSize::UByte: __pre_unary_op(_Op, UInt8); break; \
	case op::data::DataSize::UWord: __pre_unary_op(_Op, UInt16); break; \
	case op::data::DataSize::ULong: __pre_unary_op(_Op, UInt32); break; \
	case op::data::DataSize::UQuad: __pre_unary_op(_Op, UInt64); break; \
	case op::data::DataSize::SByte: __pre_unary_op(_Op, Int8); break; \
	case op::data::DataSize::SWord: __pre_unary_op(_Op, Int16); break; \
	case op::data::DataSize::SLong: __pre_unary_op(_Op, Int32); break; \
	case op::data::DataSize::SQuad: __pre_unary_op(_Op, Int64); break; \
}

#define __post_unary_op(_Op, _Type) reg_stc(fabc, A) = static_cast<_Type>((reg_stc(fabc, B))_Op)
#define post_unary_op(_Op) switch (fabc->features.data_size) { \
	case op::data::DataSize::UByte: __post_unary_op(_Op, UInt8); break; \
	case op::data::DataSize::UWord: __post_unary_op(_Op, UInt16); break; \
	case op::data::DataSize::ULong: __post_unary_op(_Op, UInt32); break; \
	case op::data::DataSize::UQuad: __post_unary_op(_Op, UInt64); break; \
	case op::data::DataSize::SByte: __post_unary_op(_Op, Int8); break; \
	case op::data::DataSize::SWord: __post_unary_op(_Op, Int16); break; \
	case op::data::DataSize::SLong: __post_unary_op(_Op, Int32); break; \
	case op::data::DataSize::SQuad: __post_unary_op(_Op, Int64); break; \
}



#define fab reinterpret_cast<kram::op::data::FAB_Instruction*>(inst)
#define fabc reinterpret_cast<kram::op::data::FABC_Instruction*>(inst)
#define fabx reinterpret_cast<kram::op::data::FABx_Instruction*>(inst)
#define fabxc reinterpret_cast<kram::op::data::FABxC_Instruction*>(inst)

#define end_empty() end_opcode(sizeof(kram::op::data::EmptyInstruction))
#define end_fab() end_opcode(sizeof(kram::op::data::FAB_Instruction))
#define end_fabc() end_opcode(sizeof(kram::op::data::FABC_Instruction))
#define end_fabx() end_opcode(sizeof(kram::op::data::FABx_Instruction))
#define end_fabxc() end_opcode(sizeof(kram::op::data::FABxC_Instruction))

void kram::runtime::execute(RegisterStack* rstack, AutodataStack* dstack, CallStack* cstack, bin::Chunk* chunk)
{
	using kram::op::data::DataSize;

	Instruction* inst = reinterpret_cast<Instruction*>(chunk->code);
	Instruction* lastInst = reinterpret_cast<Instruction*>(chunk->code) + (chunk->codeSize - 1);
	RuntimeState state{ rstack, dstack, cstack, &chunk, &inst, &lastInst };
	cstack->push(&state);

	for (;inst <= lastInst;)
	{
		switch (*inst)
		{
			opcode(Instruction::NOP)
			end_empty();


			opcode(Instruction::MOV)
				reg_stc(fab, A) = reg_stc(fab, B);
			end_fab();


			opcode(Instruction::PUT)
				reg_stc(fabx, A) = bx(fabx);
			end_fabx();


			opcode(Instruction::LD_S)
				reg_stc(fabx, A) = chunk->statics[bx(fabx)].value;
			end_fabx();

			opcode(Instruction::ST_S)
				chunk->statics[bx(fabx)].value = reg_stc(fabx, A);
			end_fabx();


			opcode(Instruction::LD_D)
				reg_stc(fabxc, A) = get_in_addr_reg_stc(fabxc, C, bx(fabxc));
			end_fabxc();

			opcode(Instruction::ST_D)
				set_in_addr_reg_stc(fabxc, A, bx(fabxc), reg_stc(fabxc, C));
			end_fabxc();


			opcode(Instruction::LD_RD)
				reg_stc(fab, A) = reinterpret_cast<Register>(&rstack->registers[fab->B]);
			end_fab();

			opcode(Instruction::LD_SD)
				reg_stc(fabx, A) = reinterpret_cast<Register>(&chunk->statics[bx(fabx)].value);
			end_fabx();

			opcode(Instruction::LD_AD)
				reg_stc(fabx, A) = reinterpret_cast<Register>(dstack->bottom + bx(fabx));
			end_fabx();


			opcode(Instruction::I2I)
				switch (fab->features.data_size)
				{
					case DataSize::UByte: reg_stc(fab, A) = UBYTE(reg_stc(fab, B)); break;
					case DataSize::UWord: reg_stc(fab, A) = UWORD(reg_stc(fab, B)); break;
					case DataSize::ULong: reg_stc(fab, A) = ULONG(reg_stc(fab, B)); break;
					case DataSize::UQuad: reg_stc(fab, A) = UQUAD(reg_stc(fab, B)); break;
					case DataSize::SByte: reg_stc(fab, A) = SBYTE(reg_stc(fab, B)); break;
					case DataSize::SWord: reg_stc(fab, A) = SWORD(reg_stc(fab, B)); break;
					case DataSize::SLong: reg_stc(fab, A) = SLONG(reg_stc(fab, B)); break;
					case DataSize::SQuad: reg_stc(fab, A) = SQUAD(reg_stc(fab, B)); break;
				}
			end_fab();

			opcode(Instruction::I2F)
				if (fab->features.is_double)
					set_double_reg_stc(fab, A, static_cast<double>(reg_stc(fab, B)));
				else set_float_reg_stc(fab, A, static_cast<float>(reg_stc(fab, B)));
			end_fab();

			opcode(Instruction::F2I)
				if (fab->features.is_double)
					switch (fab->features.data_size)
					{
						case DataSize::UByte: reg_stc(fab, A) = UBYTE(get_double_reg_stc(fab, B)); break;
						case DataSize::UWord: reg_stc(fab, A) = UWORD(get_double_reg_stc(fab, B)); break;
						case DataSize::ULong: reg_stc(fab, A) = ULONG(get_double_reg_stc(fab, B)); break;
						case DataSize::UQuad: reg_stc(fab, A) = UQUAD(get_double_reg_stc(fab, B)); break;
						case DataSize::SByte: reg_stc(fab, A) = SBYTE(get_double_reg_stc(fab, B)); break;
						case DataSize::SWord: reg_stc(fab, A) = SWORD(get_double_reg_stc(fab, B)); break;
						case DataSize::SLong: reg_stc(fab, A) = SLONG(get_double_reg_stc(fab, B)); break;
						case DataSize::SQuad: reg_stc(fab, A) = SQUAD(get_double_reg_stc(fab, B)); break;
					}
				else
					switch (fab->features.data_size)
					{
						case DataSize::UByte: reg_stc(fab, A) = UBYTE(get_float_reg_stc(fab, B)); break;
						case DataSize::UWord: reg_stc(fab, A) = UWORD(get_float_reg_stc(fab, B)); break;
						case DataSize::ULong: reg_stc(fab, A) = ULONG(get_float_reg_stc(fab, B)); break;
						case DataSize::UQuad: reg_stc(fab, A) = UQUAD(get_float_reg_stc(fab, B)); break;
						case DataSize::SByte: reg_stc(fab, A) = SBYTE(get_float_reg_stc(fab, B)); break;
						case DataSize::SWord: reg_stc(fab, A) = SWORD(get_float_reg_stc(fab, B)); break;
						case DataSize::SLong: reg_stc(fab, A) = SLONG(get_float_reg_stc(fab, B)); break;
						case DataSize::SQuad: reg_stc(fab, A) = SQUAD(get_float_reg_stc(fab, B)); break;
					}
			end_fab();

			opcode(Instruction::F2F)
				if (fab->features.is_double)
					set_double_reg_stc(fab, A, static_cast<double>(get_double_reg_stc(fab, B)));
				else set_float_reg_stc(fab, A, static_cast<float>(get_double_reg_stc(fab, B)));
			end_fab();


			opcode(Instruction::ADD)
				binary_op(+);
			end_fabc();

			opcode(Instruction::SUB)
				binary_op(-);
			end_fabc();

			opcode(Instruction::MUL)
				binary_op(*);
			end_fabc();

			opcode(Instruction::DIV)
				binary_op(/);
			end_fabc();



			opcode(Instruction::RET)
				finish_call(&state, 0);
			end_opcode(1);
		}
	}
}

