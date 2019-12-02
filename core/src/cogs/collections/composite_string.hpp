//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Placeholder, WorkInProgress

#ifndef COGS_HEADER_COLLECTION_COMPOSITE_STRING
#define COGS_HEADER_COLLECTION_COMPOSITE_STRING


#include "cogs/collections/composite_vector.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/math/const_max_int.hpp"


#pragma warning(push)
#pragma warning (disable: 4521) // multiple copy constructors specified
#pragma warning (disable: 4522) // multiple assignment operators specified


namespace cogs {


// forward declare
namespace io
{
	class composite_buffer;
}

/// @ingroup LockFreeCollections
/// @brief A string comprised of one or more substrings.
/// @tparam T Character type.  Usually char or wchar_t.
template <typename T>
class composite_string_t
{
private:
	friend class io::composite_buffer;

	template <typename>
	friend class composite_vector;

	typedef composite_vector<T> content_t;
	content_t m_contents;

	composite_string_t(const composite_vector<T>& src)
		: m_contents(src)
	{ }

	composite_string_t(const vector<vector<T> >& src, size_t n)
		: m_contents(src, n)
	{ }

public:
	typedef T type;
	typedef type char_t;

	typedef composite_string_t<type> this_t;

	typedef string_t<type> inner_t;

	/// @brief An index position within a composite_string
	class position_t
	{
	private:
		template <typename>
		friend class composite_string_t;

		typename content_t::position_t m_pos;

		position_t(const typename content_t::position_t& pos) : m_pos(pos) { }

	public:
		position_t() { }
		position_t(const position_t& pos) : m_pos(pos.m_pos) { }
		position_t(size_t subStringIndex, size_t i) : m_pos(subStringIndex, i) { }

		position_t& operator=(const position_t& pos) { m_pos = pos.m_pos; return *this; }

		void set(size_t subStringIndex, size_t i) { m_pos.set(subStringIndex, i); }
		bool operator==(const position_t& cmp) const { return (m_pos == cmp.m_pos); }
		bool operator!=(const position_t& cmp) const { return !operator==(cmp); }
		bool operator<(const position_t& cmp) const { return (m_pos < cmp.m_pos); }
		bool operator>(const position_t& cmp) const { return (m_pos > cmp.m_pos); }
		bool operator<=(const position_t& cmp) const { return !operator>(cmp); }
		bool operator>=(const position_t& cmp) const { return !operator<(cmp); }

		size_t get_outer_index() const { return m_pos.get_outer_index(); }
		size_t get_inner_index() const { return m_pos.get_inner_index(); }
	};

	/// @brief A composite_string element iterator
	class iterator
	{
	private:
		template <typename>
		friend class composite_string_t;

		typename content_t::iterator m_contents;

		iterator(const typename content_t::iterator& i) : m_contents(i) { }

	public:
		iterator() { }
		iterator(const iterator& i) : m_contents(i.m_contents) { }

		iterator& operator=(const iterator& i) { m_contents = i.m_contents; return *this; }

		void release() { m_contents.release(); }

		iterator& operator++() { ++m_contents; return *this; }
		iterator& operator--() { --m_contents; return *this; }

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_contents; }

		bool operator==(const iterator& i) const { return m_contents == i.m_contents; }
		bool operator!=(const iterator& i) const { return !operator==(i); }

		type* get() const { return m_contents.get(); }
		type& operator*() const { return *get(); }
		type* operator->() const { return get(); }

		position_t get_position() const { return m_contents.get_position(); }

		iterator next() const { return m_contents.next(); }
		iterator prev() const { return m_contents.prev(); }
	};

	/// @brief A composite_string constant element iterator
	class const_iterator
	{
	private:
		template <typename>
		friend class composite_string_t;

		typename content_t::const_iterator m_contents;

		const_iterator(const typename content_t::const_iterator& i) : m_contents(i) { }

	public:
		const_iterator() { }
		const_iterator(const const_iterator& i) : m_contents(i.m_contents) { }
		const_iterator(const iterator& i) : m_contents(i.m_contents) { }

		const_iterator& operator=(const const_iterator& i) { m_contents = i.m_contents; return *this; }
		const_iterator& operator=(const iterator& i) { m_contents = i.m_contents; return *this; }

		void release() { m_contents.release(); }

		const_iterator& operator++() { ++m_contents; return *this; }
		const_iterator& operator--() { --m_contents; return *this; }

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_contents; }

		bool operator==(const const_iterator& i) const { return m_contents == i.m_contents; }
		bool operator==(const iterator& i) const { return m_contents == i.m_contents; }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }

		const type* get() const { return m_contents.get(); }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return get(); }

		position_t get_position() const { return m_contents.get_position(); }

		const_iterator next() const { return m_contents.next(); }
		const_iterator prev() const { return m_contents.prev(); }
	};

	const_iterator get_first_const_iterator() const { return m_contents.get_first_const_iterator(); }
	const_iterator get_last_const_iterator() const { return m_contents.get_last_const_iterator(); }

	iterator get_first_iterator() { return m_contents.get_first_iterator(); }
	iterator get_last_iterator() { return m_contents.get_last_iterator(); }

	static this_t literal(const type* src) { return this_t(string_t<type>::literal(src)); }
	static this_t literal(type& src) { return this_t(string_t<type>::literal(src)); }

	static this_t contain(const type* src) { return this_t(string_t<type>::contain(src)); }
	static this_t contain(const type* src, size_t n) { return this_t(string_t<type>::contain(n, src)); }

	composite_string_t(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	composite_string_t()
	{ }

	composite_string_t(const this_t& src)
		: m_contents(src.m_contents)
	{ }

	composite_string_t(const volatile this_t& src)
		: m_contents(src.m_contents)
	{ }

	composite_string_t(const this_t& src, size_t i)
		: m_contents(src.m_contents, i)
	{ }

	composite_string_t(const volatile this_t& src, size_t i)
		: m_contents(src.m_contents, i)
	{ }

	composite_string_t(const this_t& src, size_t i, size_t n)
		: m_contents(src.m_contents, i, n)
	{ }

	composite_string_t(const volatile this_t& src, size_t i, size_t n)
		: m_contents(src.m_contents, i, n)
	{ }

	composite_string_t(const string_t<type>& src)
		: m_contents(src.get_vector())
	{ }

	composite_string_t(const volatile string_t<type>& src)
		: m_contents(src.get_vector())
	{ }

	composite_string_t(const string_t<type>& src, size_t i)
		: m_contents(src.get_vector(), i)
	{ }

	composite_string_t(const volatile string_t<type>& src, size_t i)
		: m_contents(src.get_vector(), i)
	{ }

	composite_string_t(const string_t<type>& src, size_t i, size_t n)
		: m_contents(src.get_vector(), i, n)
	{ }

	composite_string_t(const volatile string_t<type>& src, size_t i, size_t n)
		: m_contents(src.get_vector(), i, n)
	{ }

	composite_string_t(const type& src)
		: m_contents(vector<type>(src))
	{ }

	composite_string_t(size_t n, const type& src)
		: m_contents(vector<type>(n, src))
	{ }

	composite_string_t(const type* src, size_t n)
		: m_contents(vector<type>(src, n))
	{ }

	composite_string_t(const type* src)
		: m_contents(vector<type>(src, inner_t::count_chars(src)))
	{ }

	string_t<type> composite() const { return string_t<type>::from_vector(m_contents.composite()); }
	string_t<type> composite() const volatile { return string_t<type>::from_vector(m_contents.composite()); }

	string_t<type> composite(size_t i, size_t n = const_max_int_v<size_t>) const { return string_t<type>::from_vector(m_contents.composite(i, n)); }
	string_t<type> composite(size_t i, size_t n = const_max_int_v<size_t>) const volatile { return string_t<type>::from_vector(m_contents.composite(i, n)); }

	string_t<type> composite(const position_t& pos, size_t n = const_max_int_v<size_t>) const { return string_t<type>::from_vector(m_contents.composite(pos.m_pos, n)); }
	string_t<type> composite(const position_t& pos, size_t n = const_max_int_v<size_t>) const volatile { return string_t<type>::from_vector(m_contents.composite(pos.m_pos, n)); }

	size_t get_length() const { return m_contents.get_length(); }
	size_t get_length() const volatile { return m_contents.get_length(); }

	bool is_empty() const { return m_contents.is_empty(); }
	bool is_empty() const volatile { return m_contents.is_empty(); }

	bool operator!() const { return m_contents.operator!(); }
	bool operator!() const volatile { return m_contents.operator!(); }


	bool equals(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool equals(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool equals(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool equals(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}


	bool equals(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool equals(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool equals(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool equals(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	bool equals(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool equals(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool equals(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool equals(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool equals(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool equals(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool equals(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool equals(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool equals(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}


	bool equals(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool equals(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_vector());
	}


	bool equals(size_t i, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}


	bool equals(size_t i, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}


	bool equals(const position_t& pos, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}


	bool equals(const position_t& pos, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}



	bool equals(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool equals(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool equals(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(size_t i, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(size_t i, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template equals<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool operator==(const string_t<type>& cmp) const { return equals(cmp); }

	bool operator==(const this_t& cmp) const { return equals(cmp); }

	bool operator==(const string_t<type>& cmp) const volatile { return equals(cmp); }

	bool operator==(const this_t& cmp) const volatile { return equals(cmp); }

	bool operator==(const volatile string_t<type>& cmp) const { return equals(cmp); }

	bool operator==(const volatile this_t& cmp) const { return equals(cmp); }


	bool operator!=(const string_t<type>& cmp) const { return !equals(cmp); }

	bool operator!=(const this_t& cmp) const { return !equals(cmp); }

	bool operator!=(const string_t<type>& cmp) const volatile { return !equals(cmp); }

	bool operator!=(const this_t& cmp) const volatile { return !equals(cmp); }

	bool operator!=(const volatile string_t<type>& cmp) const { return !equals(cmp); }

	bool operator!=(const volatile this_t& cmp) const { return !equals(cmp); }


	int compare(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	int compare(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	int compare(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	int compare(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}


	int compare(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}


	int compare(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	int compare(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	int compare(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	int compare(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	int compare(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	int compare(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	int compare(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	int compare(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	int compare(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	int compare(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	int compare(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	int compare(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	int compare(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	int compare(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	int compare(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	int compare(size_t i, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	int compare(size_t i, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	int compare(const position_t& pos, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	int compare(const position_t& pos, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}




	int compare(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}
	int compare(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(size_t i, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template compare<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}









	bool is_less_than(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool is_less_than(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool is_less_than(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	bool is_less_than(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool is_less_than(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool is_less_than(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}




	bool is_less_than(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_less_than(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_less_than(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool is_less_than(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_less_than(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_less_than(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}



	bool is_less_than(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_less_than(size_t i, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_less_than(size_t i, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_less_than(const position_t& pos, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool is_less_than(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool is_less_than(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool is_less_than(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}
	bool is_less_than(size_t i, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}
	bool is_less_than(const position_t& pos, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}
	bool is_less_than(const position_t& pos, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_less_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}



	bool is_greater_than(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool is_greater_than(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	bool is_greater_than(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool is_greater_than(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	bool is_greater_than(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_greater_than(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_greater_than(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool is_greater_than(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_greater_than(size_t i, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_greater_than(size_t i, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, size_t n, const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}


	bool is_greater_than(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool is_greater_than(size_t i, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_vector());
	}

	bool is_greater_than(size_t i, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_vector());
	}

	bool is_greater_than(const position_t& pos, size_t n, const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_vector());
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_vector());
	}





	bool is_greater_than(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool is_greater_than(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool is_greater_than(const position_t& pos, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool is_greater_than(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, buf.get_composite_vector(), cmpIndex, cmpLength);
	}
	bool is_greater_than(size_t i, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(i, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template is_greater_than<type, default_comparator>(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(pos.m_pos, n, buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool operator<(const string_t<type>& cmp) const { return is_less_than(cmp); }
	bool operator<(const string_t<type>& cmp) const volatile { return is_less_than(cmp); }
	bool operator<(const volatile string_t<type>& cmp) const { return is_less_than(cmp); }
	bool operator<(const this_t& cmp) const { return is_less_than(cmp); }
	bool operator<(const this_t& cmp) const volatile { return is_less_than(cmp); }
	bool operator<(const volatile this_t& cmp) const { return is_less_than(cmp); }

	bool operator>(const string_t<type>& cmp) const { return is_greater_than(cmp); }
	bool operator>(const string_t<type>& cmp) const volatile { return is_greater_than(cmp); }
	bool operator>(const volatile string_t<type>& cmp) const { return is_greater_than(cmp); }
	bool operator>(const this_t& cmp) const { return is_greater_than(cmp); }
	bool operator>(const this_t& cmp) const volatile { return is_greater_than(cmp); }
	bool operator>(const volatile this_t& cmp) const { return is_greater_than(cmp); }

	bool operator<=(const string_t<type>& cmp) const { return !operator>(cmp); }
	bool operator<=(const string_t<type>& cmp) const volatile { return !operator>(cmp); }
	bool operator<=(const volatile string_t<type>& cmp) const { return !operator>(cmp); }
	bool operator<=(const this_t& cmp) const { return !operator>(cmp); }
	bool operator<=(const this_t& cmp) const volatile { return !operator>(cmp); }
	bool operator<=(const volatile this_t& cmp) const { return !operator>(cmp); }

	bool operator>=(const string_t<type>& cmp) const { return !operator<(cmp); }
	bool operator>=(const string_t<type>& cmp) const volatile { return !operator<(cmp); }
	bool operator>=(const volatile string_t<type>& cmp) const { return !operator<(cmp); }
	bool operator>=(const this_t& cmp) const { return !operator<(cmp); }
	bool operator>=(const this_t& cmp) const volatile { return !operator<(cmp); }
	bool operator>=(const volatile this_t& cmp) const { return !operator<(cmp); }

	bool starts_with(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(cmp, cmpLength);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool starts_with(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(cmp, cmpLength);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool starts_with(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_vector());
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool starts_with(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_vector());
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool starts_with(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_vector());
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}


	bool starts_with(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool starts_with(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool starts_with(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template starts_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}



	bool ends_with(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(cmp, cmpLength);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool ends_with(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(cmp, cmpLength);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool ends_with(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_vector());
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool ends_with(const string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_vector());
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}

	bool ends_with(const volatile string_t<type>& buf, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_vector());
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_vector());
	}


	bool ends_with(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool ends_with(const this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool ends_with(const volatile this_t& buf, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template ends_with<type, default_comparator>(buf.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(buf.get_composite_vector(), cmpIndex, cmpLength);
	}



	size_t index_of(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp);
	}

	size_t index_of(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp);
	}

	size_t index_of(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(i, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp);
	}

	size_t index_of(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(i, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp);
	}

	size_t index_of(size_t i, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(i, n, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, n, cmp);
	}

	size_t index_of(size_t i, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of<type, default_comparator>(i, n, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, n, cmp);
	}

	position_t position_of(const position_t& pos, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of<type, default_comparator>(pos.m_pos, cmp);
		return m_contents.template position_of<type, case_insensitive_comparator<type> >(pos.m_pos, cmp);
	}

	position_t position_of(const position_t& pos, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of<type, default_comparator>(pos.m_pos, cmp);
		return m_contents.template position_of<type, case_insensitive_comparator<type> >(pos.m_pos, cmp);
	}

	position_t position_of(const position_t& pos, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of<type, default_comparator>(pos.m_pos, n, cmp);
		return m_contents.template position_of<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp);
	}

	position_t position_of(const position_t& pos, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of<type, default_comparator>(pos.m_pos, n, cmp);
		return m_contents.template position_of<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp);
	}



	size_t index_of_any(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	size_t index_of_any(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	size_t index_of_any(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	size_t index_of_any(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	size_t index_of_any(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	size_t index_of_any(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_any<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}


	position_t position_of_any(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_any<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template position_of_any<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	position_t position_of_any(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_any<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template position_of_any<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	position_t position_of_any(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_any<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template position_of_any<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}

	position_t position_of_any(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_any<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template position_of_any<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}




	size_t index_of_segment(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	size_t index_of_segment(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	size_t index_of_segment(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	size_t index_of_segment(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}




	size_t index_of_segment(const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}

	size_t index_of_segment(const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}

	size_t index_of_segment(const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}


	size_t index_of_segment(const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}


	size_t index_of_segment(size_t i, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}




	size_t index_of_segment(size_t i, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, size_t n, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	size_t index_of_segment(size_t i, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	size_t index_of_segment(const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template index_of_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template index_of_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}





	position_t position_of_segment(const position_t& pos, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}

	position_t position_of_segment(const position_t& pos, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}

	position_t position_of_segment(const position_t& pos, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}



	position_t position_of_segment(const position_t& pos, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	position_t position_of_segment(const position_t& pos, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}



	position_t position_of_segment(const position_t& pos, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}

	position_t position_of_segment(const position_t& pos, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}



	position_t position_of_segment(const position_t& pos, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	position_t position_of_segment(const position_t& pos, size_t n, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template position_of_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template position_of_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}



	bool contains(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(cmp);
	}

	bool contains(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(cmp);
	}

	bool contains(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(i, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(i, cmp);
	}

	bool contains(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(i, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(i, cmp);
	}

	bool contains(const position_t& pos, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(pos.m_pos, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(pos.m_pos, cmp);
	}

	bool contains(const position_t& pos, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(pos.m_pos, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(pos.m_pos, cmp);
	}

	bool contains(size_t i, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(i, n, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(i, n, cmp);
	}

	bool contains(size_t i, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(i, n, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(i, n, cmp);
	}

	bool contains(const position_t& pos, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(pos.m_pos, n, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp);
	}

	bool contains(const position_t& pos, size_t n, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains<type, default_comparator>(pos.m_pos, n, cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp);
	}



	bool contains_segment(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool contains_segment(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool contains_segment(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool contains_segment(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}

	bool contains_segment(const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp, cmpLength);
	}

	bool contains_segment(size_t i, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp, cmpLength);
	}

	bool contains_segment(size_t i, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp, cmpLength);
	}

	bool contains_segment(const position_t& pos, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const type* cmp, size_t cmpLength, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp, cmpLength);
	}



	bool contains_segment(const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}

	bool contains_segment(const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}

	bool contains_segment(const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_vector());
	}

	bool contains_segment(const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(size_t i, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	bool contains_segment(size_t i, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	bool contains_segment(size_t i, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_vector());
	}

	bool contains_segment(size_t i, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(size_t i, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(size_t i, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}


	bool contains_segment(size_t i, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	bool contains_segment(size_t i, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	bool contains_segment(size_t i, size_t n, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_vector());
	}

	bool contains_segment(size_t i, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(size_t i, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(size_t i, size_t n, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(i, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	bool contains_segment(const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}


	bool contains_segment(const position_t& pos, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}

	bool contains_segment(const position_t& pos, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}

	bool contains_segment(const position_t& pos, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_vector());
	}



	bool contains_segment(const position_t& pos, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	bool contains_segment(const position_t& pos, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}



	bool contains_segment(const position_t& pos, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}

	bool contains_segment(const position_t& pos, size_t n, const string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile string_t<type>& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_vector());
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_vector());
	}



	bool contains_segment(const position_t& pos, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile this_t& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpIndex, cmpLength);
	}



	bool contains_segment(const position_t& pos, size_t n, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile this_t& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.template contains_segment<type, default_comparator>(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(pos.m_pos, n, cmp.get_composite_vector(), cmpPos.m_pos, cmpLength);
	}

	void reverse() { m_contents.reverse(); }

	size_t get_inner_count() const { return m_contents.get_inner_count(); }
	size_t get_inner_count() const volatile { return m_contents.get_inner_count(); }

	const composite_vector<type>& get_composite_vector() const { return m_contents; }
	const volatile composite_vector<type>& get_composite_vector() const volatile { return m_contents; }

	static this_t from_composite_vector(const composite_vector<type>& v)
	{
		this_t result(v);
		return result;
	}

	static this_t from_composite_vector(const volatile composite_vector<type>& v)
	{
		composite_vector<type> cpy(v);
		this_t result(cpy);
		return result;
	}

	const vector<type>& get_inner_array(size_t i) const { return m_contents.get_inner_array(i); }
	const vector<type> get_inner_array(size_t i) const volatile { return m_contents.get_inner_array(i); }

	string_t<type> get_inner_string(size_t i) const
	{
		return string_t<type>::from_vector(get_inner_array(i));
	}

	string_t<type> get_inner_string(size_t i) const volatile
	{
		return string_t<type>::from_vector(get_inner_array(i));
	}


	string_t<type> get_inner(size_t i) const
	{
		return string_t<type>::from_vector(get_inner_array(i));
	}

	string_t<type> get_inner(size_t i) const volatile
	{
		return string_t<type>::from_vector(get_inner_array(i));
	}

	void reserve_sub_strings(size_t n) { return m_contents.reserve_sub_arrays(n); }


	void reset() { m_contents.reset(); }
	void reset() volatile { m_contents.reset(); }

	const type& operator[](const position_t& pos) const { return get_inner_string(pos)[pos.get_inner_index()]; }


	this_t subrange(size_t i) const
	{
		return m_contents.subrange(i);
	}

	this_t subrange(size_t i, size_t n) const
	{
		return m_contents.subrange(i, n);
	}

	this_t subrange(size_t i) const volatile
	{
		return m_contents.subrange(i);
	}

	this_t subrange(size_t i, size_t n) const volatile
	{
		return m_contents.subrange(i, n);
	}


	this_t subrange(const position_t& start) const
	{
		return m_contents.subrange(start.m_pos);
	}

	this_t subrange(const position_t& start, size_t n) const
	{
		return m_contents.subrange(start.m_pos, n);
	}

	this_t subrange(const position_t& start) const volatile
	{
		return m_contents.subrange(start.m_pos);
	}

	this_t subrange(const position_t& start, size_t n) const volatile
	{
		return m_contents.subrange(start.m_pos, n);
	}

	this_t subrange(const position_t& start, const position_t& end) const
	{
		return m_contents.subrange(start.m_pos, end.m_pos);
	}

	this_t subrange(const position_t& start, const position_t& end) const volatile
	{
		return m_contents.subrange(start.m_pos, end.m_pos);
	}

	void assign(const type& src) { assign(string_t<type>(src)); }

	void assign(const type& src) volatile { assign(string_t<type>(src)); }

	void assign(size_t n, const type& src) { assign(string_t<type>(n, src)); }

	void assign(size_t n, const type& src) volatile { assign(string_t<type>(n, src)); }

	void assign(const type* src, size_t n) { assign(string_t<type>(src, n)); }

	void assign(const type* src, size_t n) volatile { assign(string_t<type>(src, n)); }

	void assign(const this_t& src) { m_contents.assign(src.m_contents); }

	void assign(const this_t& src) volatile { m_contents.assign(src.m_contents); }

	void assign(const volatile this_t& src) { m_contents.assign(src.m_contents); }


	void assign(const this_t& src, size_t i) { m_contents.assign(src.m_contents, i); }

	void assign(const this_t& src, size_t i) volatile { m_contents.assign(src.m_contents, i); }

	void assign(const volatile this_t& src, size_t i) { m_contents.assign(src.m_contents, i); }



	void assign(const this_t& src, size_t i, size_t n) { m_contents.assign(src.m_contents, i, n); }

	void assign(const this_t& src, size_t i, size_t n) volatile { m_contents.assign(src.m_contents, i, n); }

	void assign(const volatile this_t& src, size_t i, size_t n) { m_contents.assign(src.m_contents, i, n); }



	void assign(const string_t<type>& src) { m_contents.assign(src.m_contents); }

	void assign(const string_t<type>& src) volatile { m_contents.assign(src.m_contents); }

	void assign(const volatile string_t<type>& src) { m_contents.assign(src.m_contents); }



	void assign(const string_t<type>& src, size_t i) { m_contents.assign(src.m_contents, i); }

	void assign(const string_t<type>& src, size_t i) volatile { m_contents.assign(src.m_contents, i); }

	void assign(const volatile string_t<type>& src, size_t i) { m_contents.assign(src.m_contents, i); }



	void assign(const string_t<type>& src, size_t i, size_t n) { m_contents.assign(src.m_contents, i, n); }

	void assign(const string_t<type>& src, size_t i, size_t n) volatile { m_contents.assign(src.m_contents, i, n); }

	void assign(const volatile string_t<type>& src, size_t i, size_t n) { m_contents.assign(src.m_contents, i, n); }


	this_t& operator=(const this_t& src) { m_contents.assign(src.m_contents); return *this; }

	void operator=(const this_t& src) volatile { m_contents.assign(src.m_contents); }

	this_t& operator=(const volatile this_t& src) { m_contents.assign(src.m_contents); return *this; }

	this_t& operator=(this_t&& src) { m_contents = std::move(src.m_contents); return *this; }

	volatile this_t& operator=(this_t&& src) volatile { m_contents = std::move(src.m_contents); return *this; }

	this_t& operator=(const string_t<type>& src) { m_contents.assign(src.get_vector()); return *this; }

	this_t& operator=(const volatile string_t<type>& src) { m_contents.assign(src.get_vector()); return *this; }

	void operator=(const string_t<type>& src) volatile { m_contents.assign(src.get_vector()); }

	void append(const type& src) { m_contents.append(1, src); }
	void append(size_t n, const type& src) { m_contents.append(n, src); }
	void append(const type* src, size_t n) { m_contents.append(src, n); }

	void append(const this_t& src) { m_contents.append(src.m_contents); }

	void append(const volatile this_t& src) { m_contents.append(src.m_contents); }


	void append(const string_t<type>& src) { m_contents.append(src.get_vector()); }

	void append(const volatile string_t<type>& src) { m_contents.append(src.get_vector()); }


	this_t& operator+=(const this_t& src)
	{
		m_contents.append(src.m_contents);
		return *this;
	}

	this_t& operator+=(const volatile this_t& src)
	{
		m_contents.append(src.m_contents);
		return *this;
	}

	this_t& operator+=(const string_t<type>& src)
	{
		m_contents.append(src.get_vector());
		return *this;
	}

	this_t& operator+=(const volatile string_t<type>& src)
	{
		m_contents.append(src.get_vector());
		return *this;
	}


	this_t operator+(const this_t& src)
	{
		this_t result(*this);
		result.m_contents.append(src.m_contents);
		return result;
	}

	this_t operator+(const volatile this_t& src)
	{
		this_t result(*this);
		result.m_contents.append(src.m_contents);
		return result;
	}

	this_t operator+(const string_t<type>& src)
	{
		this_t result(*this);
		result.m_contents.append(src.get_vector());
		return result;
	}

	this_t operator+(const volatile string_t<type>& src)
	{
		this_t result(src);
		result.m_contents.append(src.get_vector());
		return result;
	}


	void prepend(const type& src) { m_contents.prepend(1, src); }
	void prepend(size_t n, const type& src) { m_contents.prepend(n, src); }
	void prepend(const type* src, size_t n) { m_contents.prepend(src, n); }

	void prepend(const this_t& src) { m_contents.prepend(src.get_vector()); }

	void prepend(const volatile this_t& src) { m_contents.prepend(src.m_contents); }


	void prepend(const string_t<type>& src) { m_contents.prepend(src.get_vector()); }

	void prepend(const volatile string_t<type>& src) { m_contents.prepend(src.get_vector()); }




	void insert(size_t i, size_t n) { m_contents.insert(i, n); }

	void insert(size_t i, size_t n, const type& src) { m_contents.insert(i, n, src); }

	void insert(size_t i, const type* src, size_t n) { m_contents.insert(i, src, n); }


	void insert(const position_t& pos, size_t n) { m_contents.insert(pos.m_pos, n); }

	void insert(const position_t& pos, size_t n, const type& src) { m_contents.insert(pos.m_pos, n, src); }

	void insert(const position_t& pos, const type* src, size_t n) { m_contents.insert(pos.m_pos, src, n); }


	void insert(size_t i, const string_t<type>& src)
	{
		m_contents.insert(i, src.get_vector());
	}

	void insert(size_t i, const volatile string_t<type>& src)
	{
		m_contents.insert(i, src.get_vector());
	}

	void insert(size_t i, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(i, src.get_composite_vector(), srcIndex, n);
	}

	void insert(size_t i, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(i, src.get_composite_vector(), srcIndex, n);
	}

	void insert(size_t i, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(i, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void insert(size_t i, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(i, src.get_composite_vector(), srcPos.m_pos, n);
	}



	void insert(const position_t& pos, const string_t<type>& src)
	{
		m_contents.insert(pos.m_pos, src.get_vector());
	}

	void insert(const position_t& pos, const volatile string_t<type>& src)
	{
		m_contents.insert(pos.m_pos, src.get_vector());
	}

	void insert(const position_t& pos, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(pos.m_pos, src.get_composite_vector(), srcIndex, n);
	}

	void insert(const position_t& pos, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(pos.m_pos, src.get_composite_vector(), srcIndex, n);
	}

	void insert(const position_t& pos, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(pos.m_pos, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void insert(const position_t& pos, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert(pos.m_pos, src.get_composite_vector(), srcPos.m_pos, n);
	}







	void replace(size_t i, size_t replaceLength, const type& src)
	{
		m_contents.replace(i, replaceLength, src);
	}

	void replace(size_t i, const type* src, size_t replaceLength)
	{
		m_contents.replace(i, src, replaceLength);
	}

	void replace(const position_t& pos, size_t replaceLength, const type& src)
	{
		m_contents.replace(pos.m_pos, replaceLength, src);
	}

	void replace(const position_t& pos, const type* src, size_t replaceLength)
	{
		m_contents.replace(pos.m_pos, src, replaceLength);
	}


	void replace(size_t i, const string_t<type>& src)
	{
		m_contents.replace(i, src.get_vector());
	}

	void replace(size_t i, const volatile string_t<type>& src)
	{
		m_contents.replace(i, src.get_vector());
	}

	void replace(size_t i, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(i, src, srcIndex, n);
	}

	void replace(size_t i, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(i, src.get_composite_vector(), srcIndex, n);
	}

	void replace(size_t i, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(i, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void replace(size_t i, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(i, src.get_composite_vector(), srcPos.m_pos, n);
	}


	void replace(const position_t& pos, const string_t<type>& src)
	{
		m_contents.replace(pos.m_pos, src.get_vector());
	}

	void replace(const position_t& pos, const volatile string_t<type>& src)
	{
		m_contents.replace(pos.m_pos, src.get_vector());
	}

	void replace(const position_t& pos, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(pos.m_pos, src.get_composite_vector(), srcIndex, n);
	}

	void replace(const position_t& pos, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(pos.m_pos, src.get_composite_vector(), srcIndex, n);
	}

	void replace(const position_t& pos, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(pos.m_pos, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void replace(const position_t& pos, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.replace(pos.m_pos, src.get_composite_vector(), srcPos.m_pos, n);
	}



	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, const type& src)
	{
		m_contents.insert_replace(i, replaceLength, insertLength, src);
	}

	void insert_replace(size_t i, size_t replaceLength, const type* src, size_t insertLength)
	{
		m_contents.insert_replace(i, replaceLength, src, insertLength);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, size_t insertLength, const type& src)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, insertLength, src);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const type* src, size_t insertLength)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src, insertLength);
	}



	void insert_replace(size_t i, size_t replaceLength, const string_t<type>& src)
	{
		m_contents.insert_replace(i, replaceLength, src.get_vector());
	}

	void insert_replace(size_t i, size_t replaceLength, const volatile string_t<type>& src)
	{
		m_contents.insert_replace(i, replaceLength, src.get_vector());
	}

	void insert_replace(size_t i, size_t replaceLength, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(i, replaceLength, src.get_composite_vector(), srcIndex, n);
	}

	void insert_replace(size_t i, size_t replaceLength, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(i, replaceLength, src.get_composite_vector(), srcIndex, n);
	}

	void insert_replace(size_t i, size_t replaceLength, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(i, replaceLength, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void insert_replace(size_t i, size_t replaceLength, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(i, replaceLength, src.get_composite_vector(), srcPos.m_pos, n);
	}


	void insert_replace(const position_t& pos, size_t replaceLength, const string_t<type>& src)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_vector());
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const volatile string_t<type>& src)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_vector());
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_composite_vector(), srcIndex, n);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const volatile this_t& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_composite_vector(), srcIndex, n);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_composite_vector(), srcPos.m_pos, n);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const volatile this_t& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents.insert_replace(pos.m_pos, replaceLength, src.get_composite_vector(), srcPos.m_pos, n);
	}


	void erase(size_t i) { m_contents.erase(i); }

	void erase(size_t i, size_t n) { m_contents.erase(i, n); }


	void clear() { m_contents.clear(); }
	void clear() volatile { m_contents.clear(); }

	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }

	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth) { m_contents.swap(wth.m_contents); }


	void advance_buffers(size_t n) { m_contents.advance_arrays(n); }
	void advance_buffers(size_t n) volatile { m_contents.advance_arrays(n); }

	void truncate_buffers(size_t n) { m_contents.truncate_arrays(n); }
	void truncate_buffers(size_t n) volatile { m_contents.truncate_arrays(n); }

	void truncate_buffers_to(size_t n) { m_contents.truncate_arrays_to(n); }
	void truncate_buffers_to(size_t n) volatile { m_contents.truncate_arrays_to(n); }

	void advance_buffer() { m_contents.advance_array(); }
	void truncate_buffer() { m_contents.truncate_array(); }

	string_t<type> pop_first_buffer() { return m_contents.pop_first_array(); }
	string_t<type> pop_first_buffer() volatile { return m_contents.pop_first_array(); }

	string_t<type> pop_last_buffer() { return m_contents.pop_last_array(); }
	string_t<type> pop_last_buffer() volatile { return m_contents.pop_last_array(); }

	this_t split_off_buffers_before(size_t i)
	{
		return this_t(m_contents.split_off_arrays_before(i));
	}

	this_t split_off_buffers_before(size_t i) volatile
	{
		return this_t(m_contents.split_off_arrays_before(i));
	}

	this_t split_off_buffers_after(size_t i)
	{
		return this_t(m_contents.split_off_arrays_after(i));
	}

	this_t split_off_buffers_after(size_t i) volatile
	{
		return this_t(m_contents.split_off_arrays_after(i));
	}


	this_t split_off_before(size_t i)
	{
		return this_t(m_contents.split_off_before(i));
	}

	this_t split_off_before(size_t i) volatile
	{
		return this_t(m_contents.split_off_before(i));
	}

	this_t split_off_before(const position_t& pos)
	{
		return this_t(m_contents.split_off_before(pos.m_pos));
	}

	this_t split_off_before(const position_t& pos) volatile
	{
		return this_t(m_contents.split_off_before(pos.m_pos));
	}

	this_t split_off_after(size_t i)
	{
		return this_t(m_contents.split_off_after(i));
	}

	this_t split_off_after(size_t i) volatile
	{
		return this_t(m_contents.split_off_after(i));
	}

	this_t split_off_after(const position_t& pos)
	{
		return this_t(m_contents.split_off_after(pos.m_pos));
	}

	this_t split_off_after(const position_t& pos) volatile
	{
		return this_t(m_contents.split_off_after(pos.m_pos));
	}


	this_t split_off_front(size_t n)
	{
		return this_t(m_contents.split_off_before(n));
	}

	this_t split_off_front(size_t n) volatile
	{
		return this_t(m_contents.split_off_before(n));
	}

	this_t split_off_back(size_t n)
	{
		return this_t(m_contents.split_off_back(n));
	}

	this_t split_off_back(size_t n) volatile
	{
		return this_t(m_contents.split_off_back(n));
	}

	void set_to_subrange(size_t i)
	{
		m_contents.set_to_subrange(i);
	}

	void set_to_subrange(size_t i) volatile
	{
		m_contents.set_to_subrange(i);
	}

	void set_to_subrange(size_t i, size_t n)
	{
		m_contents.set_to_subrange(i, n);
	}

	void set_to_subrange(size_t i, size_t n) volatile
	{
		m_contents.set_to_subrange(i, n);
	}

	void set_to_subrange(const position_t& start)
	{
		m_contents.set_to_subrange(start);
	}

	void set_to_subrange(const position_t& start) volatile
	{
		m_contents.set_to_subrange(start);
	}

	void set_to_subrange(const position_t& start, size_t n)
	{
		m_contents.set_to_subrange(start, n);
	}

	void set_to_subrange(const position_t& start, size_t n) volatile
	{
		m_contents.set_to_subrange(start, n);
	}

	void set_to_subrange(const position_t& start, const position_t& end)
	{
		m_contents.set_to_subrange(start, end);
	}

	void set_to_subrange(const position_t& start, const position_t& end) volatile
	{
		m_contents.set_to_subrange(start, end);
	}

	position_t get_end_position() const
	{
		return m_contents.get_end_position();
	}

	position_t get_end_position() const volatile
	{
		return m_contents.get_end_position();
	}

	position_t get_last_position() const
	{
		return m_contents.get_last_position();
	}

	position_t get_last_position() const volatile
	{
		return m_contents.get_last_position();
	}

	position_t get_next_position(const position_t& pos) const
	{
		return m_contents.get_next_position(pos.m_pos);
	}

	position_t get_next_position(const position_t& pos) const volatile
	{
		return m_contents.get_next_position(pos.m_pos);
	}

	// Caller error to call with (pos == 0, 0)
	position_t get_prev_position(const position_t& pos) const
	{
		return m_contents.get_prev_position(pos.m_pos);
	}

	// Caller error to call with (pos == 0, 0)
	position_t get_prev_position(const position_t& pos) const volatile
	{
		return m_contents.get_prev_position(pos.m_pos);
	}

	void advance(size_t n = 1)
	{
		m_contents.advance(n);
	}

	void advance(size_t n = 1) volatile
	{
		m_contents.advance(n);
	}

	void advance_to(const position_t& pos)
	{
		m_contents.advance_to(pos.m_pos);
	}

	void advance_to(const position_t& pos) volatile
	{
		m_contents.advance_to(pos.m_pos);
	}

	void truncate(size_t n)
	{
		m_contents.truncate(n);
	}

	void truncate(size_t n) volatile
	{
		m_contents.truncate(n);
	}

	void truncate_to(size_t n)
	{
		m_contents.truncate_to(n);
	}

	void truncate_to(size_t n) volatile
	{
		m_contents.truncate_to(n);
	}

	void truncate_to(const position_t& pos)
	{
		m_contents.truncate_to(pos.m_pos);
	}

	void truncate_to(const position_t& pos) volatile
	{
		m_contents.truncate_to(pos.m_pos);
	}

	void truncate_to_right(size_t n)
	{
		m_contents.truncate_to_right(n);
	}

	void truncate_to_right(size_t n) volatile
	{
		m_contents.truncate_to_right(n);
	}

	void truncate_to_right(const position_t& pos)
	{
		m_contents.truncate_to_right(pos.m_pos);
	}

	void truncate_to_right(const position_t& pos) volatile
	{
		m_contents.truncate_to_right(pos.m_pos);
	}

	vector<this_t> split_on(const char_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(*this, &splitOn, 1, opt);
	}

	vector<this_t> split_on(const char_t& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(tmp, &splitOn, 1, opt);
	}

	vector<this_t> split_on_any(const char_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(*this, splitOn, n, opt);
	}

	vector<this_t> split_on_any(const char_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(tmp, splitOn, n, opt);
	}

	vector<this_t> split_on_any(const vector<char_t>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<this_t> split_on_any(const vector<char_t>& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(tmp, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<this_t> split_on_any(const volatile vector<char_t>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		vector<char_t> tmp(splitOn);
		return composite_vector<char_t>::template split_on_any_inner<this_t, char_t>(*this, tmp.get_const_ptr(), tmp.get_length(), opt);
	}

	void trim_start()
	{
		const_iterator itor = get_first_const_iterator();
		while (!!itor)
		{
			if (!string_t<type>::is_white_space(*itor))
			{
				this->advance_to(itor.get_position());
				return;
			}

			++itor;
		}

		// It must have all been white space.
		clear();
	}

	void trim_end()
	{
		const_iterator lastItor = get_last_const_iterator();
		const_iterator itor = lastItor;
		while (!!itor)
		{
			if (!string_t<type>::is_white_space(*itor))
			{
				if (itor != lastItor)
				{
					++itor;
					this->truncate_to(itor.get_position());
				}
				return;
			}

			--itor;
		}

		// It must have all been white space.
		clear();
	}

	void trim()
	{
		trim_start();
		trim_end();
	}

	this_t get_trimmed()
	{
		this_t result(*this);
		result.trim();
		return result;
	}

	this_t get_trimmed() volatile
	{
		this_t result(*this);
		result.trim();
		return result;
	}

	this_t get_trimmed_start()
	{
		this_t result(*this);
		result.trim_start();
		return result;
	}

	this_t get_trimmed_start() volatile
	{
		this_t result(*this);
		result.trim_start();
		return result;
	}

	this_t get_trimmed_end()
	{
		this_t result(*this);
		result.trim_end();
		return result;
	}

	this_t get_trimmed_end() volatile
	{
		this_t result(*this);
		result.trim_end();
		return result;
	}

	template <typename int_t>
	int_t to_int(unsigned int radix = 0) const // Max radix is 36.  Radix of 0 defaults to dec but auto-detects oct and hex 
	{
		int_t result = 0;

		bool gotRadix = radix >= 2;
		if (!gotRadix)
			radix = 10;
		else if (radix > 36)
			radix = 36;

		bool neg = false;
		this_t tmp(*this);
		tmp.trim();

		for (;;)
		{
			const_iterator itor = tmp.get_first_iterator();
			if (!itor)
				break;

			if (*itor == (type)'+')
				tmp.advance();
			else if (*itor == (type)'-')
			{
				neg = !neg;
				tmp.advance();
			}
			else
				break;
		}

		// Try to deduce the radix if not specified
		if (!gotRadix)
		{
			const_iterator itor = tmp.get_first_iterator();
			if (!!itor && (*itor == (type)'0'))
			{
				radix = 8;
				tmp.advance();
				const_iterator itor = tmp.get_first_iterator();
				if (!!itor && ((*itor == (type)'x') || (*itor == (type)'X')))
				{
					radix = 16;
					tmp.advance();
				}
			}
		}

		type maxDecimalDigit = (radix < 10) ? (((type)radix - 1) + (type)'0') : (type)'9';
		if (radix > 10)
		{
			type maxAlphaDigitLower = ((type)radix - 11) + (type)'a';
			type maxAlphaDigitUpper = ((type)radix - 11) + (type)'A';
			const_iterator itor = tmp.get_first_iterator();
			while (!!itor)
			{
				type c = *itor;
				if (c == (type)'.') // stop at a deciminal place
					break;
				if ((c >= (type)'0') && (c <= maxDecimalDigit))
				{
					result *= radix;
					result += c - (type)'0';
				}
				else if ((c >= (type)'a') && (c <= maxAlphaDigitLower))
				{
					result *= radix;
					result += 10 + (c - (type)'a');
				}
				else if ((c >= (type)'A') && (c <= maxAlphaDigitUpper))
				{
					result *= radix;
					result += 10 + (c - (type)'A');
				}
				else //if (!is_white_space(c)) // unhandleable characters cause 0 to be returned
				{
					result = 0;
					break;
				}

				++itor;
			}
		}
		else
		{
			const_iterator itor = tmp.get_first_iterator();
			while (!!itor)
			{
				type c = *itor;
				if (c == (type)'.') // stop at a deciminal place
					break;
				if ((c >= (type)'0') && (c <= maxDecimalDigit))
				{
					result *= radix;
					result += c - (type)'0';
				}
				else //if (!is_white_space(c)) // unhandleable characters cause 0 to be returned
				{
					result = 0;
					break;
				}

				++itor;
			}
		}

		if (neg)
			result = negate_if_signed<int_t>::get(result);
		return result;
	}
};


template <typename type, typename type2>
class case_insensitive_comparator<composite_string_t<type>, composite_string_t<type2> >
{
public:
	static bool is_less_than(const composite_string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t1.is_less_than(t2, is_case_insensitive);
	}

	static bool is_greater_than(const composite_string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t1.is_greater_than(t2, is_case_insensitive);
	}

	static bool equals(const composite_string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t1.equals(t2, is_case_insensitive);
	}

	static int compare(const composite_string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t1.compare(t2, is_case_insensitive);
	}
};


template <typename type, typename type2>
class case_insensitive_comparator<composite_string_t<type>, string_t<type2> >
{
public:
	static bool is_less_than(const composite_string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.is_less_than(t2, is_case_insensitive);
	}

	static bool is_greater_than(const composite_string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.is_greater_than(t2, is_case_insensitive);
	}

	static bool equals(const composite_string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.equals(t2, is_case_insensitive);
	}

	static int compare(const composite_string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.compare(t2, is_case_insensitive);
	}
};


template <typename type, typename type2>
class case_insensitive_comparator<string_t<type>, composite_string_t<type2> >
{
public:
	static bool is_less_than(const string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t2.is_greater_than(t1, is_case_insensitive);
	}

	static bool is_greater_than(const string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t2.is_less_than(t1, is_case_insensitive);
	}

	static bool equals(const string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t2.equals(t1, is_case_insensitive);
	}

	static int compare(const string_t<type>& t1, const composite_string_t<type2>& t2)
	{
		return t2.compare(t1, is_case_insensitive);
	}
};


template <typename type>
composite_string_t<type> operator+(const string_t<type>& src1, const string_t<type>& src2)
{
	composite_string_t<type> result(src1);
	result += src2;
	return result;
}

template <typename type>
composite_string_t<type> operator+(const string_t<type>& src1, const composite_string_t<type>& src2)
{
	composite_string_t<type> result(src1);
	result += src2;
	return result;
}

template <typename type>
composite_string_t<type> operator+(const composite_string_t<type>& src1, const composite_string_t<type>& src2)
{
	composite_string_t<type> result(src1);
	result += src2;
	return result;
}

template <typename type>
composite_string_t<type> operator+(const composite_string_t<type>& src1, const string_t<type>& src2)
{
	composite_string_t<type> result(src1);
	result += src2;
	return result;
}


typedef composite_string_t<wchar_t> composite_string;

typedef composite_string_t<char> composite_cstring;



inline cstring string_to_cstring(const composite_string& s)
{
	cstring s2;
	s2.resize(s.get_length());
	char* s2Ptr = s2.get_ptr();
	composite_string::const_iterator itor = s.get_first_const_iterator();
	for (size_t i = 0; i < s.get_length(); i++)
		s2Ptr[i] = (char)*(itor++);
	return s2;
}

inline string cstring_to_string(const composite_cstring& s)
{
	string s2;
	s2.resize(s.get_length());
	wchar_t* s2Ptr = s2.get_ptr();
	composite_cstring::const_iterator itor = s.get_first_const_iterator();
	for (size_t i = 0; i < s.get_length(); i++)
		s2Ptr[i] = (wchar_t)*(itor++);
	return s2;
}



template <typename T> inline std::enable_if_t<std::is_integral_v<T>, composite_string>
to_string(const T& t)
{
	int_to_fixed_integer_t<T> tmp(t);
	return tmp.to_string();
}

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, composite_string>
to_string(const T& t)
{
	std::wstring s1 = std::to_wstring(t);
	string s2(s1.data(), s1.size());
	return s2;
	// Restore once std::to_chars is implemented
	//return cstring_to_string(to_cstring(t));
}

template <typename T> inline std::enable_if_t<std::is_integral_v<T>, composite_cstring>
to_cstring(const T& t)
{
	int_to_fixed_integer_t<T> tmp(t);
	return tmp.to_cstring();
}

template <typename T> inline std::enable_if_t<std::is_floating_point_v<T>, composite_cstring>
to_cstring(const T& t)
{
	std::string s1 = std::to_string(t);
	cstring s2(s1.data(), s1.size());
	return s2;

	// Restore once std::to_chars is implemented
	//static constexpr size_t max_digits = 3 + DBL_MANT_DIG - DBL_MIN_EXP;
	//cstring s;
	//s.resize(max_digits);
	//char* cptr = s.get_ptr();
	//std::to_chars_result result = std::to_chars(cptr, cptr + (max_digits - 1), t);
	//s.resize(result.ptr - cptr);
	//return s;
}

}



#pragma warning(pop)

#endif
