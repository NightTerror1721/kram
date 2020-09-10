#pragma once

#include <type_traits>
#include <functional>
#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <utility>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <set>
#include <map>
#include <bit>
#include <new>

#ifndef __cpp_lib_concepts
#define __cpp_lib_concepts
#endif
#include <concepts>

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
	typedef std::size_t Offset;

	class KramState;
	class Heap;
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

	inline std::string to_string(char value) { return std::string(&value, 1); }

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

	template<typename _Ty>
	concept boolean = std::same_as<_Ty, bool>;

	template<typename _Ty>
	inline _Ty* arraycopy_raw(_Ty** dst, const void* src, Size size)
	{
		_Ty* array = malloc_raw<_Ty>(size);
		std::memcpy(array, src, size);
		return dst ? (*dst = array) : array;
	}

	template<typename _Ty>
	inline _Ty* arraycopy_raw(const void* src, Size size) { return arraycopy_raw<_Ty>(nullptr, src, size); }

	template<typename _Ty>
	_Ty* arraycopy(_Ty** dst, const _Ty* src, Size size)
	{
		_Ty* array = malloc_raw<_Ty>(size * sizeof(_Ty));
		const _Ty *s_ptr = src, *end = src + size;
		
		for (_Ty* d_ptr = array; s_ptr < end; ++s_ptr, ++d_ptr)
			copy<_Ty>(*d_ptr, *s_ptr);

		if (dst)
			*dst = array;
		return array;
	}

	template<typename _Ty>
	inline _Ty* arraycopy(const _Ty* src, Size size) { return arraycopy<_Ty>(nullptr, src, size); }

	template<typename _Ty>
	_Ty* arraymove(_Ty** dst, _Ty* src, Size size)
	{
		_Ty* array = malloc_raw<_Ty>(size * sizeof(_Ty));
		const _Ty* end = src + size;

		for (_Ty *d_ptr = array, *s_ptr = src; s_ptr < end; ++s_ptr, ++d_ptr)
			move<_Ty>(*d_ptr, std::move<_Ty>(*s_ptr));

		if (dst)
			*dst = array;
		return array;
	}

	template<typename _Ty>
	inline _Ty* arraymove(_Ty* src, Size size) { return arraymove<_Ty>(nullptr, src, size); }

	template<typename _Ty>
	concept _Type_char = std::same_as<_Ty, char>;

	template<typename _TypeToTest, typename _Ty>
	inline bool instance_of(const _Ty& value)
	{
		try { return dynamic_cast<const _TypeToTest&>(value), true; }
		catch (const std::bad_cast& ex) { return false; }
		return false;
	}

	template<typename _TypeToTest, typename _Ty>
	inline bool instance_of(const _Ty* value)
	{
		return dynamic_cast<const _TypeToTest*>(value) != nullptr;
	}

	template<typename _TypeToTest, typename _Ty>
	bool instance_of_then_do(const _Ty& value, const std::function<void(const _TypeToTest&)>& action)
	{
		try
		{
			action(dynamic_cast<const _TypeToTest&>(value));
			return true;
		}
		catch (const std::bad_cast&) { return false; }
		return false;
	}

	template<typename _TypeToTest, typename _Ty>
	bool instance_of_then_do(_Ty& value, const std::function<void(_TypeToTest&)>& action)
	{
		try
		{
			action(dynamic_cast<_TypeToTest&>(value));
			return true;
		}
		catch (const std::bad_cast& ex) { return false; }
		return false;
	}

	template<typename _TypeToTest, typename _Ty>
	bool instance_of_then_do(_Ty* value, const std::function<void(_TypeToTest*)>& action)
	{
		_TypeToTest* ptr = dynamic_cast<_TypeToTest*>(value);
		if (ptr)
			return action(ptr), true;
		return false;
	}

	template<typename _TypeToTest, typename _Ty>
	bool instance_of_then_do(const _Ty* value, const std::function<void(const _TypeToTest*)>& action)
	{
		const _TypeToTest* ptr = dynamic_cast<const _TypeToTest*>(value);
		if (ptr)
			return action(ptr), true;
		return false;
	}

	template<std::integral _Ty>
	constexpr _Ty hex_unit_to_integer(char unit, unsigned int offset = 0)
	{
		if (unit >= '0' && unit <= '9')
			return static_cast<_Ty>(unit - '0') << (offset * 4);
		if (unit >= 'a' && unit <= 'f')
			return static_cast<_Ty>(unit - 'a') << (offset * 4);
		if (unit >= 'A' && unit <= 'F')
			return static_cast<_Ty>(unit - 'A') << (offset * 4);
		return 0;
	}
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


#define _kram_malloc(_Type, _Size) reinterpret_cast<_Type*>(::operator new(_Size))
#define _kram_free(_Ptr) ::operator delete(_Ptr)


namespace kram::utils
{
	template<typename _Ty = void>
	forceinline _Ty* malloc_raw(Size size) { return _kram_malloc(_Ty, size); }

	forceinline void free_raw(void* ptr) noexcept { _kram_free(ptr); }
}

