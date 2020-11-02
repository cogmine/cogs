//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_SIMPLE_VECTOR
#define COGS_HEADER_COLLECTION_SIMPLE_VECTOR

#include <type_traits>

#include "cogs/math/const_max_int.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @ingroup Collections
/// @brief A resizable dynamically allocated array.  Does not share buffer, so copies are deep.
///
/// A simple_vector is essentially a dynamically allocated array that can be resized.
/// Indexing is constant time.
/// Growing from the end is efficient, buffered by a capacity
/// Inserting anywhere but at the end requires that all subsequent elements are moved forward.
/// Removal from the end is efficient.
/// Removal from elsewhere in the list requires that all subsequent elements are moved back.
/// A const simple_vector extends const-ness to its elements.
/// Move operations transfer ownership of the buffer, so the source simple_vector gets cleared.
/// simple_vector is not thread-safe.
/// Const and/or volatile element types are not supported.
/// @tparam T type to contain
/// @tparam allocator_type Type of allocator to use to allocate the vector's buffer.  Default: default_allocator
template <typename T, class allocator_t = default_allocator<T>>
class simple_vector
{
public:
	static_assert(!std::is_const_v<T>);
	static_assert(!std::is_volatile_v<T>);
	static_assert(!std::is_void_v<T>);

	typedef T type;
	typedef allocator_t allocator_type;
	typedef simple_vector<type, allocator_type> this_t;

	typedef size_t position_t;

private:
	ptr<type> m_ptr;
	size_t m_length;
	size_t m_capacity;

	allocator_type m_allocator;

	void construct(size_t n)
	{
		if (!!n)
		{
			m_length = n;
			m_ptr = m_allocator.allocate(n, m_capacity);
			placement_construct_multiple(m_ptr.get_ptr(), n);
		}
	}

	template <typename type2 = type>
	void construct(size_t n, const type2& src)
	{
		if (!!n)
		{
			m_length = n;
			m_ptr = m_allocator.allocate(n, m_capacity);
			placement_construct_multiple(m_ptr.get_ptr(), n, src);
		}
	}

	template <typename type2 = type>
	void construct(size_t n, type2&& src)
	{
		if (!!n)
		{
			m_length = n;
			m_ptr = m_allocator.allocate(n, m_capacity);
			size_t lastPosition = n - 1;
			if (lastPosition != 0)
				placement_construct_multiple(m_ptr.get_ptr(), lastPosition, src);
			placement_construct(m_ptr.get_ptr() + lastPosition, std::forward<type2>(src));
		}
	}

	template <typename type2>
	void construct(type2* src, size_t n)
	{
		if (!!n)
		{
			m_length = n;
			m_ptr = m_allocator.allocate(n, m_capacity);
			placement_copy_construct_array(m_ptr.get_ptr(), src, n);
		}
	}

	template <typename type2, class allocator_type2>
	void construct(const simple_vector<type2, allocator_type2>& src)
	{
		construct(src.get_const_ptr(), src.get_length());
	}

	template <typename type2, class allocator_type2>
	void construct(const simple_vector<type2, allocator_type2>& src, size_t i)
	{
		size_t length = src.get_length();
		if (i < length)
			construct(src.get_const_ptr() + i, length - i);
	}

	template <typename type2, class allocator_type2>
	void construct(const simple_vector<type2, allocator_type2>& src, size_t i, size_t n)
	{
		size_t length = src.get_length();
		if (i < length)
		{
			size_t remainingLength = length - i;
			if (n > remainingLength)
				n = remainingLength;
			construct(src.get_const_ptr() + i, n);
		}
	}

	void prepare_assign(size_t newLength)
	{
		placement_destruct_multiple(m_ptr.get_ptr(), m_length);
		if ((newLength > m_capacity) && (!try_reallocate(newLength)))
		{
			size_t newCapacity = newLength * 2; // pad capacity
			m_allocator.destruct_deallocate(m_ptr.get_ptr());
			m_ptr = m_allocator.allocate(newCapacity, m_capacity);
		}
		m_length = newLength;
	}

	void insert_inner(size_t i, size_t n)
	{
		size_t numMoving = m_length - i;
		size_t newLength = m_length + n;
		ptr<type> insertPtr = m_ptr + i;
		if ((newLength <= m_capacity) || (!!try_reallocate(newLength)))
			placement_move(insertPtr + n, insertPtr, numMoving);
		else
		{
			size_t capacity;
			ptr<type> newPtr = m_allocator.allocate(newLength, capacity);
			placement_copy_construct_array(newPtr, m_ptr, i);
			placement_copy_construct_array(newPtr + i + n, insertPtr, numMoving);
			clear_inner();
			m_capacity = capacity;
			m_ptr = newPtr;
		}
		m_length = newLength;
	}

	void replace_inner(size_t i, size_t replaceLength, size_t n)
	{
		type* insertPtr = m_ptr + i;
		size_t adjustedLength = m_length - i; // constructed elements beyond the insert index
		if (replaceLength > adjustedLength) // Can't replace more than exists
			replaceLength = adjustedLength;
		if (n == replaceLength) // No resizing necessary
			placement_destruct_multiple(insertPtr, n);
		else
		{
			size_t lengthWithoutReplaced = m_length - replaceLength;
			size_t newLength = lengthWithoutReplaced + n;
			size_t numMoving = adjustedLength - replaceLength; // i.e. lengthWithoutReplaced - i;
			type* src = insertPtr + replaceLength;
			if ((newLength > m_capacity) && (!try_reallocate(newLength)))
			{ // need to reallocate
				size_t capacity;
				type* newPtr = m_allocator.allocate(newLength, capacity);
				placement_copy_construct_array(newPtr, m_ptr, i);
				placement_copy_construct_array(newPtr + i + n, src, numMoving);
				clear_inner();
				m_capacity = capacity;
				m_ptr = newPtr;
			}
			else
			{
				type* dst = insertPtr + n;
				if (n < replaceLength)
				{
					placement_copy_reconstruct_array(dst, src, numMoving);
					placement_destruct_multiple(dst + numMoving, replaceLength - n);
				}
				else // if (n > replaceLength)
					placement_move(dst, src, numMoving);
			}
			m_length = newLength;
		}
	}

	void clear_inner()
	{
		if (m_capacity > 0)
			m_allocator.destruct_deallocate(m_ptr.get_ptr(), m_length);
	}

	void set_temporary(type* ptr, size_t n)
	{
		m_ptr = ptr;
		m_length = n;
	}

	bool try_reallocate(size_t n)
	{
		if (m_allocator.template try_reallocate_type<type>(m_ptr.get_ptr(), n))
		{
			m_capacity = n;
			return true;
		}

		return false;
	}

public:
	/// @brief A simple_vector element iterator
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

	/// @brief A simple_vector constant element iterator
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

	simple_vector()
		: m_length(0),
		m_capacity(0)
	{ }

	simple_vector(this_t&& src)
		: m_allocator(std::move(src.m_allocator)),
		m_length(src.m_length),
		m_capacity(src.m_capacity),
		m_ptr(src.m_ptr)
	{
		src.m_length = 0;
		src.m_capacity = 0;
	}

	simple_vector(const this_t& src)
		: m_length(0),
		m_capacity(0)
	{
		construct(src);
	}

	explicit simple_vector(size_t n)
		: m_length(0),
		m_capacity(0)
	{
		construct(n);
	}

	template <typename type2 = type>
	simple_vector(size_t n, const type2& src)
		: m_length(0),
		m_capacity(0)
	{
		construct(n, src);
	}

	template <typename type2 = type>
	simple_vector(size_t n, type2&& src)
		: m_length(0),
		m_capacity(0)
	{
		construct(n, std::forward<type2>(src));
	}

	template <typename type2>
	simple_vector(type2* src, size_t n)
		: m_length(0),
		m_capacity(0)
	{
		construct(src, n);
	}

	template <typename type2, class allocator_type2>
	simple_vector(const simple_vector<type2, allocator_type2>& src)
		: m_length(0),
		m_capacity(0)
	{
		construct(src);
	}

	template <typename type2, class allocator_type2>
	simple_vector(const simple_vector<type2, allocator_type2>& src, size_t i)
		: m_length(0),
		m_capacity(0)
	{
		construct(src, i);
	}

	template <typename type2, class allocator_type2>
	simple_vector(const simple_vector<type2, allocator_type2>& src, size_t i, size_t n)
		: m_length(0),
		m_capacity(0)
	{
		construct(src, i, n);
	}

	~simple_vector()
	{
		clear_inner();
	}

	this_t& operator=(const type& src) { assign(1, src); return *this; }
	this_t& operator=(type&& src) { assign(1, std::move(src)); return *this; }

	this_t& operator=(const this_t& src) { assign(src); return *this; }

	template <typename type2, class allocator_type2>
	this_t& operator=(const simple_vector<type2, allocator_type2>& src) { assign(src); return *this; }

	this_t& operator=(this_t&& src)
	{
		m_allocator = std::move(src.m_allocator);
		m_length = std::move(src.m_length);
		m_capacity = src.m_capacity;
		m_ptr = src.m_ptr;
		src.m_capacity = 0;
		return *this;
	}

	size_t get_length() const { return m_length; }
	size_t get_capacity() const { return m_capacity; }

	bool is_empty() const { return m_length == 0; }
	bool operator!() const { return m_length == 0; }

	const type* get_const_ptr() const { return m_ptr.get_ptr(); }

	const type& operator[](size_t i) const { return m_ptr[i]; }

	const type& get_first_const() const { return *(m_ptr.get_ptr()); }
	const type& get_last_const() const { return *((m_ptr.get_ptr() + m_length) - 1); }

	const simple_vector<type>& subrange(size_t i, simple_vector<type>& storage = simple_vector<type>()) const
	{
		if (i <= m_length)
		{
			size_t adjustedLength = m_length - i;
			storage.set_temporary(m_ptr + i, adjustedLength);
		}
		return storage;
	}

	const simple_vector<type>& subrange(size_t i, size_t n, simple_vector<type>& storage = simple_vector<type>()) const
	{
		if (i <= m_length)
		{
			size_t adjustedLength = m_length - i;
			if (n > adjustedLength)
				n = adjustedLength;
			storage.set_temporary(m_ptr + i, n);
		}
		return storage;
	}

	template <typename type2>
	bool equals(size_t n, type2& cmp) const
	{
		bool result = false;
		if (m_length == n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (!(m_ptr[i] == cmp))
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
				if (!(m_ptr[i] == cmp[i]))
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, class allocator_type2>
	bool equals(const simple_vector<type2, allocator_type2>& cmp) const
	{
		return equals(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	bool equals(const simple_vector<type2, allocator_type2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !*this;
		return equals(cmp.get_const_ptr(), length);
	}

	template <typename type2, class allocator_type2>
	bool equals(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (!n || i >= length)
			return !*this;
		return equals(cmp.get_const_ptr(), cmp.get_length());
	}

	bool operator==(const this_t& cmp) const { return equals(cmp); }

	template <typename type2, class allocator_type2>
	bool operator==(const simple_vector<type2, allocator_type2>& cmp) const { return equals(cmp); }

	bool operator!=(const this_t& cmp) const { return !equals(cmp); }

	template <typename type2, class allocator_type2>
	bool operator!=(const simple_vector<type2, allocator_type2>& cmp) const { return !equals(cmp); }

	template <typename type2>
	bool starts_with(size_t n, type2& cmp) const
	{
		bool result = false;
		if (!!n && m_length >= n)
		{
			result = true;
			for (size_t i = 0; i < n; i++)
			{
				if (m_ptr[i] != cmp)
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
				if (m_ptr[i] != cmp[i])
				{
					result = false;
					break;
				}
			}
		}
		return result;
	}

	template <typename type2, class allocator_type2>
	bool starts_with(const simple_vector<type2, allocator_type2>& cmp) const
	{
		return starts_with(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	bool starts_with(const simple_vector<type2, allocator_type2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return starts_with(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, class allocator_type2>
	bool starts_with(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const
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
				if (m_ptr[i] != cmp)
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
				if (m_ptr[i] != cmp[i])
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

	template <typename type2, class allocator_type2>
	bool ends_with(const simple_vector<type2, allocator_type2>& cmp) const
	{
		return ends_with(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	bool ends_with(const simple_vector<type2, allocator_type2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return ends_with(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, class allocator_type2>
	bool ends_with(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const
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
			const type& value = m_ptr[i];
			const type& cmpValue = cmp[i];
			if (value < cmpValue)
				return -1;
			if (value > cmpValue)
				return 1;
		} while (++i != shorterLength);
		return !!cmpIsLonger ? -1 : ((n == m_length) ? 0 : 1);
	}

	template <typename type2, class allocator_type2>
	int compare(const simple_vector<type2, allocator_type2>& cmp) const
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
				const type& value = m_ptr[i];
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

	template <typename type2, class allocator_type2>
	bool is_less_than(const simple_vector<type2, allocator_type2>& cmp) const
	{
		return is_less_than(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	bool is_less_than(const simple_vector<type2, allocator_type2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		return is_less_than(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, class allocator_type2>
	bool is_less_than(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return false;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return is_less_than(cmp.get_const_ptr() + i, n);
	}

	template <typename type2, class allocator_type2>
	bool operator<(const simple_vector<type2, allocator_type2>& cmp) const { return is_less_than(cmp); }

	template <typename type2, class allocator_type2>
	bool operator>=(const simple_vector<type2, allocator_type2>& cmp) const { return !is_less_than(cmp); }

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
				const type& value = m_ptr[i];
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

	template <typename type2, class allocator_type2>
	bool is_greater_than(const simple_vector<type2, allocator_type2>& cmp) const
	{
		return is_greater_than(cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	bool is_greater_than(const simple_vector<type2, allocator_type2>& cmp, size_t i) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !!m_length;
		return is_greater_than(cmp.get_const_ptr() + i, length - i);
	}

	template <typename type2, class allocator_type2>
	bool is_greater_than(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const
	{
		size_t length = cmp.get_length();
		if (i >= length)
			return !!m_length;
		size_t remainingLength = length - i;
		if (n > remainingLength)
			n = remainingLength;
		return is_greater_than(cmp.get_const_ptr() + i, n);
	}

	template <typename type2, class allocator_type2>
	bool operator>(const simple_vector<type2, allocator_type2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2, class allocator_type2>
	bool operator<=(const simple_vector<type2, allocator_type2>& cmp) const { return !is_greater_than(cmp); }

	template <typename type2>
	size_t index_of(const type2& cmp) const { return index_of(0, cmp); }

	template <typename type2>
	size_t index_of(size_t i, const type2& cmp) const
	{
		size_t length = m_length;
		for (; (i < length); i++)
			if (m_ptr[i] == cmp)
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
			type& t = m_ptr[i2];
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

	template <typename type2, class allocator_type2>
	size_t index_of_segment(const simple_vector<type2, allocator_type2>& cmp) const { return index_of_segment(0, cmp); }

	template <typename type2, class allocator_type2>
	size_t index_of_segment(const simple_vector<type2, allocator_type2>& cmp, size_t i) const { return index_of_segment(0, cmp, i); }

	template <typename type2, class allocator_type2>
	size_t index_of_segment(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const { return index_of_segment(0, cmp, i, n); }

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
					if (m_ptr[i] == cmp[0])
					{
						bool isMatch = true;
						for (size_t i2 = 1; i2 < n; i2++)
						{
							if (m_ptr[i + i2] != cmp[i2])
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


	template <typename type2, class allocator_type2>
	size_t index_of_segment(size_t i, const simple_vector<type2, allocator_type2>& cmp) const
	{
		return index_of_segment(i, cmp.get_const_ptr(), cmp.get_length());
	}

	template <typename type2, class allocator_type2>
	size_t index_of_segment(size_t i, const simple_vector<type2, allocator_type2>& cmp, size_t i2) const
	{
		size_t cmpLength = cmp.get_length();
		if (i2 >= cmpLength)
			return const_max_int_v<size_t>;
		return index_of_segment(i, cmp.get_const_ptr() + i2, cmpLength - i2);
	}

	template <typename type2, class allocator_type2>
	size_t index_of_segment(size_t i, const simple_vector<type2, allocator_type2>& cmp, size_t i2, size_t n) const
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

	template <typename type2, class allocator_type2>
	bool contains_segment(const simple_vector<type2, allocator_type2>& cmp) const { return index_of_segment(cmp) != const_max_int_v<size_t>; }

	template <typename type2, class allocator_type2>
	bool contains_segment(const simple_vector<type2, allocator_type2>& cmp, size_t i) const { return index_of_segment(cmp, i) != const_max_int_v<size_t>; }

	template <typename type2, class allocator_type2>
	bool contains_segment(const simple_vector<type2, allocator_type2>& cmp, size_t i, size_t n) const { return index_of_segment(cmp, i, n) != const_max_int_v<size_t>; }

	type* get_ptr() { return m_ptr.get_ptr(); }
	type& get_first() { return m_ptr.get_ptr()[0]; }
	type& get_last() { return m_ptr.get_ptr()[m_length - 1]; }

	template <typename type2>
	void set_index(size_t i, type2& src) { m_ptr[i] = src; }

	void reverse()
	{
		if (m_length > 1)
		{
			size_t lengthMinusOne = m_length - 1;
			for (size_t i = 0; i < m_length/2; i++)
			{
				type tmp = m_ptr[i];
				size_t alternatePosition = lengthMinusOne - i;
				m_ptr[i] = m_ptr[alternatePosition];
				m_ptr[alternatePosition] = tmp;
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

	// clears reserved space as well
	void reset()
	{
		clear();
		m_capacity = 0;
		m_ptr.clear();
	}

	void reserve(size_t n)
	{
		if ((n > m_capacity) && (!m_capacity || !try_reallocate(n)))
		{
			size_t capacity;
			type* newPtr = m_allocator.allocate(n, capacity);
			placement_copy_construct_array(newPtr, m_ptr.get_ptr(), m_length);
			clear_inner();
			m_capacity = capacity;
			m_ptr = newPtr;
		}
	}

	template <typename type2 = type>
	void assign(size_t n, const type2& src)
	{
		if (!n)
			clear();
		else
		{
			prepare_assign(n);
			placement_construct_multiple(m_ptr, n, src);
		}
	}

	template <typename type2 = type>
	void assign(size_t n, type2&& src)
	{
		if (!n)
			clear();
		else
		{
			prepare_assign(n);
			size_t lastPosition = n - 1;
			if (lastPosition != 0)
				placement_construct_multiple(m_ptr, lastPosition, src);
			placement_construct(m_ptr + lastPosition, std::forward<type2>(src));
		}
	}

	template <typename type2>
	void assign(type2* src, size_t n)
	{
		if (!n)
			clear();
		else
		{
			prepare_assign(n);
			placement_copy_construct_array(m_ptr, src, n);
		}
	}

	void assign(const this_t& src)
	{
		if (this != &src)
		{
			size_t n = src.get_length();
			if (!n)
				clear();
			else
			{
				prepare_assign(n);
				placement_copy_construct_array(m_ptr, src.get_const_ptr(), n);
			}
		}
	}

	template <typename type2, class allocator_type2>
	void assign(const simple_vector<type2, allocator_type2>& src)
	{
		size_t n = src.m_length;
		if (!n)
			clear();
		else
		{
			prepare_assign(n);
			placement_copy_construct_array(m_ptr, src.get_const_ptr(), n);
		}
	}

	template <typename type2, class allocator_type2>
	void assign(const simple_vector<type2, allocator_type2>& src, size_t i)
	{
		size_t length = src.m_length;
		if (i >= src)
			clear();
		else
		{
			size_t remainingLength = length - i;
			prepare_assign(remainingLength);
			placement_copy_construct_array(m_ptr, src.get_const_ptr() + i, remainingLength);
		}
	}

	template <typename type2, class allocator_type2>
	void assign(const simple_vector<type2, allocator_type2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			size_t length = src.m_length;
			if (i < length)
			{
				size_t remainingLength = length - i;
				if (n > remainingLength)
					n = remainingLength;
				prepare_assign(n);
				placement_copy_construct_array(m_ptr, src.get_const_ptr() + i, n);
				return;
			}
		}
		clear();
	}

	void append(size_t n = 1)
	{
		if (!!n)
		{
			size_t newLength = m_length + n;
			reserve(newLength);
			placement_construct_multiple(m_ptr + m_length, n);
			m_length = newLength;
		}
	}

	template <typename type2 = type>
	void append(size_t n, const type2& src)
	{
		if (!!n)
		{
			size_t newLength = m_length + n;
			reserve(newLength);
			placement_construct_multiple(m_ptr + m_length, n, src);
			m_length = newLength;
		}
	}

	template <typename type2 = type>
	void append(size_t n, type2&& src)
	{
		if (!!n)
		{
			size_t newLength = m_length + n;
			reserve(newLength);
			size_t lastPosition = n - 1;
			type* dst = m_ptr + m_length;
			if (lastPosition != 0)
				placement_construct_multiple(dst, lastPosition, src);
			placement_construct(dst + lastPosition, std::forward<type2>(src));
			m_length = newLength;
		}
	}

	template <typename type2>
	void append(type2* src, size_t n)
	{
		if (!!n)
		{
			size_t newLength = m_length + n;
			reserve(newLength);
			placement_copy_construct_array(m_ptr + m_length, src, n);
			m_length = newLength;
		}
	}

	template <typename type2, class allocator_type2>
	void append(const simple_vector<type2, allocator_type2>& src)
	{
		size_t srcLength = src.get_length();
		if (!!srcLength)
		{
			size_t newLength = m_length + srcLength;
			reserve(newLength);
			placement_copy_construct_array(m_ptr + m_length, src.get_const_ptr(), srcLength);
			m_length = newLength;
		}
	}

	template <typename type2, class allocator_type2>
	void append(const simple_vector<type2, allocator_type2>& src, size_t i)
	{
		size_t srcLength = src.get_length();
		if (i < srcLength)
		{
			srcLength -= i;
			size_t newLength = m_length + srcLength;
			reserve(newLength);
			placement_copy_construct_array(m_ptr + m_length, src.get_const_ptr() + i, srcLength);
			m_length = newLength;
		}
	}

	template <typename type2, class allocator_type2>
	void append(const simple_vector<type2, allocator_type2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			size_t srcLength = src.get_length();
			if (i < srcLength)
			{
				size_t remainingLength = srcLength - i;
				if (n > remainingLength)
					n = remainingLength;
				size_t newLength = m_length + n;
				reserve(newLength);
				placement_copy_construct_array(m_ptr + m_length, src.get_const_ptr() + i, n);
				m_length = newLength;
			}
		}
	}

	this_t& operator+=(const type& src) { append(1, src); return *this; }

	template <typename type2, class allocator_type2>
	this_t& operator+=(const simple_vector<type2, allocator_type2>& src) { append(src); return *this; }

	void resize(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			size_t numToDestruct = m_length - n;
			m_length = n;
			placement_destruct_multiple(m_ptr + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			size_t numToConstruct = n - m_length;
			reserve(n);
			placement_construct_multiple(m_ptr + m_length, numToConstruct);
			m_length = n;
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
			placement_destruct_multiple(m_ptr + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			size_t numToConstruct = n - m_length;
			reserve(n);
			placement_construct_multiple(m_ptr + m_length, numToConstruct, src);
			m_length = n;
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
			placement_destruct_multiple(m_ptr + m_length, numToDestruct);
		}
		else if (n > m_length)
		{
			reserve(n);
			size_t lastPosition = n - m_length - 1;
			type* dst = m_ptr + m_length;
			if (lastPosition != 0)
				placement_construct_multiple(dst, lastPosition, src);
			placement_construct(dst + lastPosition, std::forward<type2>(src));
			m_length = n;
		}
	}

	void erase(size_t i)
	{
		if (i < m_length)
		{
			m_length--;
			type* erasePositionPtr = m_ptr + i;
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
			type* erasePositionPtr = m_ptr + i;
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
			placement_destruct_multiple(m_ptr + m_length, numToDestruct);
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
			{
				insert_inner(i, n);
				placement_construct_multiple(m_ptr + i, n);
			}
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
			{
				insert_inner(i, n);
				placement_construct_multiple(m_ptr + i, n, src);
			}
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
				insert_inner(i, n);
				size_t lastPosition = n - 1;
				type* dst = m_ptr + i;
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
			{
				insert_inner(i, n);
				ptr<type> insertPtr = m_ptr + i;
				placement_copy_construct_array(insertPtr, src, n);
			}
		}
	}

	template <typename type2, class allocator_type2>
	void insert(size_t i, const simple_vector<type2, allocator_type2>& src)
	{
		insert(i, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, class allocator_type2>
	void insert(size_t i, const simple_vector<type2, allocator_type2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			insert(i, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
	}

	template <typename type2, class allocator_type2>
	void insert(size_t i, const simple_vector<type2, allocator_type2>& src, size_t srcIndex, size_t n)
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
			placement_reconstruct_multiple(m_ptr + i, replaceLength, src);
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
			placement_copy_reconstruct_array(m_ptr + i, src, replaceLength);
		}
	}

	template <typename type2, class allocator_type2>
	void replace(size_t i, const simple_vector<type2, allocator_type2>& src)
	{
		replace(i, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, class allocator_type2>
	void replace(size_t i, const simple_vector<type2, allocator_type2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			replace(i, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
	}

	template <typename type2, class allocator_type2>
	void replace(size_t i, const simple_vector<type2, allocator_type2>& src, size_t srcIndex, size_t n)
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
		{
			replace_inner(i, replaceLength, insertLength);
			placement_construct_multiple(m_ptr + i, insertLength, src);
		}
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
		{
			replace_inner(i, replaceLength, insertLength);
			placement_copy_construct_array(m_ptr + i, src, insertLength);
		}
	}

	template <typename type2, class allocator_type2>
	void insert_replace(size_t i, size_t replaceLength, const simple_vector<type2, allocator_type2>& src)
	{
		insert_replace(i, replaceLength, src.get_const_ptr(), src.get_length());
	}

	template <typename type2, class allocator_type2>
	void insert_replace(size_t i, size_t replaceLength, const simple_vector<type2, allocator_type2>& src, size_t srcIndex)
	{
		size_t srcLength = src.get_length();
		if (srcIndex < srcLength)
			insert_replace(i, replaceLength, src.get_const_ptr() + srcIndex, srcLength - srcIndex);
		else
			erase(i, replaceLength);
	}

	template <typename type2, class allocator_type2>
	void insert_replace(size_t i, size_t replaceLength, const simple_vector<type2, allocator_type2>& src, size_t srcIndex, size_t n)
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

	void swap(this_t& wth)
	{
		size_t capacity = m_capacity;
		m_capacity = wth.m_capacity;
		wth.m_capacity = capacity;

		size_t length = m_length;
		m_length = wth.m_length;
		wth.m_length = length;

		type* ptr = m_ptr;
		m_ptr = wth.m_ptr;
		wth.m_ptr = ptr;
	}

	this_t exchange(this_t&& src)
	{
		this_t tmp(std::move(src));
		swap(tmp);
		return tmp;
	}

	void exchange(this_t&& src, this_t& rtn)
	{
		rtn = std::move(src);
		swap(rtn);
	}

	iterator begin() const { return get_first_iterator(); }
	iterator rbegin() const { return get_last_iterator(); }
	iterator end() const { iterator i; return i; }
	iterator rend() const { iterator i; return i; }
};


}


#endif
