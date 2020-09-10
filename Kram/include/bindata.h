#pragma once

#include "common.h"
#include "opcodes.h"

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
	struct Function
	{
		Size parameterCount;
		Size stackCount;
		std::uintptr_t codeOffset;
	};

	struct Chunk
	{
	private:
		Size _size = 0;
		void* _data = nullptr;

	public:
		Size staticCount = 0;
		Size functionCount = 0;
		Size connectionCount = 0;
		Size codeCount = 0;
		
		std::byte* statics = nullptr;
		Function* functions = nullptr;
		Chunk* connections = nullptr;
		std::byte* code = nullptr;

		Chunk() = default;
		Chunk(Size size);
		~Chunk();

		void connect_ptr(void** ptr, std::uintptr_t offset);
	};
}

namespace kram::bin
{
	class ChunkBuilder;

	class FunctionBuilder
	{
	private:
		Size _params = 0;
		Size _stackSize = 0;
		op::InstructionBuilder _code;
		Size __codeByteCount = 0;

	public:
		FunctionBuilder() = default;
		FunctionBuilder(const FunctionBuilder&) = default;
		FunctionBuilder(FunctionBuilder&&) noexcept = default;
		~FunctionBuilder() = default;

		FunctionBuilder& operator= (const FunctionBuilder&) = default;
		FunctionBuilder& operator=(FunctionBuilder&&) noexcept = default;

		inline void parameters(Size parameters) { _params = parameters; }
		inline void stack_size(Size stack_size) { _stackSize = stack_size; }

		inline Size parameters() const { return _params; }
		inline Size stack_size() const { return _stackSize; }

		inline void code(const op::InstructionBuilder& code) { _code = code; }
		inline void code(op::InstructionBuilder&& code) { _code = std::move(code); }
		inline const op::InstructionBuilder& code() const { return _code; }

		friend class ChunkBuilder;
	};

	class ChunkBuilder
	{
	private:
		std::vector<Size> _statics;
		std::vector<FunctionBuilder> _functions;
		std::vector<Chunk*> _connections;

	public:
		ChunkBuilder() = default;
		ChunkBuilder(const ChunkBuilder&) = default;
		ChunkBuilder(ChunkBuilder&&) noexcept = default;
		~ChunkBuilder() = default;

		ChunkBuilder& operator= (const ChunkBuilder&) = default;
		ChunkBuilder& operator= (ChunkBuilder&&) noexcept = default;

		inline void add_static(Size size) { _statics.push_back(size); }
		inline void add_function(const FunctionBuilder& function) { _functions.push_back(function); }
		inline void add_connection(Chunk* chunk) { _connections.push_back(chunk); }

		inline ChunkBuilder& operator<< (Size static_size) { return add_static(static_size), *this; }
		inline ChunkBuilder& operator<< (const FunctionBuilder& function) { return add_function(function), *this; }
		inline ChunkBuilder& operator<< (Chunk* chunk) { return add_connection(chunk), *this; }

		void build(Chunk* chunk);
	};
}
