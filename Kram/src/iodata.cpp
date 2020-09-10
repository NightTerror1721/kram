#include "iodata.h"

#include "bytebuffer.h"

namespace kram::utils
{
	DataReader::Line::Line(unsigned int index, Offset offset, Size size) :
		index{ index },
		offset{ offset },
		size{ size }
	{}



	void DataReader::_destroy()
	{
		if (_chars && !_view)
			free_raw(_chars);
		if (_lines)
			delete _lines;

		_chars = nullptr;
		_charCount = 0;
		_view = false;
		_lines = nullptr;
		_lineCount = 0;
		_currentLine = nullptr;
		_currentIndex = 0;
	}

	DataReader& DataReader::_copy(const DataReader& dr, bool reset)
	{
		if (reset)
			_destroy();

		_chars = arraycopy_raw<char>(dr._chars, dr._charCount);
		_charCount = dr._charCount;
		_view = false;

		_lines = arraycopy(dr._lines, dr._lineCount);
		_lineCount = dr._lineCount;

		_currentLine = _lines + (dr._currentLine - dr._lines);
		_currentIndex = dr._currentIndex;

		return *this;
	}

	DataReader& DataReader::_move(DataReader&& dr, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_chars = dr._chars;
		_charCount = dr._charCount;
		_view = dr._view;
		_lines = dr._lines;
		_lineCount = dr._lineCount;
		_currentLine = dr._currentLine;
		_currentIndex = dr._currentIndex;

		dr._chars = nullptr;
		dr._charCount = 0;
		dr._view = false;
		dr._lines = nullptr;
		dr._lineCount = 0;
		dr._currentLine = nullptr;
		dr._currentIndex = 0;

		return *this;
	}

	void DataReader::_load(std::istream& input)
	{
		char buffer[8192];
		ByteBufferWriter databuffer;
		std::vector<Line> lines;
		Line current{ 0, 0, 0 };
		while (input)
		{
			input.getline(buffer, sizeof(buffer), newline);
			Size count = input.gcount();
			if (buffer[count - 1] == '\r')
				count--;
			databuffer.write(buffer, count);
			databuffer << newline;
			current.size += count;
			if (!input.fail())
			{
				lines.push_back(current);
				current.index++;
				current.offset += current.size;
				current.size = 0;
			}
			else input.clear(input.rdstate() & ~std::ios_base::failbit);
		}

		if (current.size > 0)
			lines.push_back(current);

		_destroy();
		if(!lines.empty())
		{
			_charCount = databuffer.size();
			databuffer.extract(rcast(std::byte**, &_chars));

			_lines = arraycopy(lines.data(), lines.size());
			_lineCount = lines.size();

			_currentLine = _lines;
			_currentIndex = 0;
		}
	}


	DataReader::DataReader(std::istream& input) :
		DataReader{}
	{
		_load(input);
	}
	DataReader::DataReader(const std::string& str) :
		DataReader{}
	{
		std::stringstream ss{ str };
		_load(ss);
	}

	void DataReader::load(std::istream& input)
	{
		_load(input);
	}
	void DataReader::load(const std::string& str)
	{
		std::stringstream ss{ str };
		_load(ss);
	}

	void DataReader::clear() { _destroy(); }

	char DataReader::next()
	{
		if (eof())
			return invalid_char;

		char c = _chars[_currentIndex++];
		if (_currentIndex == _currentLine->offset + _currentLine->size)
			_currentLine++;

		return c;
	}

	char DataReader::prev()
	{
		if (_currentIndex == 0)
			return invalid_char;

		if (_currentIndex == _currentLine->offset)
			_currentLine--;
		return _chars[--_currentIndex];
	}

	void DataReader::skip_line()
	{
		if (_lines)
		{
			if (_currentLine >= (_lines + (_lineCount - 1)))
			{
				_currentLine++;
				_currentIndex = _charCount;
			}
			else _currentIndex = (_currentLine++)->offset;
		}
	}

	char DataReader::_move_index(Int64 delta, bool persist)
	{
		if (delta == 0)
			return current();

		Offset offset = _currentIndex;
		Line* line = _currentLine;
		char c = invalid_char;

		if (delta > 0)
		{
			offset += delta;
			if (offset >= _charCount)
			{
				offset = _charCount;
				line = _lines ? _lines + _lineCount : nullptr;
			}
			else
			{
				while (offset > line->offset)
					line++;
				c = _chars[offset - 1];
			}
		}
		else
		{
			if (scast(Int64, offset) <= -delta)
			{
				offset = 0;
				line = _lines;
			}
			else
			{
				offset += delta;
				while (offset < line->offset)
					line--;
				c = _chars[offset - 1];
			}
		}

		if (persist)
		{
			_currentIndex = offset;
			_currentLine = line;
		}

		return c;
	}

	std::vector<char> DataReader::peek(Int64 from, Int64 to) const
	{
		Offset offset = _currentIndex;
		Line* line = _currentLine;
		DataReader* self = const_cast<DataReader*>(this);

		from = std::min(from, to);
		to = std::max(from, to);

		std::vector<char> chars(to - from);
		for (char c = self->_move_index(from, true); from <= to; c = self->next(), from++)
			chars.push_back(c);

		self->_currentIndex = offset;
		self->_currentLine = line;

		return chars;
	}

	DataReader DataReader::sub(Offset from, Offset to) const
	{
		if (!_chars)
			return {};

		from = std::min(from, to);
		to = std::min(_charCount, std::max(from, to));

		DataReader dr;
		dr._view = true;
		dr._chars = _chars + from;
		dr._charCount = to - from;

		std::vector<Line> lines;
		for (Offset i = 0; i < _lineCount; i++)
		{
			Line* line = _lines + i;
			if (line->offset < from)
				continue;
			if (line->offset > to)
				break;

			Line lcopy = *line;
			if (lines.empty())
			{
				lcopy.index = 0;
				lcopy.offset = 0;
				if(from > line->offset)
					lcopy.size = line->size - (from - line->offset);
			}
			else
			{
				lcopy.index = lines.back().index + 1;
				lcopy.offset -= from;
			}
			lines.push_back(lcopy);
		}

		if (!lines.empty())
		{
			Line& line = lines.back();
			if (line.offset + line.size > to)
				line.size -= to - (line.offset + line.size);
		}

		dr._lines = arraycopy(lines.data(), lines.size());
		dr._lineCount = lines.size();

		dr._currentLine = dr._lines;
		dr._currentIndex = 0;

		return dr;
	}
}
