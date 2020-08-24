#include "vm.h"

namespace kram
{
	KramState::KramState() :
		_rstack{}
	{
		runtime::_build_stack(&_rstack, utils::RuntimeStackDefaultSize);
	}
	KramState::~KramState()
	{
		runtime::_destroy_stack(&_rstack);
	}
}
