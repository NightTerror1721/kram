#pragma once

#include "common.h"

namespace kram::utils
{
	class ByteBufferWriter
	{
	private:
		std::byte* _buffer;
		Size _size;
		Size _capacity;

	private:
		void _destroy() noexcept;
		ByteBufferWriter& _copy(const ByteBufferWriter& bbw, bool reset);
		ByteBufferWriter& _move(ByteBufferWriter&& bbw, bool reset) noexcept;

		void _ensure_capacity(Size size);

	public:
		ByteBufferWriter(Size capacity);

		void extract(std::byte** dst);

		void write(std::byte byte);
		void write(const void* buffer, Size size);
		void write(std::istream& is, Size size = 0);

		void write(UInt8 value);
		void write(UInt16 value);
		void write(UInt32 value);
		void write(UInt64 value);

		void write(float value);
		void write(double value);
		void write(long double value);

	public:
		inline void write(char value) { write(static_cast<std::byte>(value)); }
		inline void write(Int8 value) { write(static_cast<UInt8>(value)); }
		inline void write(Int16 value) { write(static_cast<UInt16>(value)); }
		inline void write(Int32 value) { write(static_cast<UInt32>(value)); }
		inline void write(Int64 value) { write(static_cast<UInt64>(value)); }

		inline void write(const char* str) { write(str, std::strlen(str)); }
		inline void write(const std::string& str) { write(str.data(), str.size()); }

		template<typename _Ty>
		inline void write(const _Ty& value)
		{
			_ensure_capacity(sizeof(_Ty));
			*reinterpret_cast<_Ty*>(_buffer + _size) = value;
			_size += sizeof(_Ty);
		}

		inline ByteBufferWriter& operator<< (std::byte right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (std::istream& right) { return write(right), *this; }

		inline ByteBufferWriter& operator<< (char right) { return write(right), * this; }

		inline ByteBufferWriter& operator<< (UInt8 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (UInt16 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (UInt32 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (UInt64 right) { return write(right), *this; }

		inline ByteBufferWriter& operator<< (Int8 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (Int16 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (Int32 right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (Int64 right) { return write(right), *this; }

		inline ByteBufferWriter& operator<< (float right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (double right) { return write(right), *this; }
		inline ByteBufferWriter& operator<< (long double right) { return write(right), *this; }

		inline ByteBufferWriter& operator<< (const char* str) { return write(str), *this; }
		inline ByteBufferWriter& operator<< (const std::string& str) { return write(str), *this; }

		template<typename _Ty>
		inline ByteBufferWriter& operator<< (const _Ty& right) { return write<_Ty>(right), *this; }

	public:
		constexpr ByteBufferWriter() : _buffer{ nullptr }, _size{ 0 }, _capacity{ 0 } {}
		inline ByteBufferWriter(const ByteBufferWriter& bbw) : ByteBufferWriter{} { _copy(bbw, false); }
		inline ByteBufferWriter(ByteBufferWriter&& bbw) noexcept : ByteBufferWriter{} { _move(std::move(bbw), false); }

		ByteBufferWriter& operator= (const ByteBufferWriter& right) { return _copy(right, true); }
		ByteBufferWriter& operator= (ByteBufferWriter&& right) noexcept { return _move(std::move(right), true); }

		inline Size size() const { return _size; }
		inline bool empty() const { return _size == 0; }

		inline void clear() { _size = 0; }

		inline Size capacity() const { return _capacity; }

		static constexpr Size DefaultCapacity = 8192;
	};
}
