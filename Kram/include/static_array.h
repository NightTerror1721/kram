#pragma once

#include "common.h"

namespace kram::utils
{
	template<typename _Ty>
	class StaticArray
	{
	private:
		_Ty* _elems;
		Size _size;

	public:
		StaticArray(const std::vector<_Ty>& v);
		StaticArray(std::vector<_Ty>&& v);
		StaticArray(_Ty* buffer, Size size, bool move = false);

		StaticArray subarray(Size offset, Size length = 0);

		void concat(_Ty* buffer, Size size, bool move = false);

	public:
		inline Size size() const { return _size; }
		inline bool empty() const { return !_elems; }

		inline void set(Size index, const _Ty& elem) { _elems[index] = elem; }
		inline void set(Size index, _Ty&& elem) { _elems[index] = std::move(elem); }

		inline _Ty& get(Size index) { return _elems[index]; }
		inline const _Ty& get(Size index) const { return _elems[index]; }

		inline void concat(const _Ty* buffer, Size size) { concat(const_cast<_Ty*>(buffer), size, false); }
		inline void concat(const StaticArray& a) { concat(a._elems, a._size, false); }
		inline void concat(StaticArray&& a) { concat(a._elems, a._size, true), a._elems = nullptr, a._size = 0; }
		inline void concat(const std::vector<_Ty>& v) { concat(v.data(), v.size()); }
		inline void concat(std::vector<_Ty>&& v) { concat(v.data(), v.size(), true); }
		inline void concat(const _Ty& elem) { concat(&elem, 1); }
		inline void concat(_Ty&& elem) { concat(&elem, 1, true); }

		inline StaticArray concat(_Ty* buffer, Size size, bool move = false) const { StaticArray copy{ *this }; return copy.concat(buffer, size, move), copy; }
		inline StaticArray concat(const _Ty* buffer, Size size) const { StaticArray copy{ *this }; return copy.concat(buffer, size), copy; }
		inline StaticArray concat(const StaticArray& a) const { StaticArray copy{ *this }; return copy.concat(a), copy; }
		inline StaticArray concat(StaticArray&& a) const { StaticArray copy{ *this }; return copy.concat(std::move(a)), copy; }
		inline StaticArray concat(const std::vector<_Ty>& a) const { StaticArray copy{ *this }; return copy.concat(a), copy; }
		inline StaticArray concat(std::vector<_Ty>&& a) const { StaticArray copy{ *this }; return copy.concat(std::move(a)), copy; }
		inline StaticArray concat(const _Ty& a) const { StaticArray copy{ *this }; return copy.concat(a), copy; }
		inline StaticArray concat(_Ty&& a) const { StaticArray copy{ *this }; return copy.concat(std::move(a)), copy; }

	private:
		StaticArray(Size size);

		void _reconstruct(Size size, bool destroy);
		void _destroy();
		StaticArray& _copy(const StaticArray& a, bool reset);
		StaticArray& _move(StaticArray&& a, bool reset) noexcept;

	public:
		constexpr StaticArray() : _elems{ nullptr }, _size{ 0 } {}
		inline StaticArray(const _Ty* buffer, Size size) : StaticArray{ const_cast<_Ty*>(buffer), size, false } {}
		inline StaticArray(const _Ty& value) : StaticArray{ &value, 1U } {}
		inline StaticArray(_Ty&& value) : StaticArray{ &value, 1U, true } {}
		inline StaticArray(const StaticArray& a) : StaticArray{} { _copy(a, false); }
		inline StaticArray(StaticArray&& a) noexcept : StaticArray{} { _move(std::move(a), false); }
		inline ~StaticArray() { _destroy(); }

		inline StaticArray& operator= (const StaticArray& right) { return _copy(right, true); }
		inline StaticArray& operator= (StaticArray&& right) noexcept { return _move(std::move(right), true); }

		inline _Ty& operator[] (Size index) { return _elems[index]; }
		inline const _Ty& operator[] (Size index) const { return _elems[index]; }

		inline StaticArray& operator+= (const StaticArray& right) { return concat(right), * this; }
		inline StaticArray& operator+= (StaticArray&& right) { return concat(std::move(right)), * this; }
		inline StaticArray& operator+= (const std::vector<_Ty>& right) { return concat(right), * this; }
		inline StaticArray& operator+= (std::vector<_Ty>&& right) { return concat(std::move(right)), * this; }
		inline StaticArray& operator+= (const _Ty& right) { return concat(right), * this; }
		inline StaticArray& operator+= (_Ty&& right) { return concat(std::move(right)), * this; }

		inline StaticArray operator+ (const StaticArray& right) const { return concat(right); }
		inline StaticArray operator+ (StaticArray&& right) const { return concat(std::move(right)); }
		inline StaticArray operator+ (const std::vector<_Ty>& right) const { return concat(right); }
		inline StaticArray operator+ (std::vector<_Ty>&& right) const { return concat(std::move(right)); }
		inline StaticArray operator+ (const _Ty& right) const { return concat(right); }
		inline StaticArray operator+ (_Ty&& right) const { return concat(std::move(right)); }

	public:
		class iterator;
		class const_iterator;

		class const_iterator
		{
		private:
			const StaticArray* _array;
			const _Ty* _offset;

		public:
			const_iterator() : _array{ nullptr }, _offset{ nullptr } {}
			const_iterator(const StaticArray& array, std::uintptr_t offset) :
				_array{ &array },
				_offset{ array._elems + offset }
			{}
			const_iterator& operator++ () { return ++_offset, *this; }
			const_iterator operator++ (int) { return { _array, _offset++ }; }
			bool operator== (const const_iterator& right) const
			{
				return _offset >= (_array->_elems + _array->_size)
					? right._offset >= (right._array->_elems + right._array->_size)
					: _offset == right._offset;
			}
			bool operator!= (const const_iterator& right) const
			{
				return _offset >= (_array->_elems + _array->_size)
					? right._offset < (right._array->_elems + right._array->_size)
					: _offset != right._offset;
			}
			const _Ty& operator* () const { return *_offset; }
			const _Ty* operator-> () const { return _offset; }

			const_iterator(const iterator& it);

			const_iterator& operator= (const iterator& right);

			bool operator== (const iterator& right);
			inline bool operator!= (const iterator& right) { return !(*this == right); }

			// iterator traits
			using difference_type = std::ptrdiff_t;
			using value_type = _Ty;
			using pointer = _Ty*;
			using reference = _Ty&;
			using iterator_category = std::forward_iterator_tag;
		};

		class iterator
		{
		private:
			StaticArray* _array;
			_Ty* _offset;

		public:
			iterator() : _array{ nullptr }, _offset{ nullptr } {}
			iterator(StaticArray& array, std::uintptr_t offset) :
				_array{ &array },
				_offset{ array._elems + offset }
			{}
			iterator& operator++ () { return ++_offset, * this; }
			iterator operator++ (int) { return { _array, _offset++ }; }
			bool operator== (const iterator& right) const
			{
				return _offset >= (_array->_elems + _array->_size)
					? right._offset >= (right._array->_elems + right._array->_size)
					: _offset == right._offset;
			}
			bool operator!= (const iterator& right) const
			{
				return _offset >= (_array->_elems + _array->_size)
					? right._offset < (right._array->_elems + right._array->_size)
					: _offset != right._offset;
			}
			_Ty& operator* () const { return *_offset; }
			_Ty* operator-> () const { return _offset; }

			bool operator== (const const_iterator& right);
			inline bool operator!= (const const_iterator& right) { return !(*this == right); }

			// iterator traits
			using difference_type = std::ptrdiff_t;
			using value_type = _Ty;
			using pointer = _Ty*;
			using reference = _Ty&;
			using iterator_category = std::forward_iterator_tag;
		};

		inline iterator begin() { return { *this, 0 }; }
		inline const_iterator begin() const { return { *this, 0 }; }
		inline const_iterator cbegin() const { return { *this, 0 }; }
		inline iterator end() { return { *this, _size }; }
		inline const_iterator end() const { return { *this, _size }; }
		inline const_iterator cend() const { return { *this, _size }; }
	};

	template<typename _Ty>
	StaticArray<_Ty>::StaticArray(Size size) :
		_elems{ utils::malloc_raw<_Ty>(sizeof(_Ty) * size) },
		_size{ size }
	{}

	template<typename _Ty>
	void StaticArray<_Ty>::_reconstruct(Size size, bool destroy)
	{
		if (destroy)
			_destroy();
		new (this) StaticArray(size);
	}

	template<typename _Ty>
	void StaticArray<_Ty>::_destroy()
	{
		if (_elems)
			delete _elems;
		_elems = nullptr;
		_size = 0;
	}

	template<typename _Ty>
	StaticArray<_Ty>& StaticArray<_Ty>::_copy(const StaticArray<_Ty>& a, bool reset)
	{
		_reconstruct(a._size, reset);
		for (Size idx = 0; idx < a._size; idx++)
			_elems[idx] = a._elems[idx];
		_size = a._size;

		return *this;
	}

	template<typename _Ty>
	StaticArray<_Ty>& StaticArray<_Ty>::_move(StaticArray<_Ty>&& a, bool reset) noexcept
	{
		if (reset)
			_destroy();

		_elems = a._elems;
		_size = a._size;

		a._elems = nullptr;
		a._size = 0;

		return *this;
	}

	template<typename _Ty>
	StaticArray<_Ty>::StaticArray(const std::vector<_Ty>& v) :
		StaticArray{ v.size() }
	{
		const _Ty* v_elems = v.data();
		for (Size idx = 0; idx < _size; idx++, v_elems++)
			_elems[idx] = *v_elems;
	}

	template<typename _Ty>
	StaticArray<_Ty>::StaticArray(std::vector<_Ty>&& v) :
		StaticArray{ v.size() }
	{
		_Ty* v_elems = v.data();
		for (Size idx = 0; idx < _size; idx++, v_elems++)
			_elems[idx] = std::move(*v_elems);
	}

	template<typename _Ty>
	StaticArray<_Ty>::StaticArray(_Ty* buffer, Size size, bool move) :
		StaticArray{ size }
	{
		if (move)
			for (Size idx = 0; idx < size; idx++)
				_elems[idx] = std::move(buffer[idx]);
		else for (Size idx = 0; idx < size; idx++)
			_elems[idx] = buffer[idx];
	}

	template<typename _Ty>
	StaticArray<_Ty> StaticArray<_Ty>::subarray(Size offset, Size length)
	{
		if (offset >= _size)
			return {};

		Size size = std::min(_size - offset, length == 0 ? _size : length);
		return { _elems + offset, size - offset };
	}

	template<typename _Ty>
	void StaticArray<_Ty>::concat(_Ty* buffer, Size size, bool move)
	{
		if (!buffer || !size)
			return;

		_Ty* old = _elems;
		Size old_size = _size;
		_reconstruct(old_size + size, false);

		std::memcpy(_elems, old, sizeof(_Ty) * old_size);
		utils::free_raw(old);

		_Ty* ptr = _elems + old_size;
		if (move)
			for (Size idx = 0; idx < size; idx++)
				ptr[idx] = std::move(buffer[idx]);
		else for (Size idx = 0; idx < size; idx++)
			ptr[idx] = buffer[idx];
	}

	template<typename _Ty>
	inline StaticArray<_Ty> operator+ (const _Ty& left, const StaticArray<_Ty>& right) { return right.concat(left); }



	template<typename _Ty>
	StaticArray<_Ty>::const_iterator::const_iterator(const StaticArray<_Ty>::iterator& it) :
		_array{ it._array },
		_offset{ it._offset }
	{}

	template<typename _Ty>
	typename StaticArray<_Ty>::const_iterator& StaticArray<_Ty>::const_iterator::operator= (const StaticArray<_Ty>::iterator& right)
	{
		_array = right._array;
		_offset = right._offset;
		return *this;
	}

	template<typename _Ty>
	bool StaticArray<_Ty>::const_iterator::operator== (const StaticArray<_Ty>::iterator& right)
	{
		return _offset >= (_array->_elems + _array->_size)
			? right._offset >= (right._array->_elems + right._array->_size)
			: _offset == right._offset;
	}

	template<typename _Ty>
	bool StaticArray<_Ty>::iterator::operator== (const StaticArray<_Ty>::const_iterator& right)
	{
		return _offset >= (_array->_elems + _array->_size)
			? right._offset >= (right._array->_elems + right._array->_size)
			: _offset == right._offset;
	}
}
