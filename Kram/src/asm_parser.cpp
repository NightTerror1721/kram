#include "asm_parser.h"

namespace kram::assembler::parser
{
	constexpr const char* section_name(Section section)
	{
		switch (section)
		{
			case Section::Static: return ".static";
			case Section::Function: return ".function";
			case Section::Link: return ".link";
			default: return "<unknown-section>";
		}
	}

	template<typename _Ty>
	requires std::same_as<_Ty, Value> || std::same_as<_Ty, UnsignedInteger> || std::same_as<_Ty, UInt64>
	std::string to_hex_value(_Ty value)
	{
		std::stringstream ss;
		ss << std::hex << static_cast<UInt64>(value) << 'h';
		return ss.str();
	}

	constexpr const char* register_name(Register reg)
	{
		switch (reg)
		{
			case Register::r0: return "r0";
			case Register::r1: return "r1";
			case Register::r2: return "r2";
			case Register::r3: return "r3";
			case Register::r4: return "r4";
			case Register::r5: return "r5";
			case Register::r6: return "r6";
			case Register::r7: return "r7";
			case Register::r8: return "r8";
			case Register::sd: return "sd";
			case Register::sb: return "sb";
			case Register::sp: return "sp";
			case Register::sr: return "sr";
			case Register::ch: return "ch";
			case Register::st: return "st";
			case Register::ip: return "ip";
		}
		return "";
	}

	constexpr const char* datatype_name(DataType type)
	{
		switch (type)
		{
			case DataType::UnsignedByte: return "ub";
			case DataType::UnsignedWord: return "uw";
			case DataType::UnsignedDoubleWord: return "udw";
			case DataType::UnsignedQuadWord: return "uqw";
			case DataType::SignedByte: return "sb";
			case DataType::SignedWord: return "sw";
			case DataType::SignedDoubleWord: return "sdw";
			case DataType::SignedQuadWord: return "sqw";
			case DataType::FloatingDecimal: return "fd";
			case DataType::DoubleDecimal: return "dfd";
			default: return "<unknown-type>";
		}
	}

	std::string memloc_string(const MemoryLocation& loc)
	{
		std::stringstream ss;
		switch (loc.segment.id)
		{
			case Segment::NoSegment: ss << '['; break;
			case Segment::Stack: ss << "$["; break;
			case Segment::Static: ss << "%["; break;
			case Segment::Register: ss << register_name(loc.segment.reg) << "["; break;
		}

		if (loc.split.enabled)
		{
			ss << register_name(loc.split.reg);
			switch (loc.split.size)
			{
				case DataSize::Word: ss << "*2"; break;
				case DataSize::DoubleWord: ss << "*4"; break;
				case DataSize::QuadWord: ss << "*8"; break;
			}
		}

		if (!loc.delta.is_zero())
			ss << '+' << to_hex_value(loc.delta);
		ss << ']';

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
				_data.section = Section::Static;
				break;

			case ElementType::Opcode:
				_data.opcode = AssemblerOpcode::NOP;
				break;

			case ElementType::Tag:
				utils::construct(_data.tag);
				break;

			case ElementType::DataType:
				_data.dataType = DataType::UnsignedByte;
				break;

			case ElementType::String:
				utils::construct(_data.string);
				break;

			case ElementType::Number:
				utils::construct(_data.number);
				break;

			case ElementType::Register:
				_data.reg = Register::r0;
				break;

			case ElementType::MemoryLocation:
				utils::construct(_data.memloc);
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

			case ElementType::String:
				utils::destroy(_data.string);
				break;

			case ElementType::Number:
				utils::destroy(_data.number);
				break;

			case ElementType::MemoryLocation:
				utils::destroy(_data.memloc);
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
				_data.section = elem._data.section;
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

			case ElementType::String:
				utils::copy(_data.string, elem._data.string);
				break;

			case ElementType::Number:
				_data.number = elem._data.number;
				break;

			case ElementType::Register:
				_data.reg = elem._data.reg;
				break;

			case ElementType::MemoryLocation:
				_data.memloc = elem._data.memloc;
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
				_data.section = std::move(elem._data.section);
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

			case ElementType::String:
				utils::move(_data.string, std::move(elem._data.string));
				break;

			case ElementType::Number:
				_data.number = std::move(elem._data.number);
				break;

			case ElementType::Register:
				_data.reg = std::move(elem._data.reg);
				break;

			case ElementType::MemoryLocation:
				_data.memloc = std::move(elem._data.memloc);
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

			case ElementType::String:
				return _data.string == right._data.string;

			case ElementType::Number:
				return _data.number == right._data.number;

			case ElementType::Register:
				return _data.reg == right._data.reg;

			case ElementType::MemoryLocation:
				return _data.memloc == right._data.memloc;

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

			case ElementType::End:
				return ";";

			case ElementType::Section:
				return section_name(_data.section);

			case ElementType::Opcode:
				return asm_opcode_name(_data.opcode);

			case ElementType::Tag:
				return _data.tag;

			case ElementType::TagEnd:
				return ":";

			case ElementType::DataType:
				return datatype_name(_data.dataType);

			case ElementType::String:
				return _data.string;

			case ElementType::Number:
				return to_hex_value(_data.number);

			case ElementType::Register:
				return register_name(_data.reg);

			case ElementType::StackSegment:
				return "$";

			case ElementType::StaticSegment:
				return "%";

			case ElementType::SplitIndicator:
				return "*";

			case ElementType::DeltaSeparator:
				return "+";

			case ElementType::MemoryLocation:
				return memloc_string(_data.memloc);

			case ElementType::MemoryLocationBegin:
				return "[";
				
			case ElementType::MemoryLocationEnd:
				return "]";
		}
		return "";
	}

	Element Element::token(const std::string& token) { Element e{ ElementType::Token }; e._data.token = token; return e; }
	Element Element::section(Section section) { Element e{ ElementType::Section }; e._data.section = section; return e; }
	Element Element::opcode(AssemblerOpcode opcode) { Element e{ ElementType::Opcode }; e._data.opcode = opcode; return e; }
	Element Element::tag(const std::string& tag) { Element e{ ElementType::Tag }; e._data.tag = tag; return e; }
	Element Element::dataType(DataType type) { Element e{ ElementType::DataType }; e._data.dataType = type; return e; }
	Element Element::string(const std::string string) { Element e{ ElementType::String }; e._data.string = string; return e; }
	Element Element::number(const Value& number) { Element e{ ElementType::Number }; e._data.number = number; return e; }
	Element Element::registerId(Register reg) { Element e{ ElementType::Register }; e._data.reg = reg; return e; }
	Element Element::memoryLocation(const MemoryLocation& loc) { Element e{ ElementType::MemoryLocation }; e._data.memloc = loc; return e; }
}

kram::assembler::parser::ElementArray operator+ (const kram::assembler::parser::Element& left, const kram::assembler::parser::Element& right)
{
	return static_cast<const kram::assembler::parser::ElementArray>(left).concat(right);
}

namespace kram::assembler::parser::options
{
	enum
	{
		MemlocBeginIsNotElement = 0x1 << 0
	};

	constexpr bool has(int options, int option) { return (options & ~option) == option; }
}

namespace kram::assembler::parser::support
{
	template<typename _Ty>
	inline bool contains(const std::map<std::string, _Ty>& map, const std::string& token) { return map.find(token) != map.end(); }

	template<typename _Ty>
	inline _Ty get(const std::map<std::string, _Ty>& map, const std::string& token) { return map.at(token); }

	static const std::map<std::string, Section> Sections{
		{ section_name(Section::Static) , Section::Static },
		{ section_name(Section::Function) , Section::Function },
		{ section_name(Section::Link) , Section::Link }
	};

	static const std::map<std::string, Register> Registers{
		{ register_name(Register::r0), Register::r0 },
		{ register_name(Register::r1), Register::r1 },
		{ register_name(Register::r2), Register::r2 },
		{ register_name(Register::r3), Register::r3 },
		{ register_name(Register::r4), Register::r4 },
		{ register_name(Register::r5), Register::r5 },
		{ register_name(Register::r6), Register::r6 },
		{ register_name(Register::r7), Register::r7 },
		{ register_name(Register::r8), Register::r8 },
		{ register_name(Register::sd), Register::sd },
		{ register_name(Register::sb), Register::sb },
		{ register_name(Register::sp), Register::sp },
		{ register_name(Register::sr), Register::sr },
		{ register_name(Register::ch), Register::ch },
		{ register_name(Register::st), Register::st },
		{ register_name(Register::ip), Register::ip }
	};

	static const std::map<std::string, DataType> DataTypes{
		{ datatype_name(DataType::UnsignedByte), DataType::UnsignedByte },
		{ datatype_name(DataType::UnsignedWord), DataType::UnsignedWord },
		{ datatype_name(DataType::UnsignedDoubleWord), DataType::UnsignedDoubleWord },
		{ datatype_name(DataType::UnsignedQuadWord), DataType::UnsignedQuadWord },
		{ datatype_name(DataType::SignedByte), DataType::SignedByte },
		{ datatype_name(DataType::SignedWord), DataType::SignedWord },
		{ datatype_name(DataType::SignedDoubleWord), DataType::SignedDoubleWord },
		{ datatype_name(DataType::SignedQuadWord), DataType::SignedQuadWord },
		{ datatype_name(DataType::FloatingDecimal), DataType::FloatingDecimal },
		{ datatype_name(DataType::DoubleDecimal), DataType::DoubleDecimal },
	};



	static utils::CompilerError error(const utils::DataReader& reader, const char* msg) { return utils::CompilerError(msg, reader); }
	static utils::CompilerError error(const utils::DataReader& reader, const std::string& msg) { return utils::CompilerError(msg, reader); }

	Element decode_token(const std::string& token)
	{
		if (is_valid_asm_opcode(token))
			return Element::opcode(get_asm_opcode_by_name(token));

		if (support::contains(support::Registers, token))
			return Element::registerId(support::get(support::Registers, token));

		if (support::contains(support::DataTypes, token))
			return Element::dataType(support::get(support::DataTypes, token));

		return Element::token(token);
	}

	static std::string read_token(utils::DataReader& reader)
	{
		Offset offset = reader.offset();
		Element element = parse_element(reader);
		if (!element.isToken())
		{
			reader.offset(offset);
			throw error(reader, "Expected valid token. But found: '" + element.to_string() + "'.");
		}
		return element.token();
	}

	template<typename _Ty>
	concept ElementTypeOnly = std::same_as<ElementType, _Ty>;

	template<ElementTypeOnly... _Types>
	static Element read_element(utils::DataReader& reader, _Types... types)
	{
		std::set<ElementType> tset{ types... };
		Offset offset = reader.offset();
		Element element = parse_element(reader);
		if (tset.find(element.type()) == tset.end())
		{
			reader.offset(offset);
			throw error(reader, "Unexpected element: '" + element.to_string() + "'.");
		}
		return element;
	}

	static Section read_section(utils::DataReader& reader)
	{
		std::string token = read_token(reader);
		if (!contains(Sections, token))
			throw support::error(reader, "Invalid section: " + token + ".");
		return get(Sections, token);
	}

	static std::string read_string(utils::DataReader& reader, bool single)
	{
		const char end = single ? '\'' : '\"';
		std::stringstream ss;
		while (reader)
		{
			char c = reader.next();
			if (c == end)
				return ss.str();

			switch (c)
			{
				case utils::DataReader::invalid_char:
				case utils::DataReader::newline:
					throw error(reader, "Invalid character in string: '" + utils::to_string(c) + "'.");

				case '\\': switch (c = reader.next()) {
					case '0': ss << '\0'; break;
					case 'n': ss << '\n'; break;
					case 't': ss << '\t'; break;
					case 'r': ss << '\r'; break;
					case '\'': ss << '\''; break;
					case '\"': ss << '\"'; break;
					case '\\': ss << '\\'; break;
					case 'a': {
						if (!reader.has(2))
							throw error(reader, "Invalid character in string: '" + utils::to_string(c) + "'.");

						char u1 = reader.next();
						char u0 = reader.next();
						char character = utils::hex_unit_to_integer<char>(u1, 1) | utils::hex_unit_to_integer<char>(u0, 0);
						ss << character;
					} break;

					default:
						throw error(reader, "Invalid character in string: '" + utils::to_string(c) + "'.");
				} break;

				default:
					ss << c;
					break;
			}
		}

		throw error(reader, "Malformed end of string.");
	}

	MemoryLocation read_memloc(utils::DataReader& reader)
	{
		MemoryLocation memloc;
		memloc.segment = Segment::NoSegment;

		bool bsplit = false, bdelta = false, bend = false;
		while (bend)
		{
			Element element = read_element(reader, ElementType::Register, ElementType::DeltaSeparator, ElementType::Number, ElementType::MemoryLocationEnd);

			switch_part:
			switch (element.type())
			{
				case ElementType::Register:
					if (bsplit)
						throw error(reader, "Cannot exists two or more split parts in memory location. Only one.");
					bsplit = true;

					memloc.split.enabled = true;
					memloc.split.reg = element.registerId();
					element = read_element(reader, ElementType::SplitIndicator, ElementType::DeltaSeparator, ElementType::MemoryLocationEnd);
					if (element.isSplitIndicator())
					{
						element = read_element(reader, ElementType::Number);
						UInt64 value = element.number();
						switch (value)
						{
							case 1: memloc.split.size = DataSize::Byte; break;
							case 2: memloc.split.size = DataSize::Word; break;
							case 4: memloc.split.size = DataSize::DoubleWord; break;
							case 8: memloc.split.size = DataSize::QuadWord; break;
							default:
								throw error(reader, "Split is only valid in 1, 2, 4 or 8 byte blocks.");
						}
					}
					else goto switch_part;
					break;

				case ElementType::DeltaSeparator:
					bdelta = true;
					element = read_element(reader, ElementType::Number);
					memloc.delta = static_cast<UInt64>(memloc.delta) + static_cast<UInt64>(element.number());
					break;

				case ElementType::Number:
					if (bdelta || bsplit)
						throw error(reader, "Invalid position to delta displacement in memory location.");

					element = read_element(reader, ElementType::Number);
					memloc.delta = static_cast<UInt64>(memloc.delta) + static_cast<UInt64>(element.number());
					break;

				case ElementType::MemoryLocationEnd:
					bend = true;
					break;
			}
		}

		return memloc;
	}

	Element read_memloc_or_predict(utils::DataReader& reader, SegmentData segment)
	{
		Offset offset = reader.offset();
		Element element = parse_element(reader, options::MemlocBeginIsNotElement);
		if (element.isMemoryLocationBegin())
		{
			MemoryLocation memloc = read_memloc(reader);
			memloc.segment = segment;
			return Element::memoryLocation(memloc);
		}
		reader.offset(offset);

		switch (segment.id)
		{
			case Segment::NoSegment: return Element::memoryLocation({});
			case Segment::Stack: return Element::memoryLocation({ Segment::Stack, {}, {} });
			case Segment::Static: return Element::memoryLocation({ Segment::Static, {}, {} });
			case Segment::Register: return Element::registerId(segment.reg);
		}

		return Element::memoryLocation({});
	}
}


kram::assembler::parser::Element kram::assembler::parser::parse_element(utils::DataReader& reader, int options)
{
	#define dump_buffer(_Buf) if((_Buf).tellp() > 0) return reader.prev(), support::decode_token((_Buf).str())

	std::stringstream ss;
	while (reader)
	{
		char c = reader.next();
		switch (c)
		{
			case utils::DataReader::invalid_char:
				dump_buffer(ss);
				return {};

			case utils::DataReader::newline:
				dump_buffer(ss);
				return Element::end();

			case ';':
				dump_buffer(ss);
				reader.skip_line();
				return Element::end();

			case ',':
				dump_buffer(ss);
				return Element::comma();

			case '*':
				dump_buffer(ss);
				return Element::splitIndicator();

			case '+':
				dump_buffer(ss);
				return Element::deltaSeparator();

			case ':':
				dump_buffer(ss);
				return Element::tagEnd();

			case '[':
				dump_buffer(ss);
				if (options::has(options, options::MemlocBeginIsNotElement))
					return Element::memoryLocation(support::read_memloc(reader));
				return Element::memoryLocationBegin();

			case ']':
				dump_buffer(ss);
				return Element::memoryLocationEnd();

			case '\r':
				break;

			case ' ':
			case '\t':
				dump_buffer(ss);
				break;

			case '.':
				dump_buffer(ss);
				return Element::section(support::read_section(reader));

			case '\'':
			case '\"':
				dump_buffer(ss);
				return Element::string(support::read_string(reader, c == '\''));

			case '$':
				dump_buffer(ss);
				return support::read_memloc_or_predict(reader, Segment::Stack);

			case '%':
				dump_buffer(ss);
				return support::read_memloc_or_predict(reader, Segment::Static);

			default:
				ss << c;
		}
	}

	std::string str = ss.str();
	if (str.empty())
		return Element::end();
	return support::decode_token(str);

	#undef dump_buffer
}

kram::assembler::parser::ElementArray kram::assembler::parser::parse_line(utils::DataReader& reader)
{
	std::vector<Element> elems;
	while (reader)
	{
		Element elem = parse_element(reader);
		if (elem.isEnd())
			break;

		if (elem.isTagEnd() && !elems.empty() && elems.back().isToken())
			elems.back() = Element::tag(elems.back().token());
		else
		{
			elems.push_back(elem);
		}
	}

	return elems;
}

