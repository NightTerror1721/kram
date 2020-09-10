#pragma once

#include "common.h"

#include <fstream>

namespace kram::utils
{
	class DataReader
	{
	private:
		struct Line
		{
			unsigned int index;
			Offset offset;
			Size size;

			Line(unsigned int index, Offset offset, Size size = 0);

			Line() = default;
			Line(const Line&) = default;

			Line& operator= (const Line&) = default;
		};

	public:
		static constexpr char invalid_char = '\0';
		static constexpr char newline = '\n';

	private:
		char* _chars;
		Size _charCount;
		bool _view;

		Line* _lines;
		Size _lineCount;

		Line* _currentLine;
		Offset _currentIndex;

	private:
		void _destroy();
		DataReader& _copy(const DataReader& dr, bool reset);
		DataReader& _move(DataReader&& dr, bool reset) noexcept;

		void _load(std::istream& input);

		char _move_index(Int64 delta, bool persist);

	public:
		DataReader(std::istream& input);
		DataReader(const std::string& str);

		void load(std::istream& input);
		void load(const std::string& str);

		void clear();

		char next();
		char prev();

		void skip_line();

		std::vector<char> peek(Int64 from, Int64 to) const;

		DataReader sub(Offset from, Offset to) const;

	public:
		inline bool eof() const { return _currentIndex >= _charCount; }

		inline char current() const { return _currentIndex >= _charCount || _currentIndex == 0 ? invalid_char : _chars[_currentIndex - 1]; }

		inline Size lines() const { return _lineCount; }
		inline Size current_line() const { return _currentLine ? (eof() ? _lines[_lineCount - 1].index + 1 : _currentLine->index) : 0; }
		inline Size current_column() const { return _currentLine ? (eof() ? _charCount : _currentIndex - _currentLine->offset) : 0; }

		inline char peek(Int64 delta = 0) const { return const_cast<DataReader*>(this)->_move_index(delta, false); }

		inline bool has(Int64 delta, char c) const { return peek(delta) == c; }
		inline bool has(Int64 delta) const { return peek(delta) != invalid_char; }

		inline char move(Int64 delta) { return _move_index(delta, true); }

		inline Offset offset() const { return _currentIndex; }
		inline void offset(Offset offset) { _move_index(offset - _currentIndex, true); }

		inline bool has(Int64 from, Int64 to, const std::vector<char>& chars) const { return peek(from, to) == chars; }
		inline bool has(Int64 from, const std::vector<char>& chars) const { return peek(from, from + chars.size()) == chars; }

		inline operator bool() const { return !eof(); }
		inline bool operator! () const { return eof(); }

		inline DataReader& operator>> (char& right) { return right = next(), *this; }

		template<std::integral _Ty>
		inline char operator[] (_Ty delta) const { return const_cast<DataReader*>(this)->_move_index(static_cast<Int64>(delta), false); }

		template<std::integral _Ty>
		inline DataReader& operator+= (_Ty delta) { return _move_index(static_cast<Int64>(delta), true), *this; }

		template<std::integral _Ty>
		inline DataReader& operator-= (_Ty delta) { return _move_index(-static_cast<Int64>(delta), true), *this; }

		template<_Type_char... _Args>
		inline bool has(Int64 from, _Args... chars) { return has(from, std::vector<char>{ chars... }); }

	public:
		constexpr DataReader() :
			_chars{ nullptr },
			_charCount{ 0 },
			_view{ false },
			_lines{ nullptr },
			_lineCount{ 0 },
			_currentLine{ nullptr },
			_currentIndex{ 0 }
		{}

		inline DataReader(const DataReader& dr) : DataReader{} { _copy(dr, false); }
		inline DataReader(DataReader&& dr) noexcept :
			_chars{ dr._chars },
			_charCount{ dr._charCount },
			_view{ dr._view },
			_lines{ dr._lines },
			_lineCount{ dr._lineCount },
			_currentLine{ dr._currentLine },
			_currentIndex{ dr._currentIndex }
		{
			dr._chars = nullptr;
			dr._charCount = 0;
			dr._view = false;
			dr._lines = nullptr;
			dr._lineCount = 0;
			dr._currentLine = nullptr;
			dr._currentIndex = 0;
		}
		inline ~DataReader() { _destroy(); }

		inline DataReader& operator= (const DataReader& right) { return _copy(right, true); }
		inline DataReader& operator= (DataReader&& right) noexcept { return _move(std::move(right), true); }
	};
}
