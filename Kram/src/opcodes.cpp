#include "opcodes.h"

#include <tuple>

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

	InstructionBuilder::InstructionBuilder(const std::vector<Instruction>& insts) :
		InstructionBuilder{}
	{
		if (!insts.empty())
		{
			_size = insts.size();
			for (const auto& inst : insts)
			{
				Node* node = new Node();
				node->_instruction = inst;

				if (!_tail)
					_head = _tail = node;
				else
				{
					node->_prev = _tail;
					_tail->_next = node;
					_tail = node;
				}
			}
		}
	}

	InstructionBuilder::InstructionBuilder(std::vector<Instruction>&& insts) :
		InstructionBuilder{}
	{
		if (!insts.empty())
		{
			_size = insts.size();
			for (auto& inst : insts)
			{
				Node* node = new Node();
				node->_instruction = std::move(inst);

				if (!_tail)
					_head = _tail = node;
				else
				{
					node->_prev = _tail;
					_tail->_next = node;
					_tail = node;
				}
			}
		}
	}

	void InstructionBuilder::_destroy()
	{
		if (_head)
		{
			for (Node* node = _head, *next; node; node = next)
			{
				next = node->_next;
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
			for (const Node* node = ib._head; node; node = node->_next)
			{
				Node* newnode = new Node{ *node };
				if (first)
				{
					first = false;
					newnode->_next = newnode->_prev = nullptr;
					_head = _tail = newnode;
				}
				else
				{
					newnode->_next = nullptr;
					newnode->_prev = _tail;
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
			newnode->_next = newnode->_prev = nullptr;
		}
		else
		{
			newnode->_next = _head;
			newnode->_prev = nullptr;
			_head->_prev = newnode;
			_head = newnode;
		}
		return _size++, newnode;
	}
	InstructionBuilder::Node* InstructionBuilder::_push_back(Node* newnode)
	{
		if (!_tail)
		{
			_head = _tail = newnode;
			newnode->_next = newnode->_prev = nullptr;
		}
		else
		{
			newnode->_prev = _tail;
			newnode->_next = nullptr;
			_head->_next = newnode;
			_head = newnode;
		}
		return _size++, newnode;
	}
	InstructionBuilder::Node* InstructionBuilder::_insert(Node* next, Node* newnode)
	{
		newnode->_next = next;
		newnode->_prev = next->_prev;
		next->_prev->_next = newnode;
		next->_prev = newnode;
		return _size++, newnode;
	}

	InstructionBuilder::Location InstructionBuilder::push_front(const Instruction& inst)
	{
		Node* node = new Node();
		node->_instruction = inst;
		return _push_front(node);
	}
	InstructionBuilder::Location InstructionBuilder::push_front(Instruction&& inst)
	{
		Node* node = new Node();
		node->_instruction = std::move(inst);
		return _push_front(node);
	}

	InstructionBuilder::Location InstructionBuilder::push_back(const Instruction& inst)
	{
		Node* node = new Node();
		node->_instruction = inst;
		return _push_back(node);
	}
	InstructionBuilder::Location InstructionBuilder::push_back(Instruction&& inst)
	{
		Node* node = new Node();
		node->_instruction = std::move(inst);
		return _push_back(node);
	}

	InstructionBuilder::Location InstructionBuilder::insert(Location position, const Instruction& inst)
	{
		Node* node = new Node();
		node->_instruction = inst;
		if (position == _head)
			return _push_front(node);
		return _insert(position, node);
	}
	InstructionBuilder::Location InstructionBuilder::insert(Location position, Instruction&& inst)
	{
		Node* node = new Node();
		node->_instruction = std::move(inst);
		if (position == _head)
			return _push_front(node);
		return _insert(position, node);
	}

	InstructionBuilder::Location InstructionBuilder::insert_before(Location position, const Instruction& inst)
	{
		if (position == _head)
			return push_front(inst);
		return insert(position->_prev, inst);
	}
	InstructionBuilder::Location InstructionBuilder::insert_before(Location position, Instruction&& inst)
	{
		if (position == _head)
			return push_front(std::move(inst));
		return insert(position->_prev, std::move(inst));
	}

	InstructionBuilder::Location InstructionBuilder::insert_after(Location position, const Instruction& inst)
	{
		if (position == _tail)
			return push_back(inst);
		return insert(position->_next, inst);
	}
	InstructionBuilder::Location InstructionBuilder::insert_after(Location position, Instruction&& inst)
	{
		if (position == _tail)
			return push_back(std::move(inst));
		return insert(position->_next, std::move(inst));
	}

	void InstructionBuilder::move_before(Location position, Location target)
	{
		insert_before(target, std::move(position->_instruction));
		erase(position);
	}
	void InstructionBuilder::move_after(Location position, Location target)
	{
		insert_after(target, std::move(position->_instruction));
		erase(position);
	}
	void InstructionBuilder::move(Location position, int delta)
	{
		if (delta == 0 || !position)
			return;

		Node* node = position;
		if (delta > 0)
		{
			for (; node && delta; node = node->_next, delta--);
			if (!node)
				push_back(std::move(position->_instruction));
			else insert(node, std::move(position->_instruction));
		}
		else
		{
			for (; node && delta; node = node->_prev, delta++);
			if (!node)
				push_front(std::move(position->_instruction));
			else insert(node, std::move(position->_instruction));
		}

		erase(position);
	}

	void InstructionBuilder::erase(Location node)
	{
		if (_head == node)
		{
			_head = node->_next;
			if (!_head)
				_tail = nullptr;
			else _head->_prev = nullptr;
		}
		else if (_tail == node)
		{
			_tail = node->_prev;
			if (!_tail)
				_head = nullptr;
			else _tail->_next = nullptr;
		}
		else
		{
			node->_prev->_next = node->_next;
			node->_next->_prev = node->_prev;
		}

		delete node;
		_size--;
	}

	void InstructionBuilder::swap(Location l0, Location l1)
	{
		Instruction aux = std::move(l0->_instruction);
		l0->_instruction = std::move(l1->_instruction);
		l1->_instruction = std::move(aux);
	}


	InstructionBuilder::Location InstructionBuilder::push_front(InstructionBuilder&& builder)
	{
		if (!builder._head)
			return _tail;

		if (!_head)
			utils::move(*this, std::move(builder));
		else
		{
			Node* oldhead = _head;
			builder._tail->_next = _head;
			_head->_prev = builder._tail;
			_head = builder._head;
			_size += builder._size;

			builder._head = builder._tail = nullptr;
			builder._size = 0;

			return oldhead->_prev;
		}

		return _tail;
	}

	InstructionBuilder::Location InstructionBuilder::push_back(InstructionBuilder&& builder)
	{
		if (!builder._head)
			return _tail;

		if (!_head)
			utils::move(*this, std::move(builder));
		else
		{
			builder._head->_prev = _tail;
			_tail->_next = builder._head;
			_tail = builder._tail;
			_size += builder._size;

			builder._head = builder._tail = nullptr;
			builder._size = 0;
		}

		return _tail;
	}

	InstructionBuilder::Location InstructionBuilder::insert(Location position, InstructionBuilder&& builder)
	{
		if (!builder._head)
			return _tail;

		if (position == _head)
			return push_front(std::move(builder));

		builder._head->_prev = position->_prev;
		builder._tail->_next = position;
		position->_prev->_next = builder._head;
		position->_prev = builder._tail;
		_size += builder._size;

		Node* ret = builder._tail;
		builder._head = builder._tail = nullptr;
		builder._size = 0;

		return ret;
	}

	InstructionBuilder::Location InstructionBuilder::insert_before(Location position, InstructionBuilder&& builder)
	{
		if (!position->_prev || position->_prev == _head)
			return push_front(std::move(builder));
		return insert(position->_prev, std::move(builder));
	}

	InstructionBuilder::Location InstructionBuilder::insert_after(Location position, InstructionBuilder&& builder)
	{
		if (!position->_next || position->_next == _tail)
			return push_back(std::move(builder));
		return insert(position->_next, std::move(builder));
	}
}




namespace kram::op::build
{
	constexpr bool is_auto_argmode(ArgumentMode mode) { return mode < ArgumentMode::Byte || mode > ArgumentMode::QuadWord; }

	static inline ArgumentMode detect_argmode(const std::vector<UInt64>& args)
	{
		ArgumentMode mode = ArgumentMode::Byte;
		for (UInt64 arg : args)
		{
			switch (mode)
			{
				case ArgumentMode::Byte:
					if (arg > 0xffffffffU)
						mode = ArgumentMode::QuadWord;
					else if (arg > 0xffffU)
						mode = ArgumentMode::DoubleWord;
					else if (arg > 0xffU)
						mode = ArgumentMode::Word;
					break;

				case ArgumentMode::Word:
					if (arg > 0xffffffffU)
						mode = ArgumentMode::QuadWord;
					else if (arg > 0xffffU)
						mode = ArgumentMode::DoubleWord;
					break;

				case ArgumentMode::DoubleWord:
					if (arg > 0xffffffffU)
						mode = ArgumentMode::QuadWord;
					break;
			}

			if (mode == ArgumentMode::QuadWord)
				break;
		}

		return mode;
	}

	static inline Instruction& push_arg(Instruction& inst, DataSize size, UInt64 arg)
	{
		switch (size)
		{
			case DataSize::Byte: inst.add_byte(scast(UInt8, arg)); break;
			case DataSize::Word: inst.add_word(scast(UInt16, arg)); break;
			case DataSize::DoubleWord: inst.add_dword(scast(UInt32, arg)); break;
			case DataSize::QuadWord: inst.add_qword(arg); break;
		}
		return inst;
	}

	constexpr bool has_delta(LocationMode locMode)
	{
		switch (locMode)
		{
			case LocationMode::StackDelta:
			case LocationMode::StaticDelta:
			case LocationMode::AddressInRegisterDelta:
				return true;
		}
		return false;
	}

	Instruction mov(
		DataSize dataSize,
		LocationMode destLoc,
		LocationMode srcLoc,
		UInt64 dest,
		UInt64 destDelta,
		UInt64 src,
		UInt64 srcDelta,
		ArgumentMode argMode
	)
	{
		Instruction inst;

		if (is_auto_argmode(argMode))
			argMode = detect_argmode({ dest, destDelta, src, srcDelta });

		UInt8 pars = 0;
		pars = utils::set_bits<0, 2>(pars, scast(UInt8, dataSize));
		pars = utils::set_bits<2, 3>(pars, scast(UInt8, destLoc));
		pars = utils::set_bits<5, 3>(pars, scast(UInt8, srcLoc));
		inst.add_byte(pars);

		push_arg(inst, dataSize, dest);
		if(has_delta(destLoc))
			push_arg(inst, dataSize, destDelta);

		push_arg(inst, dataSize, src);
		if (has_delta(srcLoc))
			push_arg(inst, dataSize, srcDelta);

		return inst;
	}
}
