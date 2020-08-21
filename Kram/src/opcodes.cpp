#include "opcodes.h"

namespace kram::op
{
	Instruction::Instruction() :
		_opcode{ Opcode::NOP },
		_args{}
	{}

	Instruction::Instruction(Opcode opcode, const std::vector<std::byte>& args) :
		_opcode{ opcode },
		_args{ args }
	{}

	bool Instruction::operator== (const Instruction& right) const { return _opcode == right._opcode && _args == right._args; }
	bool Instruction::operator!= (const Instruction& right) const { return _opcode != right._opcode || _args != right._args; }

	Instruction& Instruction::add_word(UInt16 value)
	{
		UInt8* ptr = reinterpret_cast<UInt8*>(&value);
		return add_byte(ptr[0]).add_byte(ptr[1]);
	}
	Instruction& Instruction::add_dword(UInt32 value)
	{
		UInt8* ptr = reinterpret_cast<UInt8*>(&value);
		return add_byte(ptr[0]).add_byte(ptr[1]).add_byte(ptr[2]).add_byte(ptr[3]);
	}
	Instruction& Instruction::add_qword(UInt64 value)
	{
		UInt8* ptr = reinterpret_cast<UInt8*>(&value);
		return add_byte(ptr[0]).add_byte(ptr[1]).add_byte(ptr[2]).add_byte(ptr[3])
			.add_byte(ptr[4]).add_byte(ptr[5]).add_byte(ptr[6]).add_byte(ptr[7]);
	}

	Instruction& Instruction::add_sword(Int16 value)
	{
		Int8* ptr = reinterpret_cast<Int8*>(&value);
		return add_sbyte(ptr[0]).add_sbyte(ptr[1]);
	}
	Instruction& Instruction::add_sdword(Int32 value)
	{
		Int8* ptr = reinterpret_cast<Int8*>(&value);
		return add_sbyte(ptr[0]).add_sbyte(ptr[1]).add_sbyte(ptr[2]).add_sbyte(ptr[3]);
	}
	Instruction& Instruction::add_sqword(Int64 value)
	{
		Int8* ptr = reinterpret_cast<Int8*>(&value);
		return add_sbyte(ptr[0]).add_sbyte(ptr[1]).add_sbyte(ptr[2]).add_sbyte(ptr[3])
			.add_sbyte(ptr[4]).add_sbyte(ptr[5]).add_sbyte(ptr[6]).add_sbyte(ptr[7]);
	}

	std::ostream& operator<< (std::ostream& os, const Instruction& inst)
	{
		os << static_cast<UInt8>(inst._opcode);
		if (!inst._args.empty())
			return os.write(reinterpret_cast<const char*>(inst._args.data()), inst._args.size());
		return os;
	}
}

namespace kram::op
{
	InstructionBuilder::InstructionBuilder() :
		_head{ nullptr },
		_tail{ nullptr },
		_size{ 0 }
	{}

	void InstructionBuilder::_destroy()
	{
		if (_head)
		{
			for (Node* node = _head, *next; node; node = next)
			{
				next = node->next;
				delete node;
			}
			_head = _tail = nullptr;
			_size = 0;
		}
	}
	InstructionBuilder& InstructionBuilder::_copy(const InstructionBuilder& ib, bool reset)
	{
		if (reset)
			_destroy();

		if (ib._head)
		{
			bool first = true;
			for (const Node* node = ib._head; node; node = node->next)
			{
				Node* newnode = new Node{ *node };
				if (first)
				{
					first = false;
					newnode->next = newnode->prev = nullptr;
					_head = _tail = newnode;
				}
				else
				{
					newnode->next = nullptr;
					newnode->prev = _tail;
					_tail = newnode;
				}
			}
			_size = ib._size;
		}
		return *this;
	}
	InstructionBuilder& InstructionBuilder::_move(InstructionBuilder&& ib, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_head = ib._head;
		_tail = ib._tail;
		_size = ib._size;

		ib._head = ib._tail = nullptr;
		ib._size = 0;

		return *this;
	}

	InstructionBuilder::Node* InstructionBuilder::_push_front(Node* newnode)
	{
		if (!_head)
		{
			_head = _tail = newnode;
			newnode->next = newnode->prev = nullptr;
		}
		else
		{
			newnode->next = _head;
			newnode->prev = nullptr;
			_head->prev = newnode;
			_head = newnode;
		}
		return _size++, newnode;
	}
	InstructionBuilder::Node* InstructionBuilder::_push_back(Node* newnode)
	{
		if (!_tail)
		{
			_head = _tail = newnode;
			newnode->next = newnode->prev = nullptr;
		}
		else
		{
			newnode->prev = _tail;
			newnode->next = nullptr;
			_head->next = newnode;
			_head = newnode;
		}
		return _size++, newnode;
	}
	InstructionBuilder::Node* InstructionBuilder::_insert(Node* next, Node* newnode)
	{
		newnode->next = next;
		newnode->prev = next->prev;
		next->prev->next = newnode;
		next->prev = newnode;
		return _size++, newnode;
	}

	InstructionBuilder::Location InstructionBuilder::push_front(const Instruction& inst)
	{
		Node* node = new Node();
		node->instruction = inst;
		return _push_front(node);
	}
	InstructionBuilder::Location InstructionBuilder::push_front(Instruction&& inst)
	{
		Node* node = new Node();
		node->instruction = std::move(inst);
		return _push_front(node);
	}

	InstructionBuilder::Location InstructionBuilder::push_back(const Instruction& inst)
	{
		Node* node = new Node();
		node->instruction = inst;
		return _push_back(node);
	}
	InstructionBuilder::Location InstructionBuilder::push_back(Instruction&& inst)
	{
		Node* node = new Node();
		node->instruction = std::move(inst);
		return _push_back(node);
	}

	InstructionBuilder::Location InstructionBuilder::insert(Location position, const Instruction& inst)
	{
		Node* node = new Node();
		node->instruction = inst;
		if (position == _head)
			return _push_front(node);
		return _insert(position, node);
	}
	InstructionBuilder::Location InstructionBuilder::insert(Location position, Instruction&& inst)
	{
		Node* node = new Node();
		node->instruction = std::move(inst);
		if (position == _head)
			return _push_front(node);
		return _insert(position, node);
	}

	InstructionBuilder::Location InstructionBuilder::insert_before(Location position, const Instruction& inst)
	{
		if (position == _head)
			return push_front(inst);
		return insert(position->prev, inst);
	}
	InstructionBuilder::Location InstructionBuilder::insert_before(Location position, Instruction&& inst)
	{
		if (position == _head)
			return push_front(std::move(inst));
		return insert(position->prev, std::move(inst));
	}

	InstructionBuilder::Location InstructionBuilder::insert_after(Location position, const Instruction& inst)
	{
		if (position == _tail)
			return push_back(inst);
		return insert(position->next, inst);
	}
	InstructionBuilder::Location InstructionBuilder::insert_after(Location position, Instruction&& inst)
	{
		if (position == _tail)
			return push_back(std::move(inst));
		return insert(position->next, std::move(inst));
	}
}
