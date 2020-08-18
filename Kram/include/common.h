#pragma once

#include <type_traits>
#include <algorithm>
#include <iostream>
#include <utility>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <map>

namespace kram
{
	typedef std::uint8_t UInt8;
	typedef std::uint16_t UInt16;
	typedef std::uint32_t UInt32;
	typedef std::uint64_t UInt64;

	typedef std::int8_t Int8;
	typedef std::int16_t Int16;
	typedef std::int32_t Int32;
	typedef std::int64_t Int64;

	typedef std::size_t Size;

	//typedef UInt64 Register;

	union Register
	{
		UInt8 u8;
		UInt16 u16;
		UInt32 u32;
		UInt64 u64;

		Int8 s8;
		Int16 s16;
		Int32 s32;
		Int64 s64;

		UInt64 reg;
	};
}

namespace kram::types
{
	typedef UInt8 UnsignedByte;
	typedef UInt16 UnsignedShort;
	typedef UInt32 UnsignedInteger;
	typedef UInt64 UnsignedLong;

	typedef Int8 Byte;
	typedef Int16 Short;
	typedef Int32 Integer;
	typedef Int64 Long;

	typedef char Character;

	typedef float Float;
	typedef double Double;

	typedef void* Pointer;

	template<typename _Ty>
	using PointerOf = _Ty*;
}

namespace kram::utils
{
	inline char* c_str_copy(char* dest, const char* source)
	{
		#pragma warning(suppress : 4996)
		return std::strcpy(dest, source);
	}

	inline char* c_str_copy(char* dest, const char* source, Size num)
	{
		#pragma warning(suppress : 4996)
		return std::strncpy(dest, source, num);
	}
}

