#pragma once

#include "common.h"
#include "static_array.h"
#include "asm_common.h"
#include "iodata.h"
#include "cperrors.h"

namespace kram::assembler::parser
{
	enum class ElementType : unsigned int
	{
		Invalid = 0x0U,
		Token,
		Comma,
		End,
		Section,
		Opcode,
		Tag,
		TagEnd,
		DataType,
		String,
		Number,
		Register,
		StackSegment,
		StaticSegment,
		SplitIndicator,
		DeltaSeparator,
		MemoryLocation,
		MemoryLocationBegin,
		MemoryLocationEnd
	};

	enum class Section
	{
		Static,
		Function,
		Link
	};

	class Element
	{
	private:
		ElementType _type;
		union
		{
			union
			{
				std::string token;
				Section section;
				AssemblerOpcode opcode;
				std::string tag;
				DataType dataType;
				std::string string;
				Value number;
				Register reg;
				MemoryLocation memloc;
			} _data;
			std::byte _raw[sizeof(_data)];
		};

	private:
		Element(ElementType type);

	public:
		bool operator== (const Element& right) const;

		std::string to_string() const;

	public:
		static Element token(const std::string& token);
		static Element section(Section section);
		static Element opcode(AssemblerOpcode opcode);
		static Element tag(const std::string& tag);
		static Element dataType(DataType type);
		static Element string(const std::string string);
		static Element number(const Value& number);
		static Element registerId(Register reg);
		static Element memoryLocation(const MemoryLocation& loc);

		static inline Element comma() { return { ElementType::Comma }; }
		static inline Element end() { return { ElementType::End }; }
		static inline Element tagEnd() { return { ElementType::TagEnd }; }
		static inline Element stackSegment() { return { ElementType::StackSegment }; }
		static inline Element staticSegment() { return { ElementType::StaticSegment }; }
		static inline Element splitIndicator() { return { ElementType::SplitIndicator }; }
		static inline Element deltaSeparator() { return { ElementType::DeltaSeparator }; }
		static inline Element memoryLocationBegin() { return { ElementType::MemoryLocationBegin }; }
		static inline Element memoryLocationEnd() { return { ElementType::MemoryLocationEnd }; }

	public:
		inline const std::string& token() const { return _data.token; }
		inline Section section() const { return _data.section; }
		inline AssemblerOpcode opcode() const { return _data.opcode; }
		inline const std::string& tag() const { return _data.tag; }
		inline DataType dataType() const { return _data.dataType; }
		inline const std::string& string() const { return _data.string; }
		inline const Value& number() const { return _data.number; }
		inline Register registerId() const { return _data.reg; }
		inline const MemoryLocation& memoryLocation() const { return _data.memloc; }

		inline bool isInvalid() const { return _type == ElementType::Invalid; }
		inline bool isToken() const { return _type == ElementType::Token; }
		inline bool isComma() const { return _type == ElementType::Comma; }
		inline bool isEnd() const { return _type == ElementType::End; }
		inline bool isSection() const { return _type == ElementType::Section; }
		inline bool isOpcode() const { return _type == ElementType::Opcode; }
		inline bool isTag() const { return _type == ElementType::Tag; }
		inline bool isTagEnd() const { return _type == ElementType::TagEnd; }
		inline bool isDataType() const { return _type == ElementType::DataType; }
		inline bool isString() const { return _type == ElementType::String; }
		inline bool isNumber() const { return _type == ElementType::Number; }
		inline bool isRegisterId() const { return _type == ElementType::Register; }
		inline bool isStackSegment() const { return _type == ElementType::StackSegment; }
		inline bool isStaticSegment() const { return _type == ElementType::StaticSegment; }
		inline bool isSplitIndicator() const { return _type == ElementType::SplitIndicator; }
		inline bool isDeltaSeparator() const { return _type == ElementType::DeltaSeparator; }
		inline bool isMemoryLocation() const { return _type == ElementType::MemoryLocation; }
		inline bool isMemoryLocationBegin() const { return _type == ElementType::MemoryLocationBegin; }
		inline bool isMemoryLocationEnd() const { return _type == ElementType::MemoryLocationEnd; }

	private:
		void _destroy();
		Element& _copy(const Element& elem, bool reset);
		Element& _move(Element&& elem, bool reset) noexcept;

	public:
		inline Element() : Element(ElementType::Invalid) {}
		inline Element(const Element& e) : Element{} { _copy(e, false); }
		inline Element(Element&& e) noexcept : Element{} { _move(std::move(e), false); }
		inline ~Element() { _destroy(); }

		inline Element& operator= (const Element& right) { return _copy(right, true); }
		inline Element& operator= (Element&& right) noexcept { return _move(std::move(right), true); }

		inline bool operator!= (const Element& right) const { return  !(*this == right); }

		inline operator bool() const { return _type != ElementType::Invalid; }
		inline bool operator! () const { return _type == ElementType::Invalid; }

		inline ElementType type() const { return _type; }
	};



	typedef utils::StaticArray<Element> ElementArray;

	

	Element parse_element(utils::DataReader& reader, int options = 0);

	ElementArray parse_line(utils::DataReader& reader);
}

kram::assembler::parser::ElementArray operator+ (const kram::assembler::parser::Element& left, const kram::assembler::parser::Element& right);
