#include "asm_common.h"

namespace kram::assembler
{
	bool MemoryLocation::operator== (const MemoryLocation& right) const
	{
		return segment == right.segment &&
			split == right.split &&
			delta == right.delta;
	}
	bool MemoryLocation::operator!= (const MemoryLocation& right) const
	{
		return segment != right.segment ||
			split != right.split ||
			delta != right.delta;
	}
}

namespace kram::assembler::instruction
{
	using namespace op;

	template<unsigned int _BitIdx, unsigned int _BitCount, typename _BaseTy, typename _ValueTy>
	constexpr UInt8 bits(_BaseTy base, _ValueTy value) { return utils::set_bits<_BitIdx, _BitCount, UInt8>(scast(UInt8, base), scast(UInt8, value)); }

	template<unsigned int _BitIdx, unsigned int _BitCount, typename _ValueTy>
	constexpr UInt8 bits(_ValueTy value) { return utils::set_bits<_BitIdx, _BitCount, UInt8>(0, scast(UInt8, value)); }

	template<typename _Ty> requires std::same_as<_Ty, UnsignedInteger> || std::same_as<_Ty, Value>
	Instruction & add_value(Instruction & inst, const _Ty & uint)
	{
		UInt64 value = uint;
		switch (uint.bytes())
		{
			case DataSize::Byte: inst.add_byte(scast(UInt8, value)); break;
			case DataSize::Word: inst.add_word(scast(UInt16, value)); break;
			case DataSize::DoubleWord: inst.add_dword(scast(UInt32, value)); break;
			case DataSize::QuadWord: inst.add_qword(value); break;
		}
		return inst;
	}

	Instruction& add_location(Instruction& inst, const MemoryLocation& loc)
	{
		inst.add_byte(bits<0, 2>(loc.segment.id) |
			bits<2, 1>(loc.split.enabled) |
			bits<3, 2>(loc.split.size) |
			bits<5, 1>(!loc.delta.is_zero()) |
			bits<6, 2>(loc.delta.bytes()));
		
		inst.add_byte(
			bits<0, 4>(loc.segment.id == Segment::Register ? loc.segment.reg : Register::r0) |
			bits<4, 4>(loc.split.enabled ? loc.split.reg : Register::r0)
		);

		if (!loc.delta.is_zero())
			add_value(inst, loc.delta);

		return inst;
	}



	Instruction mov(DataSize size, Register dest, Register src)
	{
		Instruction inst;

		switch (size)
		{
			case DataSize::Byte: inst.opcode(Opcode::MOV_r8_r8); break;
			case DataSize::Word: inst.opcode(Opcode::MOV_r16_r16); break;
			case DataSize::DoubleWord: inst.opcode(Opcode::MOV_r32_r32); break;
			case DataSize::QuadWord: inst.opcode(Opcode::MOV_r64_r64); break;
		}

		inst.add_byte(bits<0, 4>(dest) | bits<4, 4>(src));

		return inst;
	}

	Instruction mov(DataSize size, bool mem_to_reg, const MemoryLocation& location, Register reg)
	{
		Instruction inst;

		switch (size)
		{
			case DataSize::Byte: inst.opcode(mem_to_reg ? Opcode::MOV_r8_m8 : Opcode::MOV_m8_r8); break;
			case DataSize::Word: inst.opcode(mem_to_reg ? Opcode::MOV_r16_m16 : Opcode::MOV_m16_r16); break;
			case DataSize::DoubleWord: inst.opcode(mem_to_reg ? Opcode::MOV_r32_m32 : Opcode::MOV_m32_r32); break;
			case DataSize::QuadWord: inst.opcode(mem_to_reg ? Opcode::MOV_r64_m64 : Opcode::MOV_m64_r64); break;
		}

		inst.add_byte(bits<0, 4>(reg));
		add_location(inst, location);

		return inst;
	}

	Instruction mov(DataSize size, Register dest, const Value& immediateValue)
	{
		Instruction inst;

		switch (size)
		{
			case DataSize::Byte: inst.opcode(Opcode::MOV_r8_imm8); break;
			case DataSize::Word: inst.opcode(Opcode::MOV_r16_imm16); break;
			case DataSize::DoubleWord: inst.opcode(Opcode::MOV_r32_imm32); break;
			case DataSize::QuadWord: inst.opcode(Opcode::MOV_r64_imm64); break;
		}

		inst.add_byte(bits<0, 4>(dest));
		add_value(inst, immediateValue);

		return inst;
	}

	Instruction mov(DataSize size, const MemoryLocation& dest, const Value& immediateValue)
	{
		Instruction inst;

		switch (size)
		{
			case DataSize::Byte: inst.opcode(Opcode::MOV_m8_imm8); break;
			case DataSize::Word: inst.opcode(Opcode::MOV_m16_imm16); break;
			case DataSize::DoubleWord: inst.opcode(Opcode::MOV_m32_imm32); break;
			case DataSize::QuadWord: inst.opcode(Opcode::MOV_m64_imm64); break;
		}

		add_value(inst, immediateValue);
		add_location(inst, dest);

		return inst;
	}

	Instruction lea(Register dest, const MemoryLocation& src)
	{
		Instruction inst;

		inst.opcode(Opcode::LEA);

		inst.add_byte(bits<0, 4>(dest));
		add_location(inst, src);

		return inst;
	}

	Instruction mmb(DataSize size, Register dest, Register src, const UnsignedInteger& block_bytes)
	{
		Instruction inst;

		switch (size)
		{
			case DataSize::Byte: inst.opcode(Opcode::MMB_sb); break;
			case DataSize::Word: inst.opcode(Opcode::MMB_sw); break;
			case DataSize::DoubleWord: inst.opcode(Opcode::MMB_sd); break;
			case DataSize::QuadWord: inst.opcode(Opcode::MMB_sq); break;
		}

		inst.add_byte(bits<0, 4>(dest) | bits<4, 4>(src));
		add_value(inst, block_bytes);

		return inst;
	}

	Instruction new_(bool add_ref, Register dest, const UnsignedInteger& block_bytes)
	{
		Instruction inst;

		inst.opcode(Opcode::NEW_r_s);

		inst.add_byte(bits<0, 4>(dest) | bits<4, 2>(block_bytes.bytes()) | bits<6, 1>(add_ref));
		add_value(inst, block_bytes);

		return inst;
	}

	Instruction new_(bool add_ref, const MemoryLocation& dest, const UnsignedInteger& block_bytes)
	{
		Instruction inst;

		inst.opcode(Opcode::NEW_m_s);

		inst.add_byte(bits<0, 2>(block_bytes.bytes()) | bits<2, 1>(add_ref));
		add_value(inst, block_bytes);
		add_location(inst, dest);

		return inst;
	}

	Instruction del(Register src)
	{
		Instruction inst;

		inst.opcode(Opcode::DEL_r);

		inst.add_byte(bits<0, 4>(src));

		return inst;
	}

	Instruction del(const MemoryLocation& src)
	{
		Instruction inst;

		inst.opcode(Opcode::DEL_m);

		add_location(inst, src);

		return inst;
	}

	Instruction mhr(bool increase, Register src)
	{
		Instruction inst;

		inst.opcode(Opcode::MHR_r);

		inst.add_byte(bits<0, 4>(src) | bits<4, 1>(increase));

		return inst;
	}

	Instruction mhr(bool increase, const MemoryLocation& src)
	{
		Instruction inst;

		inst.opcode(Opcode::DEL_m);

		inst.add_byte(bits<0, 1>(increase));
		add_location(inst, src);

		return inst;
	}

	Instruction cst(DataType dest_type, DataType src_type, Register target)
	{
		Instruction inst;

		inst.opcode(Opcode::CST_r);

		inst.add_byte(bits<0, 4>(target));
		inst.add_byte(bits<0, 4>(dest_type) | bits<4, 4>(src_type));

		return inst;
	}

	Instruction cst(DataType dest_type, DataType src_type, const MemoryLocation& target)
	{
		Instruction inst;

		inst.opcode(Opcode::CST_m);

		inst.add_byte(bits<0, 4>(dest_type) | bits<4, 4>(src_type));
		add_location(inst, target);

		return inst;
	}
}
