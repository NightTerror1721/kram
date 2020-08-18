#pragma once

#include "common.h"

namespace kram::op::inst
{
	enum class Instruction : UInt8
	{
		NOP = 0x00,

		MOV, //FAB// Move static/register[B] to static/register[A]

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



		RET, ////Empty return
	};
}

#define INSTRUCTION_BX_GET(_Bx) *reinterpret_cast<kram::UInt64*>(&(_Bx)[0])
#define INSTRUCTION_BX_SET(_Bx, _Value) (INSTRUCTION_BX_GET(_Bx) = (_Value))
namespace kram::op::data
{
	enum class DataSize : UInt8
	{
		UByte = 0x0,
		UWord = 0x1,
		ULong = 0x2,
		UQuad = 0x3,
		SByte = 0x4,
		SWord = 0x5,
		SLong = 0x6,
		SQuad = 0x7,
	};

	namespace
	{
		struct _InstructionFeatures
		{
			bool static_A : 1;
			bool static_B : 1;
			bool static_C : 1;

			DataSize data_size : 3;

			bool is_double : 1;

			bool jump : 1;
		};
	}

	struct EmptyInstruction { inst::Instruction opcode; };

	struct FAB_Instruction
	{
		inst::Instruction opcode;
		_InstructionFeatures features;
		UInt8 A;
		UInt8 B;
	};

	struct FABC_Instruction
	{
		inst::Instruction opcode;
		_InstructionFeatures features;
		UInt8 A;
		UInt8 B;
		UInt8 C;
	};

	struct FABx_Instruction
	{
		inst::Instruction opcode;
		_InstructionFeatures features;
		UInt8 A;
		UInt8 Bx[8];
	};

	struct FABxC_Instruction
	{
		inst::Instruction opcode;
		_InstructionFeatures features;
		UInt8 A;
		UInt8 Bx[8];
		UInt8 C;
	};

	//constexpr Size s = sizeof(FBxCA_Instruction);
}
