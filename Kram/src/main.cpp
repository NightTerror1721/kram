#include <iostream>

#include "vm.h"
#include "bindata.h"
#include "opcodes.h"
#include "runtime.h"
#include "kram_asm_parser.h"
#include "static_array.h"

using namespace kram::op;
using namespace kram::op::build;

int add(int a, int b)
{
	return a + b;
}

int main(int argc, char** argv)
{
	/*kram::Register r0;
	r0.s32 = 1;

	kram::KramState state;
	int* value = static_cast<int*>(state.malloc(sizeof(int)));
	*value = 50;
	kram::Heap::decrease_ref(value);
	state.garbage_collector();*/

	/*kram::assembler::parser::Element e, e2 = kram::assembler::parser::Element::comma();

	e = e2;

	int x = add(50, -25);*/

	Instruction inst;
	inst = mov(DataSize::DoubleWord, Register::r0, location(Segment::Stack, 5));


	return 0;
}
