#include "bindata.h"

namespace kram::bin
{
	static const DataType Void{ TypeIdentifier::Void };
	static const DataType UnsignedByte{ TypeIdentifier::UnsignedByte };
	static const DataType UnsignedShort{ TypeIdentifier::UnsignedShort };
	static const DataType UnsignedInteger{ TypeIdentifier::UnsignedInteger };
	static const DataType UnsignedLong{ TypeIdentifier::UnsignedLong };
	static const DataType SignedByte{ TypeIdentifier::SignedByte };
	static const DataType SignedShort{ TypeIdentifier::SignedShort };
	static const DataType SignedInteger{ TypeIdentifier::SignedInteger };
	static const DataType SignedLong{ TypeIdentifier::SignedLong };
	static const DataType Float{ TypeIdentifier::Float };
	static const DataType Double{ TypeIdentifier::Double };
	static const DataType Character{ TypeIdentifier::Character };
	static const DataType Boolean{ TypeIdentifier::Boolean };


	const DataType& DataType::Void = kram::bin::Void;
	const DataType& DataType::UnsignedByte = kram::bin::UnsignedByte;
	const DataType& DataType::UnsignedShort = kram::bin::UnsignedShort;
	const DataType& DataType::UnsignedInteger = kram::bin::UnsignedInteger;
	const DataType& DataType::UnsignedLong = kram::bin::UnsignedLong;
	const DataType& DataType::SignedByte = kram::bin::SignedByte;
	const DataType& DataType::SignedShort = kram::bin::SignedShort;
	const DataType& DataType::SignedInteger = kram::bin::SignedInteger;
	const DataType& DataType::SignedLong = kram::bin::SignedLong;
	const DataType& DataType::Float = kram::bin::Float;
	const DataType& DataType::Double = kram::bin::Double;
	const DataType& DataType::Character = kram::bin::Character;
	const DataType& DataType::Boolean = kram::bin::Boolean;


	DataType DataType::pointerOf(const DataType& type)
	{
		DataType ptr{ TypeIdentifier::Pointer };
		ptr._pointerType = &type;
		return ptr;
	}

	DataType DataType::arrayOf(const DataType& type, Size length)
	{
		DataType arr{ TypeIdentifier::Array };
		arr._arrayType = &type;
		arr._arrayLength = length < 1 ? 1 : length;
		return arr;
	}

	DataType DataType::structOf(const std::vector<StructField>& fields)
	{
		DataType st{ TypeIdentifier::Struct };
		st._structFieldCount = fields.size();
		if (st._structFields > 0)
		{
			Size len = st._structFieldCount;
			Size idx = 0;
			st._structFields = new StructField[len];
			for (const StructField& field : fields)
			{
				StructField* fieldPtr = st._structFields + (idx++);

				utils::c_str_copy(fieldPtr->name, field.name);
				fieldPtr->type = field.type;
			}
		}
		else st._structFields = nullptr;

		return st;
	}



	DataType::StructBuilder& DataType::StructBuilder::put(const std::string& name, const DataType& type)
	{
		_fields[name] = &type;
		return *this;
	}
	DataType::StructBuilder& DataType::StructBuilder::put(const char* name, const DataType& type)
	{
		_fields[name] = &type;
		return *this;
	}

	DataType DataType::StructBuilder::build() const
	{
		DataType st{ TypeIdentifier::Struct };
		st._structFieldCount = _fields.size();
		if (st._structFields > 0)
		{
			Size len = st._structFieldCount;
			Size idx = 0;
			st._structFields = new StructField[len];
			for (const auto& field : _fields)
			{
				StructField* fieldPtr = st._structFields + (idx++);

				utils::c_str_copy(fieldPtr->name, field.first.c_str(), sizeof(fieldPtr->name));
				fieldPtr->type = field.second;
			}
		}
		else st._structFields = nullptr;

		return st;
	}
}


namespace kram::bin
{
	DataType::DataType(const DataType& type) :
		_typeId{},
		_unionraw{}
	{
		_copy(type, false);
	}
	DataType::DataType(DataType&& type) noexcept :
		_typeId{},
		_unionraw{}
	{
		_move(std::move(type), false);
	}
	DataType::~DataType() { _destroy(); }

	DataType& DataType::operator= (const DataType& right) { return _copy(right, true); }
	DataType& DataType::operator= (DataType&& right) noexcept { return _move(std::move(right), true); }

	bool DataType::operator== (const DataType& right) const
	{
		if (_typeId == right._typeId)
		{
			switch (_typeId)
			{
				case TypeIdentifier::Pointer:
					return *_pointerType == *right._pointerType;

				case TypeIdentifier::Array:
					return *_arrayType == *right._arrayType && _arrayLength == right._arrayLength;

				case TypeIdentifier::Struct:
					if (_structFieldCount == right._structFieldCount)
					{
						const Size len = _structFieldCount;
						for (Size i = 0; i < len; i++)
						{
							StructField* leftField = _structFields + i;
							StructField* rightField = right._structFields + i;

							if (std::strcmp(leftField->name, rightField->name) != 0 || !(*leftField->type == *rightField->type))
								return false;
						}
						return true;
					}
					return false;

				default:
					return true;
			}
		}
		return false;
	}

	void DataType::_destroy()
	{
		switch (_typeId)
		{
			case TypeIdentifier::Pointer:
				_pointerType = nullptr;
				break;

			case TypeIdentifier::Array:
				_arrayType = nullptr;
				_arrayLength = 0;
				break;

			case TypeIdentifier::Struct:
				_structFieldCount = 0;
				delete[] _structFields;
				_structFields = nullptr;
				break;
		}
		_typeId = TypeIdentifier::Void;
	}
	DataType& DataType::_copy(const DataType& type, bool reset)
	{
		if(reset)
			_destroy();

		_typeId = type._typeId;
		switch (_typeId)
		{
			case TypeIdentifier::Pointer:
				_pointerType = type._pointerType;
				break;

			case TypeIdentifier::Array:
				_arrayType = type._arrayType;
				_arrayLength = type._arrayLength;
				break;

			case TypeIdentifier::Struct:
				if (type._structFieldCount > 0)
				{
					Size len = type._structFieldCount;
					_structFieldCount = len;
					_structFields = new StructField[_structFieldCount];
					for (Size i = 0; i < len; i++)
					{
						StructField* leftField = _structFields + i;
						StructField* rightField = type._structFields + i;

						utils::c_str_copy(leftField->name, rightField->name);
						leftField->type = rightField->type;
					}
				}
				else
				{
					_structFieldCount = 0;
					_structFields = nullptr;
				}
				break;
		}

		return *this;
	}
	DataType& DataType::_move(DataType&& type, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_typeId = type._typeId;
		switch (_typeId)
		{
			case TypeIdentifier::Pointer:
				_pointerType = type._pointerType;
				type._pointerType = nullptr;
				break;

			case TypeIdentifier::Array:
				_arrayType = type._arrayType;
				_arrayLength = type._arrayLength;
				type._arrayType = nullptr;
				type._arrayLength = 0;
				break;

			case TypeIdentifier::Struct:
				_structFieldCount = type._structFieldCount;
				_structFields = type._structFields;
				type._structFieldCount = 0;
				type._structFields = nullptr;
				break;
		}
		type._typeId = TypeIdentifier::Void;

		return *this;
	}
}
