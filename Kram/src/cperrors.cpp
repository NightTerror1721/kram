#include "cperrors.h"

namespace kram::utils
{
	CompilerError::CompilerError() :
		exception{},
		_parserData{ false },
		_row{ 0 },
		_column{ 0 }
	{}
	CompilerError::CompilerError(const char* msg) :
		exception{ msg },
		_parserData{ false },
		_row{ 0 },
		_column{ 0 }
	{}
	CompilerError::CompilerError(const char* msg, Offset row, Offset column) :
		exception{ msg },
		_parserData{ true },
		_row{ row },
		_column{ column }
	{}
	CompilerError::CompilerError(const std::string& msg) :
		exception{ msg.c_str() },
		_parserData{ false },
		_row{ 0 },
		_column{ 0 }
	{}
	CompilerError::CompilerError(const std::string& msg, Offset row, Offset column) :
		exception{ msg.c_str() },
		_parserData{ true },
		_row{ row },
		_column{ column }
	{}



	void CompilerErrorContainer::addError(const std::exception& ex)
	{
		if (!instance_of_then_do<CompilerError>(ex, [this](const CompilerError& value) { addError(value); }))
			addError(CompilerError(ex.what()));
	}
}
