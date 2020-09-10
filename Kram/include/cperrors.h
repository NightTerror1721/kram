#pragma once

#include "common.h"
#include "iodata.h"

namespace kram::utils
{
	class CompilerError : public std::exception
	{
	private:
		bool _parserData;
		Offset _row;
		Offset _column;

	public:
		CompilerError();
		CompilerError(const char* msg);
		CompilerError(const char* msg, Offset row, Offset column);
		CompilerError(const std::string& msg);
		CompilerError(const std::string& msg, Offset row, Offset column);

		inline CompilerError(const char* msg, const DataReader& reader) : CompilerError(msg, reader.current_line(), reader.current_column()) {}
		inline CompilerError(const std::string& msg, const DataReader& reader) : CompilerError(msg, reader.current_line(), reader.current_column()) {}

		inline std::string message() const { return what(); }
		inline bool hasParserData() const { return _parserData; }
		inline Offset row() const { return _row; }
		inline Offset column() const { return _column; }
	};



	class CompilerErrorContainer
	{
	private:
		std::vector<CompilerError> _errors;

	public:
		CompilerErrorContainer() = default;
		CompilerErrorContainer(const CompilerErrorContainer&) = default;
		CompilerErrorContainer(CompilerErrorContainer&&) noexcept = default;
		~CompilerErrorContainer() = default;

		CompilerErrorContainer& operator= (const CompilerErrorContainer&) = default;
		CompilerErrorContainer& operator= (CompilerErrorContainer&&) noexcept = default;

		inline bool hasErrors() const { return !_errors.empty(); }

		inline void addError(const CompilerError& error) { _errors.push_back(error); }
		inline void addError(CompilerError&& error) { _errors.push_back(std::move(error)); }

		inline const std::vector<CompilerError>& errors() const { return _errors; }

		void addError(const std::exception& ex);

	public:
		using iterator = decltype(_errors)::iterator;
		using const_iterator = decltype(_errors)::const_iterator;

		inline iterator begin() noexcept { return _errors.begin(); }
		inline const_iterator begin() const noexcept { return _errors.begin(); }
		inline const_iterator cbegin() const noexcept { return _errors.cbegin(); }
		inline iterator end() noexcept { return _errors.end(); }
		inline const_iterator end() const noexcept { return _errors.end(); }
		inline const_iterator cend() const noexcept { return _errors.cend(); }
	};
}
