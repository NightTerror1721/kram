#pragma once

#include "common.h"

namespace kram::bin
{
	enum class TypeIdentifier : UInt8
	{
		Void,

		UnsignedByte,
		UnsignedShort,
		UnsignedInteger,
		UnsignedLong,

		SignedByte,
		SignedShort,
		SignedInteger,
		SignedLong,

		Float,
		Double,

		Character,

		Boolean,

		Pointer,

		Array,

		Struct
	};

	class DataType
	{
	public:
		static constexpr Size MaxStructFieldNameSize = 64;
		struct StructField
		{
			char name[MaxStructFieldNameSize + 1];
			const DataType* type;
		};

	private:
		TypeIdentifier _typeId;
		union
		{
			const DataType* _pointerType;
			struct
			{
				const DataType* _arrayType;
				Size _arrayLength;
			};
			struct
			{
				Size _structFieldCount;
				StructField* _structFields;
			};
			char _unionraw[sizeof(_structFieldCount) + sizeof(_structFields)];
		};

	public:
		DataType(const DataType& type);
		DataType(DataType&& type) noexcept;
		~DataType();

		DataType& operator= (const DataType& right);
		DataType& operator= (DataType&& right) noexcept;

		bool operator== (const DataType& right) const;

	private:
		void _destroy();
		DataType& _copy(const DataType& type, bool reset = true);
		DataType& _move(DataType&& type, bool reset = true) noexcept;

	public:
		constexpr DataType(TypeIdentifier typeId) :
			_typeId{ typeId },
			_unionraw{}
		{}
		constexpr DataType() : DataType{ TypeIdentifier::Void } {}

		inline bool operator!= (const DataType& right) const { return !(*this == right); }

		inline TypeIdentifier id() const { return _typeId; }

		inline bool isPointer() const { return _typeId == TypeIdentifier::Pointer; }
		inline bool isArray() const { return _typeId == TypeIdentifier::Array; }
		inline bool isStruct() const { return _typeId == TypeIdentifier::Struct; }

		inline const DataType& pointerType() const { return *_pointerType; }

		inline const DataType& arrayType() const { return *_arrayType; }
		inline Size arrayLength() const { return _arrayLength; }

		inline Size structFieldCount() const { return _structFieldCount; }
		inline const StructField& structField(Size index) const { return _structFields[index]; }

	public:
		static const DataType& Void;
		static const DataType& UnsignedByte;
		static const DataType& UnsignedShort;
		static const DataType& UnsignedInteger;
		static const DataType& UnsignedLong;
		static const DataType& SignedByte;
		static const DataType& SignedShort;
		static const DataType& SignedInteger;
		static const DataType& SignedLong;
		static const DataType& Float;
		static const DataType& Double;
		static const DataType& Character;
		static const DataType& Boolean;

		DataType pointerOf(const DataType& type);
		DataType arrayOf(const DataType& type, Size length);
		DataType structOf(const std::vector<StructField>& fields);

	public:
		class StructBuilder
		{
		public:
			using TypePtr = const DataType*;

		private:
			std::map<std::string, TypePtr> _fields;

		public:
			StructBuilder& put(const std::string& name, const DataType& type);
			StructBuilder& put(const char* name, const DataType& type);

			DataType build() const;
		};
		static inline StructBuilder buildStruct() { return StructBuilder(); }
	};
}


namespace kram::bin
{
	struct StaticValue
	{
		const DataType* type;
		Size size;
		void* data;
	};

	struct Function
	{
		UInt8 parameterCount;
		UInt8 registerCount;
		Size stackSize;

		const DataType** parameterTypes;
		const DataType* returnType;

		void* code;
	};

	struct Chunk
	{
		Size size;
		void* data;

		Size typeCount;
		Size staticCount;
		Size functionCount;
		Size childChunkCount;
		Size codeSize;
		
		const DataType* types;
		StaticValue* statics;
		Function* functions;
		Chunk* childChunks;
	};
}
