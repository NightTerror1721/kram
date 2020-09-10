#pragma once

#include "common.h"
#include "opcodes.h"

namespace kram::assembler
{
	enum class DataSize { Byte, Word, DoubleWord, QuadWord };

	enum class DataType
	{
		SignedByte, SignedWord, SignedDoubleWord, SignedQuadWord,
		UnsignedByte, UnsignedWord, UnsignedDoubleWord, UnsignedQuadWord,
		FloatingDecimal, DoubleDecimal
	};

	enum class Register : UInt8
	{
		r0 = 0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15,
		sd = r9,
		sb = r10,
		sp = r11,
		sr = r12,
		ch = r13,
		st = r14,
		ip = r15
	};

	enum class Segment : UInt8
	{
		NoSegment,
		Stack,
		Static,
		Register
	};

	struct SegmentData
	{
		Segment id = Segment::NoSegment;
		Register reg = Register::r0;

		SegmentData() = default;
		SegmentData(const SegmentData&) = default;

		SegmentData& operator= (const SegmentData&) = default;

		constexpr SegmentData(Segment segment, Register reg) : id{ segment }, reg{ reg } {}
		constexpr SegmentData(Segment segment) : id{ segment }, reg{ Register::r0 } {}
		constexpr SegmentData(Register reg) : id{ Segment::Register }, reg{ reg } {}

		inline bool operator== (const SegmentData& right) const
		{
			return id == Segment::Register
				? right.id == Segment::Register && reg == right.reg
				: id == right.id;
		}
		inline bool operator!= (const SegmentData& right) const
		{
			return id == Segment::Register
				? right.id != Segment::Register || reg != right.reg
				: id != right.id;
		}

		inline SegmentData& operator= (Segment segment) { return id = segment, *this; }
		inline SegmentData& operator= (Register reg) { return id = Segment::Register, this->reg = reg, *this; }
	};

	class Value;
	class UnsignedInteger;

	class Value
	{
	private:
		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
			static constexpr UInt8 _adapt(_Ty data)
		{
			if constexpr (std::integral<_Ty>)
				return static_cast<UInt64>(data);
			else if constexpr (sizeof(_Ty) == 4)
				return static_cast<UInt64>(reinterpret_cast<UInt32>(&data));
			else return reinterpret_cast<UInt64>(&data);
		}

		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
			static constexpr DataType _detect_type(_Ty data)
		{
			if constexpr (std::unsigned_integral<_Ty>)
			{
				if constexpr (sizeof(_Ty) == 1) return DataType::UnsignedByte;
				else if constexpr (sizeof(_Ty) == 2) return DataType::UnsignedWord;
				else if constexpr (sizeof(_Ty) == 4) return DataType::UnsignedDoubleWord;
				else return DataType::UnsignedQuadWord;
			}
			else if constexpr (std::signed_integral<_Ty>)
			{
				if constexpr (sizeof(_Ty) == 1) return DataType::SignedByte;
				else if constexpr (sizeof(_Ty) == 2) return DataType::SignedWord;
				else if constexpr (sizeof(_Ty) == 4) return DataType::SignedDoubleWord;
				else return DataType::SignedQuadWord;
			}
			else if constexpr (sizeof(_Ty) == 4)
				return DataType::FloatingDecimal;
			else return DataType::DoubleDecimal;
		}

		UInt64 _data = 0;
		DataType _type = DataType::UnsignedByte;

	public:
		Value() = default;

		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
		Value(_Ty data) : _data{ _adapt<_Ty>(data) }, _type{ _detect_type<_Ty>(data) } {}

		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
		inline Value & operator= (_Ty data) { _data = _adapt<_Ty>(data), _type = _detect_type<_Ty>(data); }

		inline DataSize bytes() const
		{
			if (_data > 0xFFFFFFFFULL) return DataSize::QuadWord;
			else if (_data > 0xFFFFULL) return DataSize::DoubleWord;
			else if (_data > 0xFFULL) return DataSize::Word;
			else return DataSize::Byte;
		}

		inline DataType type() const { return _type; }

		inline operator UInt64() const { return _data; }

		inline bool operator== (const Value& right) const { return _data == right._data; }
		inline bool operator!= (const Value& right) const { return _data != right._data; }

		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
		inline bool operator== (const _Ty& right) const { return _data == _adapt(right); }

		template<typename _Ty>
		requires std::integral<_Ty> || std::floating_point<_Ty>
		inline bool operator!= (const _Ty& right) const { return _data != _adapt(right); }

	public:
		Value(const UnsignedInteger& uint);

		Value& operator= (const UnsignedInteger& right);
	};

	class UnsignedInteger
	{
	private:
		template<std::integral _Ty>
		static constexpr UInt64 _adapt(_Ty value) { return static_cast<UInt64>(value); }

		UInt64 _value = 0;

	public:
		UnsignedInteger() = default;

		template<std::integral _Ty>
		UnsignedInteger(_Ty value) : _value{ _adapt<_Ty>(value) } {}

		template<std::integral _Ty>
		inline UnsignedInteger& operator= (_Ty value) { return _value = _adapt<_Ty>(value), *this; }

		inline DataSize bytes() const
		{
			if (_value > 0xFFFFFFFFULL) return DataSize::QuadWord;
			else if (_value > 0xFFFFULL) return DataSize::DoubleWord;
			else if (_value > 0xFFULL) return DataSize::Word;
			else return DataSize::Byte;
		}

		inline bool is_zero() const { return _value == 0; }

		inline operator UInt64() const { return _value; }

		inline operator bool() const { return _value; }

		inline bool operator! () const { return !_value; }

		inline bool operator== (const UnsignedInteger& right) const { return _value == right._value; }
		inline bool operator!= (const UnsignedInteger& right) const { return _value != right._value; }

		template<std::integral _Ty>
		inline bool operator== (const _Ty & right) const { return _value == _adapt(right); }

		template<std::integral _Ty>
		inline bool operator!= (const _Ty & right) const { return _value != _adapt(right); }

	public:
		UnsignedInteger(const Value& value);

		UnsignedInteger& operator= (const Value& value);
	};

	inline Value::Value(const UnsignedInteger& uint) : _data{ uint }, _type{}
	{
		switch (uint.bytes())
		{
			case DataSize::Byte: _type = DataType::UnsignedByte; break;
			case DataSize::Word: _type = DataType::UnsignedWord; break;
			case DataSize::DoubleWord: _type = DataType::UnsignedDoubleWord; break;
			case DataSize::QuadWord: _type = DataType::UnsignedQuadWord; break;
		}
	}

	inline Value& Value::operator= (const UnsignedInteger& right)
	{
		_data = right;
		switch (right.bytes())
		{
			case DataSize::Byte: _type = DataType::UnsignedByte; break;
			case DataSize::Word: _type = DataType::UnsignedWord; break;
			case DataSize::DoubleWord: _type = DataType::UnsignedDoubleWord; break;
			case DataSize::QuadWord: _type = DataType::UnsignedQuadWord; break;
		}
		return *this;
	}

	inline UnsignedInteger::UnsignedInteger(const Value& value) : _value{ value } {}

	inline UnsignedInteger& UnsignedInteger::operator= (const Value& right) { return _value = right, *this; }



	struct SplitMode
	{
		bool enabled = false;
		Register reg = Register::r0;
		DataSize size = DataSize::Byte;

		SplitMode() = default;
		SplitMode(const SplitMode&) = default;

		SplitMode& operator= (const SplitMode&) = default;

		constexpr SplitMode(bool enabled, Register reg, DataSize size) : enabled{ enabled }, reg{ reg }, size{ size } {}
		constexpr SplitMode(Register reg, DataSize size = DataSize::Byte) : enabled{ true }, reg{ reg }, size{ size } {}

		inline bool operator== (const SplitMode& right) const
		{
			return enabled
				? right.enabled && reg == right.reg && size == right.size
				: !right.enabled;
		}
		inline bool operator!= (const SplitMode& right) const
		{
			return enabled
				? !right.enabled || reg != right.reg || size != right.size
				: right.enabled;
		}

		template<utils::boolean __Bool>
		constexpr SplitMode(__Bool enabled) : enabled{ enabled }, reg{ Register::r0 }, size{ size = DataSize::Byte } {}

		inline SplitMode& operator= (bool right) { return enabled = right, *this; }
	};

	struct MemoryLocation
	{
		SegmentData segment;
		SplitMode split;
		UnsignedInteger delta;

		bool operator== (const MemoryLocation& right) const;
		bool operator!= (const MemoryLocation& right) const;
	};

	inline MemoryLocation location(const SegmentData& segment, const SplitMode& split, const UnsignedInteger& delta)
	{
		return { segment, split, delta };
	}
	inline MemoryLocation location(const SegmentData& segment, Register splitRegister, DataSize splitMode = DataSize::Byte, const UnsignedInteger& delta = {})
	{
		return { segment, { true, splitRegister, splitMode }, delta };
	}
	inline MemoryLocation location(const SegmentData& segment, const UnsignedInteger& delta)
	{
		return { segment, {}, delta };
	}
	inline MemoryLocation location(const SegmentData& segment, const SplitMode& split)
	{
		return { segment, split, {} };
	}
	inline MemoryLocation location(const UnsignedInteger& address)
	{
		return { {}, {}, address };
	}
}

namespace kram::assembler::instruction
{
	using op::Instruction;

	Instruction mov(DataSize size, Register dest, Register src);
	Instruction mov(DataSize size, bool mem_to_reg, const MemoryLocation& location, Register reg);
	Instruction mov(DataSize size, Register dest, const Value& immediateValue);
	Instruction mov(DataSize size, const MemoryLocation& dest, const Value& immediateValue);

	inline Instruction mov(DataSize size, Register dest, const MemoryLocation& src) { return mov(size, true, src, dest); }
	inline Instruction mov(DataSize size, const MemoryLocation& dest, Register src) { return mov(size, false, dest, src); }

	Instruction lea(Register dest, const MemoryLocation& src);

	Instruction mmb(DataSize size, Register dest, Register src, const UnsignedInteger& block_bytes);

	Instruction new_(bool add_ref, Register dest, const UnsignedInteger& block_bytes);
	Instruction new_(bool add_ref, const MemoryLocation& dest, const UnsignedInteger& block_bytes);

	Instruction del(Register src);
	Instruction del(const MemoryLocation& src);

	Instruction mhr(bool increase, Register src);
	Instruction mhr(bool increase, const MemoryLocation& src);

	Instruction cst(DataType dest_type, DataType src_type, Register target);
	Instruction cst(DataType dest_type, DataType src_type, const MemoryLocation& target);
}
