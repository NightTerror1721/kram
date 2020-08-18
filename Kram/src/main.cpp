#include "vm.h"
#include "bindata.h"
#include "opcodes.h"

int main(int argc, char** argv)
{
	kram::Register r0;
	r0.s32 = 256;

	/*kram::KramState state{ 1024 * 1024 };
	int* value = static_cast<int*>(state.malloc(sizeof(int)));
	*value = 50;
	state.free(value);*/


	return 0;
}
