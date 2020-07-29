#pragma once

#include "common.h"

namespace kram::op::inst
{
	enum Instruction : UInt8
	{
		NOP = 0x00,

		MOV_R_R, //Move register to register
		MOV_C_R, //Move constant to register
		MOV_DR_R, //Move register direction to register
		MOV_DC_R, //Move constant direction to register

		MOV_R_DR, //Move register to direction in register
		MOV_C_DR, //Move constant to direction in register
		MOV_DR_DR, //Move register direction to direction in register
		MOV_DC_DR, //Move constant direction to direction in register

		MOV_R_DC, //Move register to direction in constant
		MOV_C_DC, //Move constant to direction in constant
		MOV_DR_DC, //Move register direction to direction in constant
		MOV_DC_DC, //Move constant direction to direction in constant
	};
}
