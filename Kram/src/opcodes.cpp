#include "opcodes.h"

#include <tuple>

namespace kram::op
{
	Instruction::Instruction() :
		_opcode{ scast(Opcode, 0) },
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

	void Instruction::write(void* _buffer, Size buffer_size) const
	{
		Size remaining = std::min(buffer_size, byte_count());
		
		std::byte* buffer = rcast(std::byte*, _buffer);
		if (remaining - sizeof(Opcode) >= 0)
			*rcast(Opcode*, buffer) = _opcode;
		remaining -= sizeof(Opcode);

		for (std::byte byte : _args)
		{
			if (remaining-- <= 0)
				return;
			*buffer = byte;
		}
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

	Size InstructionBuilder::byte_count() const
	{
		Size count = 0;
		for (Node* node = _head; node; node = node->_next)
			count = node->_instruction.byte_count();

		return count;
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

	void InstructionBuilder::build(void* buffer, Size buffer_size) const
	{
		for (Node* node = _head; node; node = node->_next)
		{
			if (buffer_size <= 0)
				return;
			Size byte_count = node->_instruction.byte_count();
			node->_instruction.write(buffer, buffer_size);
			buffer_size = byte_count > buffer_size ? 0 : buffer_size - byte_count;
		}
	}
	void InstructionBuilder::build(std::ostream& os) const
	{
		for (Node* node = _head; node; node = node->_next)
			os << node->_instruction;
	}
}




namespace kram::op::build
{
	template<unsigned int _BitIdx, unsigned int _BitCount, typename _BaseTy, typename _ValueTy>
	constexpr UInt8 bits(_BaseTy base, _ValueTy value) { return utils::set_bits<_BitIdx, _BitCount, UInt8>(scast(UInt8, base), scast(UInt8, value)); }

	template<unsigned int _BitIdx, unsigned int _BitCount, typename _ValueTy>
	constexpr UInt8 bits(_ValueTy value) { return utils::set_bits<_BitIdx, _BitCount, UInt8>(0, scast(UInt8, value)); }

	UInt8 location_to_byte(const MemoryLocation& loc)
	{
		return bits<0, 2>(loc.segment) |
			bits<2, 1>(loc.enabledSplitRegister) |
			bits<3, 2>(loc.splitMode) |
			bits<5, 1>(!loc.delta.is_zero()) |
			bits<6, 2>(loc.delta.bytes());
	}

	template<typename _Ty> requires std::same_as<_Ty, UnsignedInteger> || std::same_as<_Ty, Value>
	Instruction& add_value(Instruction& inst, const _Ty & uint)
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

		inst.add_byte(location_to_byte(location));
		inst.add_byte(bits<0, 4>(mem_to_reg ? reg : location.splitRegister) | bits<4, 4>(mem_to_reg ? location.splitRegister : reg));

		if(location.delta)
			add_value(inst, location.delta);

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

		inst.add_byte(location_to_byte(dest));
		inst.add_byte(bits<0, 4>(dest.splitRegister));
		add_value(inst, immediateValue);

		if (dest.delta)
			add_value(inst, dest.delta);

		return inst;
	}

	Instruction lea(Register dest, const MemoryLocation& src)
	{
		Instruction inst;

		inst.opcode(Opcode::LEA);

		inst.add_byte(location_to_byte(src));
		inst.add_byte(bits<0, 4>(dest) | bits<4, 4>(src.splitRegister));

		if (src.delta)
			add_value(inst, src.delta);

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

		inst.add_byte(location_to_byte(dest));
		inst.add_byte(bits<0, 4>(dest.splitRegister) | bits<4, 2>(block_bytes.bytes()) | bits<6, 1>(add_ref));
		add_value(inst, block_bytes);

		if (dest.delta)
			add_value(inst, dest.delta);

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

		inst.add_byte(location_to_byte(src));
		inst.add_byte(bits<0, 4>(src.splitRegister));

		if (src.delta)
			add_value(inst, src.delta);

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

		inst.add_byte(location_to_byte(src));
		inst.add_byte(bits<0, 4>(src.splitRegister) | bits<4, 1>(increase));

		if (src.delta)
			add_value(inst, src.delta);

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

		inst.add_byte(location_to_byte(target));
		inst.add_byte(bits<0, 4>(target.splitRegister));
		inst.add_byte(bits<0, 4>(dest_type) | bits<4, 4>(src_type));

		if (target.delta)
			add_value(inst, target.delta);

		return inst;
	}
}
