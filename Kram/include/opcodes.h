#pragma once

#include "common.h"

namespace kram::op
{
	enum class Opcode : UInt8
	{
		NOP = 0x00,

		MOV_r8_r8,
		MOV_r16_r16,
		MOV_r32_r32,
		MOV_r64_r64, /* <dest_reg:4|src_reg:4>
					  * Move register to register.
					  */

		MOV_r8_m8,
		MOV_r16_m16,
		MOV_r32_m32,
		MOV_r64_m64, /* <segment:2|has_reg2:1|reg2_split:2|has_delta:1|delta_size:2>, <1reg:4|2reg:4>, [delta:8-64]
					  * Move register or memory data to register.
					  */

		MOV_m8_r8,
		MOV_m16_r16,
		MOV_m32_r32,
		MOV_m64_r64, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|2reg:4>, [delta:8-64]
					  * Move register data to register or memory.
					  */

		MOV_r8_imm8,
		MOV_r16_imm16,
		MOV_r32_imm32,
		MOV_r64_imm64, /* <1reg:4|(padding):4>, <value:8-64>
						* Move immediate value to register.
						*/

		MOV_m8_imm8,
		MOV_m16_imm16,
		MOV_m32_imm32,
		MOV_m64_imm64, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|<padding>:4>, <value:8-64>, [delta:8-64]
						* Move immediate value to register or memory.
						*/

		LEA, /* <segment:2|has_reg2:1|reg2_split:2|has_delta:1|delta_size:2>, <1reg:4|2reg:4>, [delta:8-64]
			  * Load effective addres from memory to register
			  */

		MMB_sb,
		MMB_sw,
		MMB_sd,
		MMB_sq, /* <dest_reg:4|src_reg:4>, <size:8-64>
				 * Move memory block with "size" size from src_reg to dest_reg
			     */

		NEW_r_s, /* <dest_reg:4|bytes_size:2|add_ref:1|(padding):1>, <bytes:8-64>
				  * Allocate new memory block of "bytes" bytes and store address into dest_reg
				  */

		NEW_m_s, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|bytes_size:2|add_ref:1|<padding>:1>, <bytes:8-64>, [delta:8-64]
				  * Allocate new memory block of "bytes" bytes and store address into memory
			      */

		DEL_r, /* <src_reg:4|(padding):4>
				* Free memory block from src_rec
				*/

		DEL_m, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|<padding>:4>, [delta:8-64]
				* Free memory block from memory
				*/

		MHR_r, /* <src_reg:4|ref_inc:1|(padding):3>
			    * Increase or decrease memory heap object counter reference from src_reg
			    */

		MHR_m, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|ref_inc:1|<padding>:3>, [delta:8-64]
				* Increase or decrease memory heap object counter reference from memory
				*/

		CST_r, /* <reg:4|(padding):4>, <dest_type:4|src_type:4>
			    * Cast value from "src_type" to "dest_type" in register.
			    */

		CST_m, /* <segment:2|has_reg1:1|reg1_split:2|has_delta:1|delta_size:2>, <1reg:4|<padding>:4>, <dest_type:4|src_type:4>, [delta:8-64]
				* Increase or decrease memory heap object counter reference from memory
				*/
	};
}

namespace kram::op
{
	class Instruction
	{
	private:
		Opcode _opcode;
		std::vector<std::byte> _args;

	public:
		Instruction();
		Instruction(Opcode opcode, const std::vector<std::byte>& args = {});

		Instruction(const Instruction&) = default;
		Instruction(Instruction&&) noexcept = default;

		Instruction& operator= (const Instruction&) = default;
		Instruction& operator= (Instruction&&) noexcept = default;

		bool operator== (const Instruction& right) const;
		bool operator!= (const Instruction& right) const;

		inline void opcode(Opcode opcode) { _opcode = opcode; }
		inline Opcode opcode() const { return _opcode; }

		inline Instruction& add_byte(UInt8 value) { _args.push_back(static_cast<std::byte>(value)); return *this; }
		inline Instruction& add_sbyte(Int8 value) { _args.push_back(static_cast<std::byte>(value)); return *this; }

		Instruction& add_word(UInt16 value);
		Instruction& add_dword(UInt32 value);
		Instruction& add_qword(UInt64 value);

		Instruction& add_sword(Int16 value);
		Instruction& add_sdword(Int32 value);
		Instruction& add_sqword(Int64 value);

		inline Instruction& operator<< (UInt8 value) { return add_byte(value); }
		inline Instruction& operator<< (UInt16 value) { return add_word(value); }
		inline Instruction& operator<< (UInt32 value) { return add_dword(value); }
		inline Instruction& operator<< (UInt64 value) { return add_qword(value); }

		inline Instruction& operator<< (Int8 value) { return add_sbyte(value); }
		inline Instruction& operator<< (Int16 value) { return add_sword(value); }
		inline Instruction& operator<< (Int32 value) { return add_sdword(value); }
		inline Instruction& operator<< (Int64 value) { return add_sqword(value); }

		inline Size byte_count() const { return _args.size() + sizeof(Opcode); }

		template<typename _Ty>
		inline _Ty& arg(unsigned int index)
		{
			if (_args.size() >= (index + (sizeof(_Ty) - 1)))
				_args.resize(index + sizeof(_Ty), static_cast<std::byte>(0));

			return reinterpret_cast<_Ty&>(_args[index]);
		}

		inline Instruction& set_byte(unsigned int index, UInt8 value) { return (arg<UInt8>(index) = value), *this; }
		inline Instruction& set_word(unsigned int index, UInt16 value) { return (arg<UInt16>(index) = value), *this; }
		inline Instruction& set_dword(unsigned int index, UInt32 value) { return (arg<UInt32>(index) = value), *this; }
		inline Instruction& set_qword(unsigned int index, UInt64 value) { return (arg<UInt64>(index) = value), *this; }

		inline Instruction& set_sbyte(unsigned int index, Int8 value) { return (arg<Int8>(index) = value), *this; }
		inline Instruction& set_sword(unsigned int index, Int16 value) { return (arg<Int16>(index) = value), *this; }
		inline Instruction& set_sdword(unsigned int index, Int32 value) { return (arg<Int32>(index) = value), *this; }
		inline Instruction& set_sqword(unsigned int index, Int64 value) { return (arg<Int64>(index) = value), *this; }

		void write(void* buffer, Size buffer_size) const;

		friend std::ostream& operator<< (std::ostream& os, const Instruction& inst);
	};

	class InstructionBuilder
	{
	private:
		struct Node;
		
	public:
		typedef Node* Location;

	private:
		struct Node
		{
			friend class InstructionBuilder;
		private:
			Instruction _instruction;
			Node* _next = nullptr;
			Node* _prev = nullptr;

		public:
			inline Location next() { return _next; }
			inline Location prev() { return _prev; }

			inline bool has_next() { return _next; }
			inline bool has_prev() { return _prev; }
		};

	private:
		Node* _head;
		Node* _tail;
		Size _size;

	public:
		InstructionBuilder();
		InstructionBuilder(const std::vector<Instruction>& insts);
		InstructionBuilder(std::vector<Instruction>&& insts);

		Location push_front(const Instruction& inst);
		Location push_front(Instruction&& inst);

		Location push_back(const Instruction& inst);
		Location push_back(Instruction&& inst);

		Location insert(Location position, const Instruction& inst);
		Location insert(Location position, Instruction&& inst);

		Location insert_before(Location position, const Instruction& inst);
		Location insert_before(Location position, Instruction&& inst);

		Location insert_after(Location position, const Instruction& inst);
		Location insert_after(Location position, Instruction&& inst);

		void move_before(Location position, Location target);
		void move_after(Location position, Location target);
		void move(Location position, int delta);

		void erase(Location position);

		void swap(Location l0, Location l1);

		Size byte_count() const;

		inline Instruction& front() { return _head->_instruction; }
		inline const Instruction& front() const { return _head->_instruction; }

		inline Instruction& back() { return _tail->_instruction; }
		inline const Instruction& back() const { return _tail->_instruction; }

		inline Size size() const { return _size; }
		inline bool empty() const { return !_size; }

	public:
		Location push_front(InstructionBuilder&& builder);
		Location push_back(InstructionBuilder&& builder);
		Location insert(Location position, InstructionBuilder&& builder);
		Location insert_before(Location position, InstructionBuilder&& builder);
		Location insert_after(Location position, InstructionBuilder&& builder);

		inline Location push_front(const InstructionBuilder& builder) { return push_front(InstructionBuilder(builder)); }
		inline Location push_back(const InstructionBuilder& builder) { return push_back(InstructionBuilder(builder)); }
		inline Location insert(Location position, const InstructionBuilder& builder) { return insert(position, InstructionBuilder(builder)); }
		inline Location insert_before(Location position, const InstructionBuilder& builder) { return insert_before(position, InstructionBuilder(builder)); }
		inline Location insert_after(Location position, const InstructionBuilder& builder) { return insert_after(position, InstructionBuilder(builder)); }

	public:
		inline Location push_front(const std::vector<Instruction>& insts) { return push_front(InstructionBuilder(insts)); }
		inline Location push_front(std::vector<Instruction>&& insts) { return push_front(InstructionBuilder(std::move(insts))); }

		inline Location push_back(const std::vector<Instruction>& insts) { return push_back(InstructionBuilder(insts)); }
		inline Location push_back(std::vector<Instruction>&& insts) { return push_back(InstructionBuilder(std::move(insts))); }

		inline Location insert(Location position, const std::vector<Instruction>& insts) { return insert(position, InstructionBuilder(insts)); }
		inline Location insert(Location position, std::vector<Instruction>&& insts) { return insert(position, InstructionBuilder(std::move(insts))); }

		inline Location insert_before(Location position, const std::vector<Instruction>& insts) { return insert_before(position, InstructionBuilder(insts)); }
		inline Location insert_before(Location position, std::vector<Instruction>&& insts) { return insert_before(position, InstructionBuilder(std::move(insts))); }

		inline Location insert_after(Location position, const std::vector<Instruction>& insts) { return insert_after(position, InstructionBuilder(insts)); }
		inline Location insert_after(Location position, std::vector<Instruction>&& insts) { return insert_after(position, InstructionBuilder(std::move(insts))); }

	private:
		void _destroy();
		InstructionBuilder& _copy(const InstructionBuilder& ib, bool reset = true);
		InstructionBuilder& _move(InstructionBuilder&& ib, bool reset = true) noexcept;

		Node* _push_front(Node* newnode);
		Node* _push_back(Node* newnode);
		Node* _insert(Node* next, Node* newnode);

	public:
		inline InstructionBuilder(const InstructionBuilder& ib) :
			InstructionBuilder{}
		{
			_copy(ib, false);
		}
		inline InstructionBuilder(InstructionBuilder&& ib) noexcept :
			InstructionBuilder{}
		{
			_move(std::move(ib), false);
		}
		inline ~InstructionBuilder() { _destroy(); }

		inline InstructionBuilder& operator= (const InstructionBuilder& right) { return _copy(right, true); }
		inline InstructionBuilder& operator= (InstructionBuilder&& right) noexcept { return _move(std::move(right), true); }

	public:
		void build(void* buffer, Size buffer_size) const;
		void build(std::ostream& os) const;

		friend inline std::ostream& operator<< (std::ostream& os, const InstructionBuilder& ib) { return ib.build(os), os; }
	};
}

namespace kram::op::build
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
		Static
	};

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
		inline Value& operator= (_Ty data) { _data = _adapt<_Ty>(data), _type = _detect_type<_Ty>(data); }

		inline DataSize bytes() const
		{
			if (_data > 0xFFFFFFFFULL) return DataSize::QuadWord;
			else if (_data > 0xFFFFULL) return DataSize::DoubleWord;
			else if (_data > 0xFFULL) return DataSize::Word;
			else return DataSize::Byte;
		}

		inline DataType type() const { return _type; }

		inline operator UInt64() const { return _data; }
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
		inline UnsignedInteger& operator= (_Ty value) { _value = _adapt<_Ty>(value); }

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
	};

	struct MemoryLocation
	{
		Segment segment;
		bool enabledSplitRegister;
		Register splitRegister;
		DataSize splitMode;
		UnsignedInteger delta;
	};

	inline MemoryLocation location(Segment segment, bool enabledSplitRegister, Register splitRegister, DataSize splitMode, const UnsignedInteger& delta)
	{
		return MemoryLocation{
			.segment = segment,
			.enabledSplitRegister = enabledSplitRegister,
			.splitRegister = splitRegister,
			.splitMode = splitMode,
			.delta = delta
		};
	}
	inline MemoryLocation location(Segment segment, Register splitRegister, DataSize splitMode = DataSize::Byte, const UnsignedInteger& delta = {})
	{
		return MemoryLocation{
			.segment = segment,
			.enabledSplitRegister = true,
			.splitRegister = splitRegister,
			.splitMode = splitMode,
			.delta = delta
		};
	}
	inline MemoryLocation location(Segment segment, const UnsignedInteger& delta = {})
	{
		return MemoryLocation{
			.segment = segment,
			.enabledSplitRegister = false,
			.splitRegister = Register::r0,
			.splitMode = DataSize::Byte,
			.delta = delta
		};
	}
	inline MemoryLocation location(const UnsignedInteger& address)
	{
		return MemoryLocation{
			.segment = Segment::NoSegment,
			.enabledSplitRegister = false,
			.splitRegister = Register::r0,
			.splitMode = DataSize::Byte,
			.delta = address
		};
	}



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
