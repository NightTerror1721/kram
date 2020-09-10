#include <iostream>

#include "vm.h"
#include "bindata.h"
#include "opcodes.h"
#include "runtime.h"
#include "asm_parser.h"
#include "static_array.h"
#include "cperrors.h"

using namespace kram::op;
using namespace kram::assembler;

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

	SplitMode mode = false;

	Instruction inst;
	inst = instruction::mov(DataSize::DoubleWord, Register::r0, location(Segment::Stack, 5));


	kram::utils::CompilerError err;
	std::exception ex;

	const kram::utils::CompilerError& rerr = err;
	const std::exception& rex = ex;
	const std::exception& rex2 = err;

	bool result = typeid(rex2) == typeid(err);

	std::cout << typeid(rerr).name() << std::endl;
	std::cout << typeid(rex).name() << std::endl;
	std::cout << typeid(rex2).name() << std::endl;
	std::cout << (typeid(rex2) == typeid(err)) << std::endl;

	return 0;
}
