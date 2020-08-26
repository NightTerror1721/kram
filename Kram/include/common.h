#pragma once

#include <type_traits>
#include <algorithm>
#include <iostream>
#include <utility>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <bit>

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

	class KramState;
	class Heap;

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

		float f32;
		double f64;


		UInt8* addr_u8;
		UInt16* addr_u16;
		UInt32* addr_u32;
		UInt64* addr_u64;

		Int8* addr_s8;
		Int16* addr_s16;
		Int32* addr_s32;
		Int64* addr_s64;

		float* addr_f32;
		double* addr_f64;

		void* addr;


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
	constexpr Size RuntimeStackDefaultSize = 1024 * 1024 * 8;
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

	template<unsigned int _BitIdx, unsigned int _BitCount, typename _Ty = UInt8>
	constexpr _Ty get_bits(_Ty value)
	{
		return static_cast<_Ty>((value >> _BitIdx) & ((0x1 << _BitCount) - 1));
	}

	template<unsigned int _BitIdx, unsigned int _BitCount, typename _Ty = UInt8>
	constexpr _Ty set_bits(_Ty base, _Ty bits)
	{
		_Ty mask = ((0x1 << _BitCount) - 1);
		_Ty value = ((bits & mask) << _BitIdx) & ~(_Ty(0));
		return (base & ~(mask << _BitIdx)) | value;
	}

	template<typename _Ty, typename... _Args>
	inline _Ty& construct(_Ty& object, _Args&&... args) { return new (&object) _Ty(std::forward<_Args>(args)...), object; }

	template<typename _Ty>
	inline _Ty& copy(_Ty& dest, const _Ty& source) { return construct<_Ty, const _Ty&>(dest, source); }

	template<typename _Ty>
	inline _Ty& move(_Ty& dest, _Ty&& source) { return construct<_Ty, _Ty&&>(dest, std::move(source)); }

	template<typename _Ty>
	inline void destroy(_Ty& object) { object.~_Ty(); }
}

#define CONCAT_MACROS(_A, _B) _A ## _B

#if defined (__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define KRAM_LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define KRAM_BIG_ENDIAN
# elif (__BYTE_ORDER == __PDP_ENDIAN)
#  define KRAM_PDP_ENDIAN
# else
#  error Unknown machine endianness detected.
# endif
# define KRAM_BYTE_ORDER __BYTE_ORDER
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
# define KRAM_BIG_ENDIAN
# define KRAM_BYTE_ORDER 4321
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
# define KRAM_LITTLE_ENDIAN
# define KRAM_BYTE_ORDER 1234
#elif defined(__sparc) || defined(__sparc__) \
   || defined(_POWER) || defined(__powerpc__) \
   || defined(__ppc__) || defined(__hpux) || defined(__hppa) \
   || defined(_MIPSEB) || defined(_POWER) \
   || defined(__s390__)
# define KRAM_BIG_ENDIAN
# define KRAM_BYTE_ORDER 4321
#elif defined(__i386__) || defined(__alpha__) \
   || defined(__ia64) || defined(__ia64__) \
   || defined(_M_IX86) || defined(_M_IA64) \
   || defined(_M_ALPHA) || defined(__amd64) \
   || defined(__amd64__) || defined(_M_AMD64) \
   || defined(__x86_64) || defined(__x86_64__) \
   || defined(_M_X64) || defined(__bfin__)

# define KRAM_LITTLE_ENDIAN
# define KRAM_BYTE_ORDER 1234
#else
# error Unknown endianness type for this CPU.
#endif

#if defined(_MSC_VER) || defined(_MSVC_LANG)
	#define forceinline __forceinline
#else
	#define forceinline inline
#endif

#define scast(_Type, _Value) static_cast<_Type>(_Value)
#define rcast(_Type, _Value) reinterpret_cast<_Type>(_Value)


