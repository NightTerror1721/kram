#include <iostream>

#include "vm.h"
#include "bindata.h"
#include "opcodes.h"

int main(int argc, char** argv)
{
	kram::Register r0;
	r0.s32 = 1;

	kram::KramState state;
	int* value = static_cast<int*>(state.malloc(sizeof(int)));
	*value = 50;
	kram::Heap::decrease_ref(value);
	state.garbage_collector();


	return 0;
}
