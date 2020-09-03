#include "kram_asm_parser.h"

namespace kram::assembler::parser
{
	std::string opcode_as_string(AssemblerOpcode opcode)
	{
		return "";
	}

	std::string datasize_as_string(DataSize size)
	{
		switch (size)
		{
			case DataSize::Byte: return "b";
			case DataSize::Word: return "w";
			case DataSize::DoubleWord: return "d";
			case DataSize::QuadWord: return "q";
			default: return "";
		}
	}

	std::string to_hex_value(UInt64 value)
	{
		std::stringstream ss;
		ss << std::hex << value << 'h';
		return ss.str();
	}

	Element::Element(ElementType type) :
		_type{ type },
		_raw{}
	{
		switch (type)
		{
			case ElementType::Token:
				utils::construct(_data.token);
				break;

			case ElementType::Section:
				utils::construct(_data.section);
				break;

			case ElementType::Opcode:
				_data.opcode = AssemblerOpcode::NOP;
				break;

			case ElementType::Tag:
				utils::construct(_data.tag);
				break;

			case ElementType::DataType:
				_data.dataType = DataType::UByte;
				break;

			case ElementType::ImmediateValue:
				_data.immediate = 0;
				break;

			case ElementType::RegisterValue:
				_data.registerValue = { DataSize::Byte, 0 };
				break;

			case ElementType::StackValue:
				_data.stackValue = { DataSize::Byte, 0, 0 };
				break;

			case ElementType::StaticValue:
				_data.staticValue = { DataSize::Byte, 0, 0 };
				break;

			case ElementType::ValueFromRegisterAddress:
				_data.registerAddr = { DataSize::Byte, 0, 0 };
				break;
		}
	}
	void Element::_destroy()
	{
		switch (_type)
		{
			case ElementType::Token:
				utils::destroy(_data.token);
				break;

			case ElementType::Section:
				utils::destroy(_data.section);
				break;

			case ElementType::Tag:
				utils::destroy(_data.tag);
				break;

			case ElementType::RegisterValue:
				utils::destroy(_data.registerValue);
				break;

			case ElementType::StackValue:
				utils::destroy(_data.stackValue);
				break;

			case ElementType::StaticValue:
				utils::destroy(_data.staticValue);
				break;

			case ElementType::ValueFromRegisterAddress:
				utils::destroy(_data.registerAddr);
				break;
		}
		_type = ElementType::Invalid;
	}

	Element& Element::_copy(const Element& elem, bool reset)
	{
		if (reset)
			_destroy();

		_type = elem._type;
		switch (_type)
		{
			case ElementType::Token:
				utils::copy(_data.token, elem._data.token);
				break;

			case ElementType::Section:
				utils::copy(_data.section, elem._data.section);
				break;

			case ElementType::Opcode:
				_data.opcode = elem._data.opcode;
				break;

			case ElementType::Tag:
				utils::copy(_data.tag, elem._data.tag);
				break;

			case ElementType::DataType:
				_data.dataType = elem._data.dataType;
				break;

			case ElementType::ImmediateValue:
				_data.immediate = elem._data.immediate;
				break;

			case ElementType::RegisterValue:
				_data.registerValue = elem._data.registerValue;
				break;

			case ElementType::StackValue:
				_data.stackValue = elem._data.stackValue;
				break;

			case ElementType::StaticValue:
				_data.staticValue = elem._data.staticValue;
				break;

			case ElementType::ValueFromRegisterAddress:
				_data.registerAddr = elem._data.registerAddr;
				break;
		}

		return *this;
	}
	Element& Element::_move(Element&& elem, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_type = elem._type;
		switch (_type)
		{
			case ElementType::Token:
				utils::move(_data.token, std::move(elem._data.token));
				break;

			case ElementType::Section:
				utils::move(_data.section, std::move(elem._data.section));
				break;

			case ElementType::Opcode:
				_data.opcode = std::move(elem._data.opcode);
				break;

			case ElementType::Tag:
				utils::move(_data.tag, std::move(elem._data.tag));
				break;

			case ElementType::DataType:
				_data.dataType = std::move(elem._data.dataType);
				break;

			case ElementType::ImmediateValue:
				_data.immediate = std::move(elem._data.immediate);
				break;

			case ElementType::RegisterValue:
				_data.registerValue = std::move(elem._data.registerValue);
				break;

			case ElementType::StackValue:
				_data.stackValue = std::move(elem._data.stackValue);
				break;

			case ElementType::StaticValue:
				_data.staticValue = std::move(elem._data.staticValue);
				break;

			case ElementType::ValueFromRegisterAddress:
				_data.registerAddr = std::move(elem._data.registerAddr);
				break;
		}

		return elem._type = ElementType::Invalid, *this;
	}

	bool Element::operator== (const Element& right) const
	{
		if (_type != right._type)
			return false;

		switch (_type)
		{
			case ElementType::Token:
				return _data.token == right._data.token;

			case ElementType::Section:
				return _data.section == right._data.section;

			case ElementType::Opcode:
				return _data.opcode == right._data.opcode;

			case ElementType::Tag:
				return _data.tag == right._data.tag;

			case ElementType::DataType:
				return _data.dataType == right._data.dataType;

			case ElementType::ImmediateValue:
				return _data.immediate == right._data.immediate;

			case ElementType::RegisterValue:
				return _data.registerValue.registerId == right._data.registerValue.registerId &&
					_data.registerValue.size == right._data.registerValue.size;

			case ElementType::StackValue:
				return _data.stackValue.address == right._data.stackValue.address &&
					_data.stackValue.size == right._data.stackValue.size &&
					_data.stackValue.delta == right._data.stackValue.delta;

			case ElementType::StaticValue:
				return _data.staticValue.address == right._data.staticValue.address &&
					_data.staticValue.size == right._data.staticValue.size &&
					_data.staticValue.delta == right._data.staticValue.delta;

			case ElementType::ValueFromRegisterAddress:
				return _data.registerAddr.registerId == right._data.registerAddr.registerId &&
					_data.registerAddr.size == right._data.registerAddr.size &&
					_data.registerAddr.delta == right._data.registerAddr.delta;

			default:
				return true;
		}
	}

	std::string Element::to_string() const
	{
		switch (_type)
		{
			case ElementType::Token:
				return _data.token;

			case ElementType::Comma:
				return ",";

			case ElementType::Semicolon:
				return ";";

			case ElementType::Section:
				return _data.section;

			case ElementType::Opcode:
				return opcode_as_string(_data.opcode);

			case ElementType::Tag:
				return _data.tag;

			case ElementType::DataType:
				switch (_data.dataType)
				{
					case DataType::UByte: return "ub";
					case DataType::UWord: return "uw";
					case DataType::UDoubleWord: return "udw";
					case DataType::UQuadWord: return "uqw";
					case DataType::SByte: return "sb";
					case DataType::SWord: return "sw";
					case DataType::SDoubleWord: return "sdw";
					case DataType::SQuadWord: return "sqw";
					case DataType::FloatingDecimal: return "fd";
					case DataType::DoubleDecimal: return "dfd";
					default: return "<unknown-type>";
				}

			case ElementType::ImmediateValue:
				return to_hex_value(_data.immediate);

			case ElementType::RegisterValue:
				return "$" + std::to_string(_data.registerValue.registerId) + datasize_as_string(_data.registerValue.size);

			case ElementType::StackValue:
				if(_data.stackValue.delta > 0)
					return "%" + std::to_string(_data.stackValue.address) + datasize_as_string(_data.stackValue.size) + "+" + to_hex_value(_data.stackValue.delta);
				return "%" + std::to_string(_data.stackValue.address) + datasize_as_string(_data.stackValue.size);

			case ElementType::StaticValue:
				if (_data.staticValue.delta > 0)
					return "&" + std::to_string(_data.staticValue.address) + datasize_as_string(_data.staticValue.size) + "+" + to_hex_value(_data.staticValue.delta);
				return "&" + std::to_string(_data.staticValue.address) + datasize_as_string(_data.staticValue.size);

			case ElementType::ValueFromRegisterAddress:
				if (_data.registerAddr.delta > 0)
					return "[$" + std::to_string(_data.registerAddr.registerId) + datasize_as_string(_data.registerAddr.size) + "+" + to_hex_value(_data.registerAddr.delta) + "]";
				return "[$" + std::to_string(_data.registerAddr.registerId) + datasize_as_string(_data.registerAddr.size) + "]";
		}
		return "";
	}

	Element Element::token(const std::string& token) { Element e{ ElementType::Token }; e._data.token = token; return e; }
	Element Element::section(const std::string& section) { Element e{ ElementType::Section }; e._data.section = section; return e; }
	Element Element::opcode(AssemblerOpcode opcode) { Element e{ ElementType::Opcode }; e._data.opcode = opcode; return e; }
	Element Element::tag(const std::string& tag) { Element e{ ElementType::Tag }; e._data.tag = tag; return e; }
	Element Element::dataType(DataType type) { Element e{ ElementType::DataType }; e._data.dataType = type; return e; }
	Element Element::immediate(UInt64 value) { Element e{ ElementType::ImmediateValue }; e._data.immediate = value; return e; }
	Element Element::registerValue(DataSize size, UInt8 registerId)
	{
		Element e{ ElementType::RegisterValue };
		e._data.registerValue = { size, registerId };
		return e;
	}
	Element Element::stackValue(DataSize size, UInt64 address, UInt64 delta)
	{
		Element e{ ElementType::RegisterValue };
		e._data.stackValue = { size, address, delta };
		return e;
	}
	Element Element::staticValue(DataSize size, UInt64 address, UInt64 delta)
	{
		Element e{ ElementType::RegisterValue };
		e._data.staticValue = { size, address, delta };
		return e;
	}
	Element Element::valueFromRegisterAddress(DataSize size, UInt8 registerId, UInt64 delta)
	{
		Element e{ ElementType::RegisterValue };
		e._data.registerAddr = { size, registerId, delta };
		return e;
	}
}

kram::assembler::parser::ElementArray operator+ (const kram::assembler::parser::Element& left, const kram::assembler::parser::Element& right)
{
	return static_cast<const kram::assembler::parser::ElementArray>(left).concat(right);
}
