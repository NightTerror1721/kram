#pragma once

#include "common.h"

namespace kram::op
{
	enum class Opcode : UInt8
	{
		NOP = 0x00,

		MOVB,
		MOVW,
		MOVL,
		MOVQ, /* [size:2|dest_mode:3|src_mode:3], [dest:8/16/32/64], ?[dest_delta:8/16/32/64], [src:8/16/32/64], ?[src_delta:8/16/32/64]
		       * Move memory from source to destination
			   * Diferences:
			   *	MOV   -> 1 byte per argument
			   *	MOVX  -> 2 bytes per argument
			   *	MOVEX -> 4 bytes per argument
			   *	MOVRX -> 8 bytes per argument
			   * Avaliable modes:
			   *	0: Register
			   *	1: Immediate value (only for source)
			   *	2: Stack data location
			   *	3: Stack data location + delta
			   *	4: Static data location
			   *	5: Static data location + delta
			   *	6: Location at register
			   *	7: Location at register + delta
			   * Available Sizes:
			   *	0: Byte (8 bits)
			   *	1: Word (16 bits)
			   *	2: DWord (32 bits)
			   *	3: QWord (64 bits)
			   */

		MMB, /* [arg_len:2|dest_mode:3|src_mode:3], [block_size:8/16/32/64], [dest:8/16/32/64], ?[dest_delta:8/16/32/64], [src:8/16/32/64], ?[src_delta:8/16/32/64]
			  * Move memory block from source to destination
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: <not used>
			  *		1: <not used>
			  *		2: Stack data location
			  *		3: Stack data location + delta
			  *		4: Static data location
			  *		5: Static data location + delta
			  *		6: Location at register
			  *		7: Location at register + delta
			  */

		LEA, /* [arg_len:2|mode:2|<ignore>:4], [register_dest:8], [source:8/16/32/64], ?[source_delta:8/16/32/64]
			  * Load efective addres (from stack data or static data) to register_dest
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Available modes:
			  *		0: Stack data
			  *		2: Stack data + delta
			  *		3: Static data
			  *		4: Static data + delta
			  */

		NEW, /* [arg_len:2|dst_mode:3|add_ref:1|<ignore>:2], [dst:8/16/32/64], ?[dst_delta:8/16/32/64], [size:8/16/32/64]
			  * Allocate memory block with "size" bytes and store address into "dst"
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: Register
			  *		1: <not used>
			  *		2: Stack data location
			  *		3: Stack data location + delta
			  *		4: Static data location
			  *		5: Static data location + delta
			  *		6: Location at register
			  *		7: Location at register + delta
			  */

		DEL, /* [arg_len:2|src_mode:3|<ignore>:3], [src:8/16/32/64], ?[src_delta:8/16/32/64]
			  * Free memory block with address allocated in "src"
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: Register
			  *		1: <not used>
			  *		2: Stack data location
			  *		3: Stack data location + delta
			  *		4: Static data location
			  *		5: Static data location + delta
			  *		6: Location at register
			  *		7: Location at register + delta
			  */

		MHR, /* [arg_len:2|dst_mode:3|is_inc:1|<ignore>:2], [target:8/16/32/64], ?[target_delta:8/16/32/64]
			  * Modify (increase or decrease) heap block memory reference counter from "target"
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: Register
			  *		1: <not used>
			  *		2: Stack data location
			  *		3: Stack data location + delta
			  *		4: Static data location
			  *		5: Static data location + delta
			  *		6: Location at register
			  *		7: Location at register + delta
			  */

		CST, /* [dest_type:4|src_type:4], [register:8]
			  * Cast register value into new type
			  * Available types:
			  *		0: unsigned byte
			  *		1: unsigned word
			  *		2: unsigned dword
			  *		3: unsigned qword
			  *		4: signed byte
			  *		5: signed word
			  *		6: signed dword
			  *		7: signed qword
			  *		8: float
			  *		9: double
			  */

		ADD, /* [type:4|<ignore>:4], [dest_register:8], [src_register:8]
			  * Add src_register to dest_register and store result into dest_register
			  * Available types:
			  *		0: unsigned byte
			  *		1: unsigned word
			  *		2: unsigned dword
			  *		3: unsigned qword
			  *		4: signed byte
			  *		5: signed word
			  *		6: signed dword
			  *		7: signed qword
			  *		8: float
			  *		9: double
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

		friend std::ostream& operator<< (std::ostream& os, const Instruction& inst);
	};

	class InstructionBuilder
	{
	private:
		struct Node;

	private:
		typedef Node* Location;
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
	};
}

namespace kram::op::build
{
	enum class ArgumentMode { Byte, Word, DoubleWord, QuadWord };

	enum class DataSize { Byte, Word, DoubleWord, QuadWord };

	enum class DataType
	{
		SignedByte, SignedWord, SignedDoubleWord, SignedQuadWord,
		UnsignedByte, UnsignedWord, UnsignedDoubleWord, UnsignedQuadWord,
		FloatingDecimal, DoubleDecimal
	};

	enum class LocationMode
	{
		Register,
		Immediate,
		Stack,
		StackDelta,
		Static,
		StaticDelta,
		AddressInRegister,
		AddressInRegisterDelta
	};

	Instruction mov(
		DataSize dataSize,
		LocationMode destLoc,
		LocationMode srcLoc,
		UInt64 dest,
		UInt64 destDelta,
		UInt64 src,
		UInt64 srcDelta,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction mmb(
		LocationMode destLoc,
		LocationMode srcLoc,
		UInt64 blockSize,
		UInt64 dest,
		UInt64 destDelta,
		UInt64 src,
		UInt64 srcDelta,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction lea(
		bool srcStaticLoc,
		bool srcHasDelta,
		UInt8 destRegister,
		UInt64 src,
		UInt64 srcDelta,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction new_(
		LocationMode destLoc,
		bool addHeapRef,
		UInt64 dest,
		UInt64 destDelta,
		UInt64 blockSize,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction del(
		LocationMode srcLoc,
		UInt64 src,
		UInt64 srcDelta,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction mhr(
		LocationMode targetLoc,
		bool isIncrease,
		UInt64 target,
		UInt64 targetDelta,
		ArgumentMode argMode = scast(ArgumentMode, -1)
	);

	Instruction cst(
		DataType destType,
		DataType srcType,
		UInt8 registerTarget
	);
}
