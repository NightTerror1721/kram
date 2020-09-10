#include "bytebuffer.h"

namespace kram::utils
{
	void ByteBufferWriter::_destroy() noexcept
	{
		if (_capacity > 0 && _buffer)
			free_raw(_buffer);

		_capacity = 0;
		_size = 0;
		_buffer = nullptr;
	}

	ByteBufferWriter& ByteBufferWriter::_copy(const ByteBufferWriter& bbw, bool reset)
	{
		if (reset)
			_destroy();

		_capacity = bbw._capacity;
		_size = bbw._size;
		_buffer = malloc_raw<std::byte>(_capacity);
		std::memcpy(_buffer, bbw._buffer, _size);

		return *this;
	}

	ByteBufferWriter& ByteBufferWriter::_move(ByteBufferWriter&& bbw, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_capacity = bbw._capacity;
		_size = bbw._size;
		_buffer = bbw._buffer;

		bbw._capacity = 0;
		bbw._size = 0;
		bbw._buffer = nullptr;

		return *this;
	}

	void ByteBufferWriter::_ensure_capacity(Size size)
	{
		if (_size + size > _capacity)
		{
			if (_capacity == 0)
			{
				_capacity = DefaultCapacity < size ? size : DefaultCapacity;
				_size = 0;
				_buffer = malloc_raw<std::byte>(_capacity);
			}
			else
			{
				std::byte* oldbuffer = _buffer;
				_capacity = _capacity * 2 < size ? size : _capacity * 2;
				_buffer = malloc_raw<std::byte>(_capacity);

				std::memcpy(_buffer, oldbuffer, _size);
				free_raw(oldbuffer);
			}
		}
	}

	ByteBufferWriter::ByteBufferWriter(Size capacity) :
		_buffer{ malloc_raw<std::byte>(capacity) },
		_size{ 0 },
		_capacity{ capacity }
	{}

	void ByteBufferWriter::extract(std::byte** dst)
	{
		*dst = _buffer;

		_capacity = 0;
		_size = 0;
		_buffer = nullptr;
	}

	void ByteBufferWriter::write(std::byte byte)
	{
		_ensure_capacity(1);
		_buffer[_size++] = byte;
	}
	void ByteBufferWriter::write(const void* buffer, Size size)
	{
		_ensure_capacity(size);
		std::memcpy(_buffer + _size, buffer, size);
		_size += size;
	}
	void ByteBufferWriter::write(std::istream& is, Size size)
	{
		char buffer[DefaultCapacity];
		bool endless = size == 0;
		
		while (is && (endless || size > 0))
		{
			if (endless)
				is.read(buffer, DefaultCapacity);
			else
			{
				if (size > DefaultCapacity)
				{
					is.read(buffer, DefaultCapacity);
					size -= DefaultCapacity;
				}
				else
				{
					is.read(buffer, size);
					size = 0;
				}
			}

			if (is.gcount() > 0)
				write(buffer, is.gcount());
		}
	}

	void ByteBufferWriter::write(UInt8 value)
	{
		_ensure_capacity(1);
		_buffer[_size++] = static_cast<std::byte>(value);
	}

	void ByteBufferWriter::write(UInt16 value)
	{
		_ensure_capacity(2);
		*reinterpret_cast<UInt16*>(_buffer + _size) = value;
		_size += 2;
	}

	void ByteBufferWriter::write(UInt32 value)
	{
		_ensure_capacity(4);
		*reinterpret_cast<UInt32*>(_buffer + _size) = value;
		_size += 4;
	}

	void ByteBufferWriter::write(UInt64 value)
	{
		_ensure_capacity(8);
		*reinterpret_cast<UInt64*>(_buffer + _size) = value;
		_size += 8;
	}

	void ByteBufferWriter::write(float value)
	{
		_ensure_capacity(sizeof(float));
		*reinterpret_cast<float*>(_buffer + _size) = value;
		_size += sizeof(float);
	}

	void ByteBufferWriter::write(double value)
	{
		_ensure_capacity(sizeof(double));
		*reinterpret_cast<double*>(_buffer + _size) = value;
		_size += sizeof(double);
	}

	void ByteBufferWriter::write(long double value)
	{
		_ensure_capacity(sizeof(long double));
		*reinterpret_cast<long double*>(_buffer + _size) = value;
		_size += sizeof(long double);
	}
}
