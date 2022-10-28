//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_FIXED_VECTOR
#define COGS_HEADER_COLLECTION_FIXED_VECTOR

#include <type_traits>

#include "cogs/math/const_max_int.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @ingroup Collections
/// @brief A resizable array, encapsulating a fixed-size backing buffer.
///
/// A backed_vector is essentially a dynamically allocated array that can be resized.
/// Indexing is constant time.
/// Growing from the end is efficient, buffered by a capacity
/// Inserting anywhere but at the end requires that all subsequent elements are moved forward.
/// Removal from the end is efficient.
/// Removal from elsewhere in the list requires that all subsequent elements are moved back.
/// A const backed_vector extends const-ness to its elements.
/// Move operations are deep moves of elements, then the source backed_vector is cleared of contents.
/// backed_vector is not thread-safe.
/// Const and/or volatile element types are not supported.
/// @tparam T type to contain
/// @tparam sz Fixed maximum number of elements in the vector
template <typename T, size_t sz>
class backed_vector
{
public:
	static_assert(!std::is_const_v<T>);
	static_assert(!std::is_volatile_v<T>);
	static_assert(!std::is_void_v<T>);
	static_assert(sz > 0);

	typedef T type;

	typedef size_t position_t;

private:
	typedef backed_vector<T, sz> this_t;

	typedef T array_t[sz];
	cogs::placement<array_t> m_storage;
	size_t m_length;

	void construct(size_t n)
	{
		if (!!n)
		{
			if (n > sz)
				n = sz;
			m_length = n;
			placement_construct_multiple(get_ptr(), n);
		}
	}

	template <typename type2 = type>
	void construct(size_t n, const type2& src)
	{
		if (!!n)
		{
			if (n > sz)
				n = sz;
			m_length = n;
			placement_construct_multiple(get_ptr(), n, src);
		}
	}

	template <typename type2 = type>
	void construct(size_t n, type2&& src)
	{
		if (!!n)
		{
			if (n > sz)
				n = sz;
			m_length = n;
			size_t lastPosition = n - 1;
			if (lastPosition != 0)
				placement_construct_multiple(get_ptr(), lastPosition, src);
			placement_construct(get_ptr() + lastPosition, std::forward<type2>(src));
		}
	}

	template <typename type2>
	void construct(type2* src, size_t n)
	{
		if (!!n)
		{
			if (n > sz)
				n = sz;
			m_length = n;
			placement_copy_construct_array(get_ptr(), src, n);
		}
	}

	template <typename type2, size_t sz2>
	void construct(const backed_vector<type2, sz2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if constexpr (sz < sz2)
			{
				if (n > sz)
					n = sz;
			}
			m_length = n;
			placement_copy_construct_array(get_ptr(), src.get_const_ptr(), n);
		}
	}

	template <typename type2, size_t sz2>
	void construct(const backed_vector<type2, sz2>& src, size_t i)
	{
		size_t length = src.get_length();
		if (i < length)
		{
			size_t n = length - i;
			if constexpr (sz < sz2)
			{
				if (n > sz)
					n = sz;
			}
			m_length = n;
			placement_copy_construct_array(get_ptr(), src.get_const_ptr() + i, n);
		}
	}

	template <typename type2, size_t sz2>
	void construct(const backed_vector<type2, sz2>& src, size_t i, size_t n)
	{
		size_t length = src.get_length();
		if (i < length)
		{
			size_t remainingLength = length - i;
			if (n > remainingLength)
				n = remainingLength;
			if constexpr (sz < sz2)
			{
				if (n > sz)
					n = sz;
			}
			m_length = n;
			placement_copy_construct_array(get_ptr(), src.get_const_ptr() + i, n);
		}
	}

	template <typename type2, size_t sz2>
	void move_construct(backed_vector<type2, sz2>&& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if constexpr (sz < sz2)
			{
				if (n > sz)
					n = sz;
			}
			m_length = n;
			placement_move_construct_array(get_ptr(), src.get_const_ptr(), n);
			src.clear();
		}
	}

	size_t prepare_assign(size_t n)
	{
		placement_destruct_multiple(get_ptr(), m_length);
		if (n > sz)
			n = sz;
		m_length = n;
		return n;
	}

	size_t prepare_append(size_t n)
	{
		size_t available = sz - m_length;
		if (n > available)
			n = available;
		m_length += n;
		return n;
	}

	// given:
	// n > 0
	// i < m_length
	// i < sz
	size_t insert_inner(size_t i, size_t n)
	{
		type* insertPtr = get_ptr() + i;
		size_t numMoving = m_length - i;
		if (i + n >= sz)
		{
			placement_destruct_multiple(insertPtr, numMoving);
			m_length = sz;
			return sz - i;
		}
		m_length += n;
		if (m_length <= sz)
			placement_move(insertPtr + n, insertPtr, numMoving);
		else
		{
			size_t trailing = m_length - sz;
			m_length = sz;
			numMoving -= trailing;
			placement_destruct_multiple(insertPtr + numMoving, trailing);
			placement_move(insertPtr + n, insertPtr, numMoving);
		}
		return n;
	}

	// given:
	// n > 0
	// replaceLength > 0
	// i < m_length
	// i < sz
	size_t replace_inner(size_t i, size_t replaceLength, size_t n)
	{
		type* insertPtr = get_ptr() + i;
		size_t adjustedLength = m_length - i; // constructed elements beyond the insert index
		size_t adjustedSize = sz - i;
		if (n >= adjustedSize)
		{
			placement_destruct_multiple(insertPtr, adjustedLength);
			m_length = sz;
			return adjustedSize;
		}
		if (replaceLength > adjustedLength)
			replaceLength = adjustedLength;
		size_t newLength = (m_length + n) - replaceLength;
		if (replaceLength == adjustedLength || n == replaceLength)
			placement_destruct_multiple(insertPtr, adjustedLength);
		else
		{
			size_t numMoving = adjustedLength - replaceLength;
			type* dst = insertPtr + n;
			type* src = insertPtr + replaceLength;
			if (n < replaceLength)
			{
				placement_copy_reconstruct_array(dst, src, numMoving);
				placement_destruct_multiple(dst + numMoving, replaceLength - n);
			}
			else // if (n > replaceLength)
			{
				if (newLength > sz)
				{
					size_t trailing = newLength - sz;
					newLength = sz;
					numMoving -= trailing;
					placement_destruct_multiple(insertPtr + numMoving, trailing);
				}
				placement_move(dst, src, numMoving);
			}
		}
		m_length = newLength;
		return n;
	}

	void clear_inner()
	{
		placement_destruct_multiple(get_ptr(), m_length);
	}

public:
	/// @brief A backed_vector element iterator
	class iterator
	{
	protected:
		this_t* m_array;
		size_t m_index;

		iterator(this_t* v, size_t i)
			: m_array(v),
			m_index(i)
		{ }

	public:
		iterator() { }
		iterator(const iterator& i) : m_array(i.m_array), m_index(i.m_index) { }

		void release() { m_array = 0; }

		iterator& operator++()
		{
			if (m_array)
			{
				m_index++;
				if (m_index >= m_array->get_length())
					m_array = 0;
			}

			return *this;
		}

		iterator& operator--()
		{
			if (m_array)
			{
				if (m_index == 0)
					m_array = 0;
				else
					--m_index;
			}

			return *this;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_array || (m_index >= m_array->get_length()); }

		bool operator==(const iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_index == i.m_index)); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		iterator& operator=(const iterator& i) { m_array = i.m_array; m_index = i.m_index; return *this; }

		type* get() const { return (m_array->get_ptr() + m_index); }
		type& operator*() const { return *get(); }
		type* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }
	};

	iterator get_first_iterator() { return iterator(!!m_length ? this : 0, 0); }
	iterator get_last_iterator() { return iterator(!!m_length ? this : 0, m_length - 1); }

	/// @brief A backed_vector constant element iterator
	class const_iterator
	{
	protected:
		const this_t* m_array;
		size_t m_index;

		const_iterator(const this_t* v, size_t i)
			: m_array(v),
			m_index(i)
		{ }

	public:
		const_iterator() { }
		const_iterator(const const_iterator& i) : m_array(i.m_array), m_index(i.m_index) { }
		const_iterator(const iterator& i) : m_array(i.m_array), m_index(i.m_index) { }

		void release() { m_array = 0; }

		const_iterator& operator++()
		{
			if (m_array)
			{
				m_index++;
				if (m_index >= m_array->get_length())
					m_array = 0;
			}

			return *this;
		}

		const_iterator& operator--()
		{
			if (m_array)
			{
				if (m_index == 0)
					m_array = 0;
				else
					--m_index;
			}

			return *this;
		}

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_array || (m_index >= m_array->get_length()); }

		bool operator==(const const_iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_index == i.m_index)); }
		bool operator==(const iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_index == i.m_index)); }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_array = i.m_array; m_index = i.m_index; return *this; }
		const_iterator& operator=(const iterator& i) { m_array = i.m_array; m_index = i.m_index; return *this; }

		const type* get() const { return m_array->get_const_ptr() + m_index; }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }
	};

	const_iterator get_first_const_iterator() const { return const_iterator(!!m_length ? this : 0, 0); }
	const_iterator get_last_const_iterator() const { return const_iterator(!!m_length ? this : 0, m_length - 1); }

	backed_vector()
		: m_length(0)
	{ }

	backed_vector(this_t&& src)
		: m_length(src.m_length)
	{
		placement_move_construct_array(get_ptr(), src.get_ptr(), src.m_length);
		src.clear();
	}

	backed_vector(const this_t& src)
		: m_length(0)
	{
		construct(src);
	}

	explicit backed_vector(size_t n)
		: m_length(0)
	{
		construct(n);
	}

	template <typename type2 = type>
	backed_vector(size_t n, const type2& src)
		: m_length(0)
	{
		construct(n, src);
	}

	template <typename type2 = type>
	backed_vector(size_t n, type2&& src)
		: m_length(0)
	{
		construct(n, std::forward<type2>(src));
	}

	template <typename type2>
	backed_vector(type2* src, size_t n)
		: m_length(0)
	{
		construct(src, n);
	}

	template <typename type2, size_t sz2>
	backed_vector(const backed_vector<type2, sz2>&& src)
		: m_length(0)
	{
		move_construct(std::move(src));
	}

	template <typename type2, size_t sz2>
	backed_vector(const backed_vector<type2, sz2>& src)
		: m_length(0)
	{
		construct(src);
	}

	template <typename type2, size_t sz2>
	backed_vector(const backed_vector<type2, sz2>& src, size_t i)
		: m_length(0)
	{
		construct(src, i);
	}

	template <typename type2, size_t sz2>
	backed_vector(const backed_vector<type2, sz2>& src, size_t i, size_t n)
		: m_length(0)
	{
		construct(src, i, n);
	}

	~backed_vector()
	{
		clear_inner();
	}

	this_t& operator=(const type& src) { assign(1, src); return *this; }

	this_t& operator=(const this_t& src) { assign(src); return *this; }

	this_t& operator=(this_t&& src)
	{
		clear_inner();
		m_length = src.get_length();
		placement_move_construct_array(get_ptr(), src.get_ptr(), src.m_length);
		src.clear();
		return *this;
	}

	template <typename type2, size_t sz2>
	this_t& operator=(const backed_vector<type2, sz2>& src) { assign(src); return *this; }

	template <typename type2, size_t sz2>
	this_t& operator=(backed_vector<type2, sz2>&& src)
	{
		clear_inner();
		move_construct(std::move(src));
		return *this;
	}

	size_t get_length() const { return m_length; }
	constexpr size_t get_capacity() { return sz; }

	bool is_empty() const { return m_length == 0; }
	bool operator!() const { return m_length == 0; }

	const type* get_const_ptr() const { return m_storage.get(); }

	const type& operator[](size_t i) const { return get_const_ptr()[i]; }

	const type& get_first_const() const { return *(get_const_ptr()); }
	const type& get_last_const() const { return *((get_const_ptr() + m_length) - 1); }

	// Subranges of this type would seem to require something like a vector_view or array_view
	//const vector_view<type>& subrange(size_t i, backed_vector<type>& storage = vector_view<type>()) const
	//{
	//}
	//
	//const vector_view<type>& subrange(size_t i, size_t n, backed_vector<type>& storage = vector_view<type>()) const
	//{
	//}

	template <typename type2>
	bool equals(size_t n, type2& cmp) const
	{
		bool result = false;
		if (m_length == n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (!(get_const_ptr()[i] == cmp))
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2>
	bool equals(type2* cmp, size_t n) const
	{
		bool result = false;
		if (m_length == n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (!(get_const_ptr()[i] == cmp[i]))
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, size_t sz2>
	bool equals(const backed_vector<type2, sz2>& cmp) const
	{
		return equals(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	bool equals(const backed_vector<type2, sz2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !*this;
		return equals(cmp.get_const_ptr(), length);
	}

	template <typename type2, size_t sz2>
	bool equals(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (!n || i >= length)
			return !*this;
		return equals(cmp.get_const_ptr(), cmp.get_length());
	}

	bool operator==(const this_t& cmp) const { return equals(cmp); }

	template <typename type2, size_t sz2>
	bool operator==(const backed_vector<type2, sz2>& cmp) const { return equals(cmp); }

	bool operator!=(const this_t& cmp) const { return !equals(cmp); }

	template <typename type2, size_t sz2>
	bool operator!=(const backed_vector<type2, sz2>& cmp) const { return !equals(cmp); }

	template <typename type2>
	bool starts_with(size_t n, type2& cmp) const
	{
		bool result = false;
		if (!!n && m_length >= n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (get_const_ptr()[i] != cmp)
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2>
	bool starts_with(type2* cmp, size_t n) const
	{
		bool result = false;
		if (!!n && m_length >= n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (get_const_ptr()[i] != cmp[i])
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, size_t sz2>
	bool starts_with(const backed_vector<type2, sz2>& cmp) const
	{
		return starts_with(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	bool starts_with(const backed_vector<type2, sz2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return starts_with(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, size_t sz2>
	bool starts_with(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return starts_with(cmp.get_const_ptr() + i, n);
	}

	template <typename type2>
	bool ends_with(size_t n, type2& cmp) const
	{
		bool result = false;
		if (!!n && m_length >= n)
		{
			size_t endIndex = m_length - n;
			result = true;
			for (size_t i = m_length; ; )
			{
				i--;
				if (get_const_ptr()[i] != cmp)
				{
					result = false;
					break;
				}
				if (i == endIndex)
					break;
			}
		}
		return result;
	}

	template <typename type2>
	bool ends_with(type2* cmp, size_t n) const
	{
		bool result = false;
		if (!!n && m_length >= n)
		{
			size_t endIndex = m_length - n;
			result = true;
			for (size_t i = m_length; ; )
			{
				i--;
				if (get_const_ptr()[i] != cmp[i])
				{
					result = false;
					break;
				}
				if (i == endIndex)
					break;
			}
		}
		return result;
	}

	template <typename type2, size_t sz2>
	bool ends_with(const backed_vector<type2, sz2>& cmp) const
	{
		return ends_with(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	bool ends_with(const backed_vector<type2, sz2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return ends_with(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, size_t sz2>
	bool ends_with(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return ends_with(cmp.get_const_ptr() + i, n);
	}

	template <typename type2>
	int compare(type2* cmp, size_t n) const
	{
		if (!m_length)
			return !!n ? -1 : 0;
		bool cmpIsLonger = (n > m_length);
		size_t shorterLength = cmpIsLonger ? m_length : n;
		size_t i = 0;
		do {
			const type& value = get_const_ptr()[i];
			const type& cmpValue = cmp[i];
			if (value < cmpValue)
				return -1;
			if (value > cmpValue)
				return 1;
		} while (++i != shorterLength);
		return !!cmpIsLonger ? -1 : ((n == m_length) ? 0 : 1);
	}

	template <typename type2, size_t sz2>
	int compare(const backed_vector<type2, sz2>& cmp) const
	{
		return compare(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2>
	bool is_less_than(type2* cmp, size_t n) const
	{
		bool result;
		if (!m_length)
			result = !!n;
		else
		{
			result = false;
			bool cmpIsLonger = (n > m_length);
			size_t shorterLength = cmpIsLonger ? m_length : n;
			size_t i = 0;
			for (;;)
			{
				const type& value = get_const_ptr()[i];
				type2& cmpValue = cmp[i];
				if (value < cmpValue)
				{
					result = true;
					break;
				}
				if (value > cmpValue)
					break;
				if (++i == shorterLength)
				{
					result = cmpIsLonger;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, size_t sz2>
	bool is_less_than(const backed_vector<type2, sz2>& cmp) const
	{
		return is_less_than(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	bool is_less_than(const backed_vector<type2, sz2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return is_less_than(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, size_t sz2>
	bool is_less_than(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return is_less_than(cmp.get_const_ptr() + i, n);
	}

	template <typename type2, size_t sz2>
	bool operator<(const backed_vector<type2, sz2>& cmp) const { return is_less_than(cmp); }

	template <typename type2, size_t sz2>
	bool operator>=(const backed_vector<type2, sz2>& cmp) const { return !is_less_than(cmp); }

	template <typename type2>
	bool is_greater_than(type2* cmp, size_t n) const
	{
		bool result;
		if (!n)
			result = !!m_length;
		else
		{
			result = false;
			bool cmpIsShorter = (n < m_length);
			size_t shorterLength = cmpIsShorter ? n : m_length;
			size_t i = 0;
			for (;;)
			{
				const type& value = get_const_ptr()[i];
				type2& cmpValue = cmp[i];
				if (value > cmpValue)
				{
					result = true;
					break;
				}
				if (value < cmpValue)
					break;
				if (++i == shorterLength)
				{
					result = cmpIsShorter;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, size_t sz2>
	bool is_greater_than(const backed_vector<type2, sz2>& cmp) const
	{
		return is_greater_than(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	bool is_greater_than(const backed_vector<type2, sz2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !!m_length;
		return is_greater_than(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, size_t sz2>
	bool is_greater_than(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !!m_length;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return is_greater_than(cmp.get_const_ptr() + i, n);
	}

	template <typename type2, size_t sz2>
	bool operator>(const backed_vector<type2, sz2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2, size_t sz2>
	bool operator<=(const backed_vector<type2, sz2>& cmp) const { return !is_greater_than(cmp); }

	template <typename type2>
	size_t index_of(const type2& cmp) const { return index_of(0, cmp); }

	template <typename type2>
	size_t index_of(size_t i, const type2& cmp) const
	{
		size_t length = m_length;
		for (; (i < length); i++)
			if (get_const_ptr()[i] == cmp)
				return i;
		return const_max_int_v<size_t>;
	}

	template <typename type2>
	size_t index_of_any(const type2* cmp, size_t n) const { return index_of_any(0, cmp, n); }

	template <typename type2>
	size_t index_of_any(size_t i, const type2* cmp, size_t n) const
	{
		if (n == 1)
			return index_of(i, *cmp);

		size_t length = m_length;
		for (size_t i2 = i; i2 < length; i2++)
		{
			type& t = get_const_ptr()[i2];
			for (size_t i3 = 0; i3 < n; i3++)
			{
				if (t == cmp[i3])
					return i2;
			}
		}
		return const_max_int_v<size_t>;
	}

	template <typename type2>
	size_t index_of_segment(type2* cmp, size_t n) const { return index_of_segment(0, cmp, n); }

	template <typename type2, size_t sz2>
	size_t index_of_segment(const backed_vector<type2, sz2>& cmp) const { return index_of_segment(0, cmp); }

	template <typename type2, size_t sz2>
	size_t index_of_segment(const backed_vector<type2, sz2>& cmp, size_t i) const { return index_of_segment(0, cmp, i); }

	template <typename type2, size_t sz2>
	size_t index_of_segment(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const { return index_of_segment(0, cmp, i, n); }

	template <typename type2>
	size_t index_of_segment(size_t i, const type2* cmp, size_t n) const
	{
		if (!!n && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (n <= remainingLength)
			{
				size_t endPos = m_length - n;
				while (i < endPos)
				{
					if (get_const_ptr()[i] == cmp[0])
					{
						bool isMatch = true;
						for (size_t i2 = 1; i2 < n; i2++)
						{
							if (get_const_ptr()[i + i2] != cmp[i2])
							{
								isMatch = false;
								break;
							}
						}
						if (isMatch)
							return i;
					}
					++i;
				}
			}
		}
		return const_max_int_v<size_t>;
	}


	template <typename type2, size_t sz2>
	size_t index_of_segment(size_t i, const backed_vector<type2, sz2>& cmp) const
	{
		return index_of_segment(i, cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, size_t sz2>
	size_t index_of_segment(size_t i, const backed_vector<type2, sz2>& cmp, size_t i2) const
	{
		size_t cmpLength = cmp.get_length();
		if (i2 >= cmpLength)
			return const_max_int_v<size_t>;
		return index_of_segment(i, cmp.get_const_ptr() + i2, cmpLength - i2);
	}

	template <typename type2, size_t sz2>
	size_t index_of_segment(size_t i, const backed_vector<type2, sz2>& cmp, size_t i2, size_t n) const
	{
		size_t cmpLength = cmp.get_length();
		if (i2 >= cmpLength)
			return const_max_int_v<size_t>;
		size_t remainingLength = cmpLength - i2;
		if (n > remainingLength)
			n = remainingLength;
		return index_of_segment(i, cmp.get_const_ptr() + i2, n);
	}

	template <typename type2>
	bool contains(type2& cmp) const { return index_of_segment(cmp) != const_max_int_v<size_t>; }

	template <typename type2>
	bool contains_any(type2* cmp, size_t n) const { return index_of_any(cmp, n) != const_max_int_v<size_t>; }

	template <typename type2>
	bool contains_segment(type2* cmp, size_t n) const { return index_of_segment(cmp, n) != const_max_int_v<size_t>; }

	template <typename type2, size_t sz2>
	bool contains_segment(const backed_vector<type2, sz2>& cmp) const { return index_of_segment(cmp) != const_max_int_v<size_t>; }

	template <typename type2, size_t sz2>
	bool contains_segment(const backed_vector<type2, sz2>& cmp, size_t i) const { return index_of_segment(cmp, i) != const_max_int_v<size_t>; }

	template <typename type2, size_t sz2>
	bool contains_segment(const backed_vector<type2, sz2>& cmp, size_t i, size_t n) const { return index_of_segment(cmp, i, n) != const_max_int_v<size_t>; }

	type* get_ptr() { return m_storage.get(); }
	type& get_first() { return get_ptr()[0]; }
	type& get_last() { return get_ptr()[m_length - 1]; }

	template <typename type2>
	void set_index(size_t i, type2& src) { get_ptr()[i] = src; }

	void reverse()
	{
		if (m_length > 1)
		{
			size_t lengthMinusOne = m_length - 1;
			for (size_t i = 0; i < m_length/2; i++)
			{
				type* p = get_ptr();
				type tmp = p[i];
				size_t alternatePosition = lengthMinusOne - i;
				p[i] = p[alternatePosition];
				p[alternatePosition] = tmp;
			}
		}
	}

	void set_to_subrange(size_t startIndex)
	{
		erase(0, startIndex);
	}

	void set_to_subrange(size_t startIndex, size_t n)
	{
		truncate_to(startIndex + n);
		erase(0, startIndex);
	}

	// Clears contents, but retains reserved space
	void clear()
	{
		clear_inner();
		m_length = 0;
	}

	template <typename type2 = type>
	void assign(size_t n, const type2& src)
	{
		if (!n)
			clear();
		else
			placement_construct_multiple(get_ptr(), prepare_assign(n), src);
	}

	template <typename type2 = type>
	void assign(size_t n, type2&& src)
	{
		if (!n)
			clear();
		else
		{
			size_t lastPosition = prepare_assign(n) - 1;
			if (lastPosition != 0)
				placement_construct_multiple(get_ptr(), lastPosition, src);
			placement_construct(get_ptr() + lastPosition, std::forward<type2>(src));
		}
	}

	template <typename type2>
	void assign(type2* src, size_t n)
	{
		if (!n)
			clear();
		else
			placement_copy_construct_array(get_ptr(), src, prepare_assign(n));
	}

	void assign(const this_t& src)
	{
		if (this != &src)
		{
			size_t n = src.get_length();
			if (!n)
				clear();
			else
				placement_copy_construct_array(get_ptr(), src.get_const_ptr(), prepare_assign(n));
		}
	}

	template <typename type2, size_t sz2>
	void assign(const backed_vector<type2, sz2>& src)
	{
		size_t n = src.m_length;
		if (!n)
			clear();
		else
			placement_copy_construct_array(get_ptr(), src.get_const_ptr(), prepare_assign(n));
	}

	template <typename type2, size_t sz2>
	void assign(const backed_vector<type2, sz2>& src, size_t i)
	{
		size_t length = src.m_length;
		if (i >= src)
			clear();
		else
			placement_copy_construct_array(get_ptr(), src.get_const_ptr() + i, prepare_assign(length - i));
	}

	template <typename type2, size_t sz2>
	void assign(const backed_vector<type2, sz2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			size_t length = src.m_length;
			if (i < length)
			{
				size_t remainingLength = length - i;
				if (n > remainingLength)
					n = remainingLength;
				;
				placement_copy_construct_array(get_ptr(), src.get_const_ptr() + i, prepare_assign(n));
				return;
			}
		}
		clear();
	}

	void append(size_t n = 1)
	{
		if (!!n)
		{
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			placement_construct_multiple(dst, prepare_append(n));
		}
	}

	template <typename type2 = type>
	void append(size_t n, const type2& src)
	{
		if (!!n)
		{
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			placement_construct_multiple(dst, prepare_append(n), src);
		}
	}

	template <typename type2 = type>
	void append(size_t n, type2&& src)
	{
		if (!!n)
		{
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			size_t lastPosition = prepare_append(n) - 1;
			if (lastPosition != 0)
				placement_construct_multiple(dst, lastPosition, src);
			placement_construct(dst + lastPosition, std::forward<type2>(src));
		}
	}

	template <typename type2>
	void append(type2* src, size_t n)
	{
		if (!!n)
		{
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			placement_copy_construct_array(dst, src, prepare_append(n));
		}
	}

	template <typename type2, size_t sz2>
	void append(const backed_vector<type2, sz2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			placement_copy_construct_array(dst, src.get_const_ptr(), prepare_append(n));
		}
	}

	template <typename type2, size_t sz2>
	void append(const backed_vector<type2, sz2>& src, size_t i)
	{
		size_t n = src.get_length();
		if (i < n)
		{
			n -= i;
			type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
			placement_copy_construct_array(dst, src.get_const_ptr() + i, prepare_append(n));
		}
	}

	template <typename type2, size_t sz2>
	void append(const backed_vector<type2, sz2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			size_t srcLength = src.get_length();
			if (i < srcLength)
			{
				size_t remainingLength = srcLength - i;
				if (n > remainingLength)
					n = remainingLength;
				type* dst = get_ptr() + m_length; // Must use m_length outside of args as it's also modified by another arg.
				placement_copy_construct_array(dst, src.get_const_ptr() + i, prepare_append(n));
			}
		}
	}

	this_t& operator+=(const type& src) { append(1, src); return *this; }

	template <typename type2, size_t sz2>
	this_t& operator+=(const backed_vector<type2, sz2>& src) { append(src); return *this; }

	void resize(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			size_t numToDestruct = m_length - n;
			m_length = n;
			placement_destruct_multiple(get_ptr() + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			if (m_length < sz)
			{
				if (n > sz)
					n = sz;
				size_t numToConstruct = n - m_length;
				placement_construct_multiple(get_ptr() + m_length, numToConstruct);
				m_length = n;
			}
		}
	}

	template <typename type2 = type>
	void resize(size_t n, const type2& src)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			size_t numToDestruct = m_length - n;
			m_length = n;
			placement_destruct_multiple(get_ptr() + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			if (m_length < sz)
			{
				if (n > sz)
					n = sz;
				size_t numToConstruct = n - m_length;
				placement_construct_multiple(get_ptr() + m_length, numToConstruct, src);
				m_length = n;
			}
		}
	}

	template <typename type2 = type>
	void resize(size_t n, type2&& src)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			size_t numToDestruct = m_length - n;
			m_length = n;
			placement_destruct_multiple(get_ptr() + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			if (m_length < sz)
			{
				if (n > sz)
					n = sz;
				size_t lastPosition = (n - m_length) - 1;
				type* dst = get_ptr() + m_length;
				if (lastPosition != 0)
					placement_construct_multiple(dst, lastPosition, src);
				placement_construct(dst + lastPosition, std::forward<type2>(src));
				m_length = n;
			}
		}
	}

	void erase(size_t i)
	{
		if (i < m_length)
		{
			m_length--;
			type* erasePositionPtr = get_ptr() + i;
			placement_destruct(erasePositionPtr);
			if (i < m_length)
				placement_move(erasePositionPtr, erasePositionPtr + 1, m_length - i);
		}
	}

	void erase(size_t i, size_t n)
	{
		if (!!n && (i < m_length))
		{
			size_t normalizedLength = m_length - i;
			type* erasePositionPtr = get_ptr() + i;
			placement_destruct_multiple(erasePositionPtr, normalizedLength);
			if (n >= normalizedLength)
				m_length = i;
			else // if (n < normalizedLength)
			{
				placement_move(erasePositionPtr, erasePositionPtr + n, normalizedLength - n);
				m_length -= n;
			}
		}
	}

	void truncate_to(size_t n)
	{
		if (!n)
			clear();
		if (n < m_length)
		{
			size_t numToDestruct = m_length - n;
			m_length = n;
			placement_destruct_multiple(get_ptr() + m_length, numToDestruct);
		}
	}

	void truncate(size_t n)
	{
		if (n >= m_length)
			clear();
		else
			truncate_to(m_length - n);
	}

	void insert(size_t i, size_t n)
	{
		if (!!n)
		{
			if (i >= m_length) // if the insert index is past the end, treat it as an append
				append(n);
			else
				placement_construct_multiple(get_ptr() + i, insert_inner(i, n));
		}
	}

	template <typename type2 = type>
	void insert(size_t i, size_t n, const type2& src)
	{
		if (!!n)
		{
			if (i >= m_length) // if the insert index is past the end, treat it as an append
				append(n, src);
			else
				placement_construct_multiple(get_ptr() + i, insert_inner(i, n), src);
		}
	}

	template <typename type2 = type>
	void insert(size_t i, size_t n, type2&& src)
	{
		if (!!n)
		{
			if (i >= m_length) // if the insert index is past the end, treat it as an append
				append(n, src);
			else
			{
				size_t lastPosition = insert_inner(i, n) - 1;
				type* dst = get_ptr() + i;
				if (lastPosition != 0)
					placement_construct_multiple(dst, lastPosition, src);
				placement_construct(dst + lastPosition, std::forward<type2>(src));
			}
		}
	}

	template <typename type2>
	void insert(size_t i, type2* src, size_t n)
	{
		if (!!n)
		{
			if (i >= m_length) // if the insert index is past the end, treat it as an append
				append(src, n);
			else
				placement_copy_construct_array(get_ptr() + i, src, insert_inner(i, n));
		}
	}

	template <typename type2, size_t sz2>
	void insert(size_t i, const backed_vector<type2, sz2>& src)
	{
		insert(i, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, size_t sz2>
	void insert(size_t i, const backed_vector<type2, sz2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			insert(i, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
	}

	template <typename type2, size_t sz2>
	void insert(size_t i, const backed_vector<type2, sz2>& src, size_t srcIndex, size_t n)
	{
		if (!!n)
		{
			size_t srcLength = src.get_length();
			if (srcIndex < srcLength)
			{
				size_t remainingLength = srcLength - srcIndex;
				if (n > remainingLength)
					n = remainingLength;
				insert(i, src.get_const_ptr() + srcIndex, n);
			}
		}
	}

	template <typename type2>
	void replace(size_t i, size_t replaceLength, type2& src)
	{
		if (!!replaceLength && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (replaceLength > remainingLength)
				replaceLength = remainingLength;
			placement_reconstruct_multiple(get_ptr() + i, replaceLength, src);
		}
	}

	template <typename type2>
	void replace(size_t i, type2* src, size_t replaceLength)
	{
		if (!!replaceLength && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (replaceLength > remainingLength)
				replaceLength = remainingLength;
			placement_copy_reconstruct_array(get_ptr() + i, src, replaceLength);
		}
	}

	template <typename type2, size_t sz2>
	void replace(size_t i, const backed_vector<type2, sz2>& src)
	{
		replace(i, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, size_t sz2>
	void replace(size_t i, const backed_vector<type2, sz2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			replace(i, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
	}

	template <typename type2, size_t sz2>
	void replace(size_t i, const backed_vector<type2, sz2>& src, size_t srcIndex, size_t n)
	{
		if (!!n)
		{
			size_t srcLength = src.get_length();
			if (srcIndex < srcLength)
			{
				size_t remainingLength = srcLength - srcIndex;
				if (n > remainingLength)
					n = remainingLength;
				replace(i, src.get_const_ptr() + srcIndex, n);
			}
		}
	}


	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, type2& src)
	{
		if (!insertLength)
			erase(i, replaceLength);
		else if (!replaceLength)
			insert(i, insertLength, src);
		else if (i >= m_length) // If index is past the end, do an append instead.  Includes !m_length
			append(insertLength, src);
		else
			placement_construct_multiple(get_ptr() + i, replace_inner(i, replaceLength, insertLength), src);
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, type2* src, size_t insertLength)
	{
		if (!insertLength)
			erase(i, replaceLength);
		else if (!replaceLength)
			insert(i, insertLength, src);
		else if (i >= m_length) // If index is past the end, do an append instead.  Includes !m_length
			append(insertLength, src);
		else
			placement_copy_construct_array(get_ptr() + i, src, replace_inner(i, replaceLength, insertLength));
	}

	template <typename type2, size_t sz2>
	void insert_replace(size_t i, size_t replaceLength, const backed_vector<type2, sz2>& src)
	{
		insert_replace(i, replaceLength, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, size_t sz2>
	void insert_replace(size_t i, size_t replaceLength, const backed_vector<type2, sz2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			insert_replace(i, replaceLength, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
		else
			erase(i, replaceLength);
	}

	template <typename type2, size_t sz2>
	void insert_replace(size_t i, size_t replaceLength, const backed_vector<type2, sz2>& src, size_t srcIndex, size_t n)
	{
		if (!!n)
		{
			size_t srcLength = src.get_length();
			if (srcIndex < srcLength)
			{
				size_t remainingLength = srcLength - srcIndex;
				if (n > remainingLength)
					n = remainingLength;
				insert_replace(i, replaceLength, src.get_const_ptr() + srcIndex, n);
				return;
			}
		}
		erase(i, replaceLength);
	}

	iterator begin() const { return get_first_iterator(); }
	iterator rbegin() const { return get_last_iterator(); }
	iterator end() const { iterator i; return i; }
	iterator rend() const { iterator i; return i; }
};


}


#endif
