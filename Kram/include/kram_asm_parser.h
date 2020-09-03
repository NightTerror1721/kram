#pragma once

#include "common.h"
#include "static_array.h"

namespace kram::assembler::parser
{
	enum class ElementType : unsigned int
	{
		Invalid = 0x0U,
		Token,
		Comma,
		Semicolon,
		Section,
		Opcode,
		Tag,
		DataType,
		ImmediateValue,
		RegisterValue,
		StackValue,
		StaticValue,
		ValueFromRegisterAddress,
	};

	enum class AssemblerOpcode
	{
		NOP,
		MOV,
		MMB,
		LEA,
		NEW,
		DEL,
		MHRI,
		MHRD,
		CAST
	};

	enum class DataSize { Byte, Word, DoubleWord, QuadWord };
	enum class DataType
	{
		UByte, UWord, UDoubleWord, UQuadWord,
		SByte, SWord, SDoubleWord, SQuadWord,
		FloatingDecimal, DoubleDecimal
	};

	class Element
	{
	private: // Data of diferent element types
		struct RegisterValue { DataSize size; UInt8 registerId; };
		struct StackValue { DataSize size; UInt64 address, delta; };
		struct StaticValue { DataSize size; UInt64 address, delta; };
		struct ValueFromRegisterAddress { DataSize size; UInt8 registerId; UInt64 delta; };

	private:
		ElementType _type;
		union
		{
			union
			{
				std::string token;
				std::string section;
				AssemblerOpcode opcode;
				std::string tag;
				DataType dataType;
				UInt64 immediate;
				RegisterValue registerValue;
				StackValue stackValue;
				StaticValue staticValue;
				ValueFromRegisterAddress registerAddr;
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
		static Element section(const std::string& section);
		static Element opcode(AssemblerOpcode opcode);
		static Element tag(const std::string& tag);
		static Element dataType(DataType type);
		static Element immediate(UInt64 value);
		static Element registerValue(DataSize size, UInt8 registerId);
		static Element stackValue(DataSize size, UInt64 address, UInt64 delta = 0);
		static Element staticValue(DataSize size, UInt64 address, UInt64 delta = 0);
		static Element valueFromRegisterAddress(DataSize size, UInt8 registerId, UInt64 delta = 0);

		static inline Element comma() { return { ElementType::Comma }; }
		static inline Element semicolon() { return { ElementType::Semicolon }; }

	public:
		inline const std::string& token() const { return _data.token; }
		inline const std::string& section() const { return _data.section; }
		inline AssemblerOpcode opcode() const { return _data.opcode; }
		inline const std::string& tag() const { return _data.tag; }
		inline DataType dataType() const { return _data.dataType; }
		inline UInt64 immediate() const { return _data.immediate; }
		inline const RegisterValue& registerValue() const { return _data.registerValue; }
		inline const StackValue& stackValue() const { return _data.stackValue; }
		inline const StaticValue& staticValue() const { return _data.staticValue; }
		inline const ValueFromRegisterAddress& valueFromRegisterAddress() const { return _data.registerAddr; }

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

	

}

kram::assembler::parser::ElementArray operator+ (const kram::assembler::parser::Element& left, const kram::assembler::parser::Element& right);
