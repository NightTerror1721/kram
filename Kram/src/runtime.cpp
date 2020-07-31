#include "runtime.h"

using namespace kram::bin;
using kram::op::inst::Instruction;

namespace kram::runtime
{
	bool CallStack::push(RuntimeState* state, RegisterOffset registerToReturn)
	{
		RuntimeStack* rstack = state->rstack;

		CallInfo* info = top + 1;
		if (info >= roof)
		{
			state->error = ErrorCode::CallStack_Overflow;
			return false;
		}

		info->inst = *state->inst;
		info->lastInst = *state->lastInst;
		info->returnRegister = registerToReturn;
		info->chunk = *state->chunk;

		info->top = rstack->top;
		info->data = rstack->data;
		info->bottom = rstack->bottom;

		return top = info, true;
	}

	bool CallStack::pushFirst(RuntimeState* state)
	{
		RuntimeStack* rstack = state->rstack;

		CallInfo* info = top + 1;
		if (info >= roof)
		{
			state->error = ErrorCode::CallStack_Overflow;
			return false;
		}

		info->inst = *state->lastInst;
		info->lastInst = *state->lastInst;
		info->returnRegister = 0;
		info->chunk = *state->chunk;

		info->top = rstack->top;
		info->data = rstack->data;
		info->bottom = rstack->bottom;

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


	RuntimeState::RuntimeState(RuntimeStack* rstack, CallStack* cstack, Chunk** chunk, Instruction** inst, op::inst::Instruction** lastInst) :
		rstack{ rstack },
		cstack{ cstack },
		chunk{ chunk },
		inst{ inst },
		lastInst{ lastInst },
		error{ ErrorCode::OK }
	{}

	void RuntimeState::set(CallInfo* info)
	{
		rstack->top = info->top;
		rstack->data = info->data;
		rstack->bottom = info->bottom;

		*chunk = info->chunk;
		*inst = info->inst;
		*lastInst = info->lastInst;
	}

	void RuntimeState::set(Chunk* newChunk)
	{
		rstack->bottom = rstack->top + 1;
		rstack->data = rstack->bottom + (static_cast<Size>(newChunk->parameterCount) + newChunk->variableCount);
		rstack->top = rstack->data + newChunk->dataByteCount;

		*chunk = newChunk;
		*inst = reinterpret_cast<Instruction*>(newChunk->code);
		*lastInst = reinterpret_cast<Instruction*>(newChunk->code) + (newChunk->codeSize - 1);
	}

	static void init_call(RuntimeState* state, ChunkOffset chunkOffset, RegisterOffset registerToReturn)
	{
		state->cstack->push(state, registerToReturn);
		state->set((*state->chunk)->childChunks + chunkOffset);
	}

	static void finish_call(RuntimeState* state, Register returnedValue)
	{
		CallInfo* info = state->cstack->top;
		state->set(info);
		state->cstack->pop(state);
		state->rstack->registers[info->returnRegister] = returnedValue;
	}
}

#define move_pc(_Amount) inst += (_Amount)
#define opcode(_Inst) case _Inst : {
#define end_opcode(_Amount) move_pc(_Amount); } break

#define _arg(_Index, _Type) *reinterpret_cast<_Type *>(inst + (_Index))
#define byte_arg(_Index) _arg(_Index, UInt8)
#define word_arg(_Index) _arg(_Index, UInt16)

void kram::runtime::execute(RuntimeStack* rstack, CallStack* cstack, Chunk* chunk)
{
	Instruction* inst = reinterpret_cast<Instruction*>(chunk->code);
	Instruction* lastInst = reinterpret_cast<Instruction*>(chunk->code) + (chunk->codeSize - 1);
	RuntimeState state{ rstack, cstack, &chunk, &inst, &lastInst };
	cstack->pushFirst(&state);

	for (;inst <= lastInst;)
	{
		switch (*inst)
		{
			opcode(Instruction::NOP)
			end_opcode(1);


			opcode(Instruction::MOV_R_R)
				rstack->registers[byte_arg(2)] = rstack->registers[byte_arg(1)];
			end_opcode(3);

			opcode(Instruction::MOV_C_R)
				rstack->registers[byte_arg(3)] = chunk->constants[word_arg(1)].value;
			end_opcode(4);

			opcode(Instruction::MOV_DR_R)
				rstack->registers[byte_arg(2)] = reinterpret_cast<Register>(rstack->registers + byte_arg(1));
			end_opcode(3);

			opcode(Instruction::MOV_DC_R)
				rstack->registers[byte_arg(3)] = reinterpret_cast<Register>(&chunk->constants[word_arg(1)].value);
			end_opcode(4);


			opcode(Instruction::MOV_R_DR)
				*reinterpret_cast<Register*>(rstack->registers[byte_arg(2)]) = rstack->registers[byte_arg(1)];
			end_opcode(3);

			opcode(Instruction::MOV_C_DR)
				*reinterpret_cast<Register*>(rstack->registers[byte_arg(3)]) = chunk->constants[word_arg(1)].value;
			end_opcode(4);

			opcode(Instruction::MOV_DR_DR)
				*reinterpret_cast<Register*>(rstack->registers[byte_arg(2)]) = reinterpret_cast<Register>(rstack->registers + byte_arg(1));
			end_opcode(3);

			opcode(Instruction::MOV_DC_DR)
				*reinterpret_cast<Register*>(rstack->registers[byte_arg(3)]) = reinterpret_cast<Register>(&chunk->constants[word_arg(1)].value);
			end_opcode(4);


			opcode(Instruction::MOV_R_DC)
				*reinterpret_cast<Register*>(chunk->constants[byte_arg(2)].value) = rstack->registers[byte_arg(1)];
			end_opcode(4);

			opcode(Instruction::MOV_C_DC)
				*reinterpret_cast<Register*>(chunk->constants[byte_arg(3)].value) = chunk->constants[word_arg(1)].value;
			end_opcode(5);

			opcode(Instruction::MOV_DR_DC)
				*reinterpret_cast<Register*>(chunk->constants[byte_arg(2)].value) = reinterpret_cast<Register>(rstack->registers + byte_arg(1));
			end_opcode(4);

			opcode(Instruction::MOV_DC_DC)
				*reinterpret_cast<Register*>(chunk->constants[byte_arg(3)].value) = reinterpret_cast<Register>(&chunk->constants[word_arg(1)].value);
			end_opcode(5);


			opcode(Instruction::RET)
				finish_call(&state, 0);
			end_opcode(1);
		}
	}
}

