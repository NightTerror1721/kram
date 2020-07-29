#include "runtime.h"

using namespace kram::bin;

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

		info->returnInstruction = *state->inst;
		info->returnRegister = registerToReturn;
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


	RuntimeState::RuntimeState(RuntimeStack* rstack, CallStack* cstack, bin::Chunk** chunk, InstructionOffset* inst) :
		rstack{ rstack },
		cstack{ cstack },
		chunk{ chunk },
		inst{ inst },
		error{ ErrorCode::OK }
	{}

	void RuntimeState::set(CallInfo* info)
	{
		rstack->top = info->top;
		rstack->data = info->data;
		rstack->bottom = info->bottom;

		*chunk = info->chunk;
		*inst = info->returnInstruction;
	}
}

void kram::runtime::execute(RuntimeStack* rstack, CallStack* cstack, Chunk* chunk)
{

}

