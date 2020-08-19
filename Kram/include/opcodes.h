#pragma once

#include "common.h"

namespace kram::op::inst
{
	enum class Instruction : UInt8
	{
		NOP = 0x00,

		MOV, /* [size:2|dest_mode:3|src_mode:3], [destination:8/16/32/64], [source:8/16/32/64]
			  * Move memory from source to destination
			  * Avaliable modes:
			  *		0: Register
			  *		1: Immediate value (only for source)
			  *		2: Stack data location
			  *		3: Stack data location + delta
			  *		4: Static data location
			  *		5: Static data location + delta
			  *		6: Location at register
			  *		7: Location at register + delta
			  * Available Sizes:
			  *		0: Byte (8 bits)
			  *		1: Word (16 bits)
			  *		2: DWord (32 bits)
			  *		3: QWord (64 bits)
			  */

		LDA, /* [register_dest:8], [stack_idx:32]
			 */


		/*MOV, //FAB// Move static/register[B] to static/register[A]

		PUT, //FABx// Set inmediate value[Bx] to static/register[A]

		LD_S, //FABx// Move static[Bx] to static/register[A]
		ST_S, //FABx// Move static/register[A] to static[Bx]

		LD_D, //FABxC// Move [static/register + [Bx]][C] to static/register[A]
		ST_D, //FABxC// Move static/register[C] to [static/register + [Bx]][A]

		LD_RD, //FAB// static/register[A] = &(register[B])
		LD_SD, //FABx// static/register[A] = &(static[Bx])
		LD_AD, //FABx// static/register[A] = &(autodata[Bx])

		I2I, //FAB// static/register[A] = (integer_type) integer(static/register)[B]
		I2F, //FAB// static/register[A] = (float_type) integer(static/register)[B]
		F2I, //FAB// static/register[A] = (integer_type) float(static/register)[B]
		F2F, //FAB// static/register[A] = (float_type) float(static/register)[B]

		ADD, //FABC// static/register[A] = (type) (static/register[B] + static/register[C])
		SUB, //FABC// static/register[A] = (type) (static/register[B] - static/register[C])
		MUL, //FABC// static/register[A] = (type) (static/register[B] * static/register[C])
		DIV, //FABC// static/register[A] = (type) (static/register[B] / static/register[C])



		RET, ////Empty return*/
	};
}

namespace kram::op::data
{
	
}
