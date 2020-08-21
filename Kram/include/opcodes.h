#pragma once

#include "common.h"

namespace kram::op
{
	enum class Opcode : UInt8
	{
		NOP = 0x00,

		MOV,
		MOVX,
		MOVEX,
		MOVRX, /* [size:2|dest_mode:3|src_mode:3], [dest:8/16/32/64], ?[dest_delta:8/16/32/64], [src:8/16/32/64], ?[src_delta:8/16/32/64]
				* Move memory from source to destination
				* Diferences:
				*		MOV   -> 1 byte per argument
				*		MOVX  -> 2 bytes per argument
				*		MOVEX -> 4 bytes per argument
				*		MOVRX -> 8 bytes per argument
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

		MMB, /* [arg_len:2|dest_mode:3|src_mode:3], [block_size:8/16/32/64], [dest:8/16/32/64], ?[dest_delta:8/16/32/64], [src:8/16/32/64], ?[src_delta:8/16/32/64]
			  * Move memory block from source to destination
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: Stack data location
			  *		1: Stack data location + delta
			  *		2: Static data location
			  *		3: Static data location + delta
			  *		4: Location at register
			  *		5: Location at register + delta
			  */

		LAS, /* [arg_len:2|size:2|src_mode:3|<ignore>:1], [dest:8/16/32/64], [src:8/16/32/64], ?[src_delta:8/16/32/64]
			  * Load argument to stack
			  * Argument lengths:
			  *		0: 1 byte (8 bits)
			  *		1: 2 bytes (16 bits)
			  *		2: 4 bytes (32 bits)
			  *		3: 8 bytes (64 bits)
			  * Avaliable modes:
			  *		0: Register
			  *		1: Immediate value
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

		LEA, /* [arg_len:2|mode:2|<ignore>:4], [register_dest:8], [source:8/16/32/64]
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
		struct Node
		{
			Instruction instruction;
			Node* next;
			Node* prev;
		};

	public:
		typedef Node* Location;

	private:
		Node* _head;
		Node* _tail;
		Size _size;

	public:
		InstructionBuilder();

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

		inline Instruction& front() { return _head->instruction; }
		inline const Instruction& front() const { return _head->instruction; }

		inline Instruction& back() { return _tail->instruction; }
		inline const Instruction& back() const { return _tail->instruction; }

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
