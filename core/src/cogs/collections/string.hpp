//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_STRING
#define COGS_HEADER_COLLECTION_STRING


#include "cogs/operators.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/env.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/negate_if_signed.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"


namespace cogs {

#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified

namespace io
{
	class buffer;
}

// TODO
//class character_encoding


template <typename T>
class string_t;


/// @ingroup Collections
/// @brief Text case sensitivity of a string comparison
enum is_case_sensitive_t
{
	/// @brief The string comparison is case insensitive
	is_case_insensitive = 0,

	/// @brief The string comparison is case sensitive
	is_case_sensitive = 1
};

/// @ingroup Collections
/// @brief A case sensitive comparator
template <typename T1, typename T2 = T1>
class case_sensitive_comparator
{
public:
	static bool is_less_than(const T1& t1, const T2& t2)
	{
		return default_comparator::is_less_than(t1, t2);
	}

	static bool is_greater_than(const T1& t1, const T2& t2)
	{
		return default_comparator::is_greater_than(t1, t2);
	}

	static bool equals(const T1& t1, const T2& t2)
	{
		return default_comparator::equals(t1, t2);
	}

	static int compare(const T1& t1, const T2& t2)
	{
		return default_comparator::compare(t1, t2);
	}
};

/// @ingroup Collections
/// @brief A case sensitive comparator
template <typename T1, typename T2 = T1>
class case_insensitive_comparator
{
public:
	static bool is_less_than(const T1& t1, const T2& t2)
	{
		return default_comparator::is_less_than(toupper(t1), toupper(t2));
	}

	static bool is_greater_than(const T1& t1, const T2& t2)
	{
		return default_comparator::is_greater_than(toupper(t1), toupper(t2));
	}

	static bool equals(const T1& t1, const T2& t2)
	{
		return default_comparator::equals(toupper(t1), toupper(t2));
	}

	static int compare(const T1& t1, const T2& t2)
	{
		return default_comparator::compare(toupper(t1), toupper(t2));
	}
};

/// @ingroup LockFreeCollections
/// @brief A text string vector.  Uses vector internally.
/// @tparam T Character type.  Usually char or wchar_t.
template <typename T>
class string_t
{
public:
	typedef T type;
	typedef type char_t;

	typedef string_t<type>	this_t;

protected:
	template <typename>
	friend class string_t;

	template <typename>
	friend class composite_string_t;

	friend class io::buffer;

	typedef vector<type> vector_t;

	vector_t	m_contents;

	static size_t count_chars(const type* ltrl)	// A good compiler should be able to optimize this out, if used with a literal.
	{
		size_t l = 0;
		if (ltrl)
			while (!!(ltrl[l]))
				++l;
		return l;
	}

	string_t(size_t n, const type* src) : m_contents(vector_t::contain(src, n)) { }

	explicit string_t(const vector_t& v) : m_contents(v) { }

	void set(typename vector_t::desc_t* d, type* p, size_t n)	{ m_contents.set(d, p, n); }

	void disown()	{ m_contents.disown(); }

	typename vector_t::desc_t* get_desc() const	{ return m_contents.get_desc(); }
	type* get_raw_ptr()const					{ return m_contents.get_raw_ptr(); }

public:
	operator vector_t&() { return m_contents; }
	operator const vector_t&() const { return m_contents; }
	operator volatile vector_t&() const { return m_contents; }
	operator const volatile vector_t&() const { return m_contents; }

	vector_t& get_vector()					{ return m_contents; }
	const          vector_t& get_vector() const				{ return m_contents; }
	      volatile vector_t& get_vector()       volatile	{ return m_contents; }
	const volatile vector_t& get_vector() const volatile	{ return m_contents; }

	static this_t& from_vector(vector_t& v, unowned_t<this_t>& storage = unowned_t<this_t>().get_unowned())
	{
		storage.m_contents.set(v.get_desc(), v.get_raw_ptr(), v.get_length());
		return storage;
	}

	static const this_t& from_vector(const vector_t& v, unowned_t<this_t>& storage = unowned_t<this_t>().get_unowned())
	{
		storage.m_contents.set(v.get_desc(), v.get_raw_ptr(), v.get_length());
		return storage;
	}

	/// @brief A string iterator
	class iterator
	{
	private:
		template <typename>
		friend class string_t;

		typename vector_t::iterator m_iterator;

		iterator(const typename vector_t::iterator& i)
			: m_iterator(i)
		{ }

	public:
		iterator() { }
		iterator(const iterator& i) : m_iterator(i.m_iterator) { }

		void release() { m_iterator.release(); }

		iterator& operator++() { ++m_iterator; return *this; }
		iterator& operator--() { --m_iterator; return *this; }

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_iterator; }

		bool operator==(const iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		iterator& operator=(const iterator& i) { m_iterator = i.m_iterator; return *this; }

		type* get() const { m_iterator.get(); }
		type& operator*() const { return *get(); }
		type* operator->() const { return get(); }

		size_t get_position() const { m_iterator.get_position(); }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }
	};

	/// @brief A string const iterator
	class const_iterator
	{
	private:
		template <typename>
		friend class string_t;

		typename vector_t::const_iterator m_iterator;

		const_iterator(const typename vector_t::const_iterator& i)
			: m_iterator(i)
		{ }

	public:
		const_iterator() { }
		const_iterator(const const_iterator& i) : m_iterator(i.m_iterator) { }
		const_iterator(const iterator& i) : m_iterator(i.m_iterator) { }

		void release() { m_iterator.release(); }

		const_iterator& operator++() { ++m_iterator; return *this; }
		const_iterator& operator--() { --m_iterator; return *this; }

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_iterator; }

		bool operator==(const const_iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator==(const iterator& i) const { return m_iterator == i.m_iterator; }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_iterator = i.m_iterator; return *this; }
		const_iterator& operator=(const iterator& i) { m_iterator = i.m_iterator; return *this; }

		const type* get() const { m_iterator.get(); }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return get(); }

		size_t get_position() const { m_iterator.get_position(); }

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }
	};

	const_iterator get_first_const_iterator() const		{ return m_contents.get_first_const_iterator(); }
	const_iterator get_last_const_iterator() const		{ return m_contents.get_last_const_iterator(); }

	iterator get_first_iterator()						{ return m_contents.get_first_iterator(); }
	iterator get_last_iterator()						{ return m_contents.get_last_iterator(); }

	static this_t literal(const type* src)				{ return this_t(count_chars(src), src); }
	static this_t literal(type& src)					{ return this_t(1, &src); }

	static this_t contain(const type* src)				{ return this_t(count_chars(src), src); }
	static this_t contain(const type* src, size_t n)	{ return this_t(n, src); }


	string_t(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}


	string_t()
	{ }

	string_t(const this_t& src)
		:	m_contents(src.m_contents)
	{ }

	string_t(const volatile this_t& src)		
		:	m_contents(src.m_contents)
	{ }

	string_t(const type& src)
		:	m_contents(1, src)
	{ }

	string_t(size_t n, const type& src)
		: m_contents(n, src)
	{ }

	string_t(const type* src)
		: m_contents(src, count_chars(src))
	{ }

	string_t(const type* src, size_t n)
		: m_contents(src, n)
	{ }

	string_t(const this_t& src, size_t i)
		:	m_contents(src.m_contents, i)		
	{ }
	
	string_t(const volatile this_t& src, size_t i)
		:	m_contents(src.m_contents, i)		
	{ }
	
	string_t(const this_t& src, size_t i, size_t n)
		:	m_contents(src.m_contents, i, n)	
	{ }
	
	string_t(const volatile this_t& src, size_t i, size_t n)
		:	m_contents(src.m_contents, i, n)	
	{ }
	
	bool is_unowned() const						{ return m_contents.is_unowned(); }
	bool is_unowned() const volatile			{ return m_contents.is_unowned(); }

	bool is_owned() const						{ return m_contents.is_owned(); }
	bool is_owned() const volatile				{ return m_contents.is_owned(); }

	bool is_shared() const						{ return m_contents.is_shared(); }
	bool is_shared() const volatile				{ return m_contents.is_shared(); }

	size_t get_length() const					{ return m_contents.get_length(); }
	size_t get_length() const volatile			{ return m_contents.get_length(); }

	size_t get_capacity() const					{ return m_contents.get_capacity(); }
	size_t get_reverse_capacity() const			{ return m_contents.get_reverse_capacity(); }

	bool is_empty() const						{ return m_contents.is_empty(); }
	bool is_empty() const volatile				{ return m_contents.is_empty(); }

	bool operator!() const						{ return !m_contents; }
	bool operator!() const volatile				{ return !m_contents; }

	const type* get_const_ptr() const			{ return m_contents.get_const_ptr(); }
	
	// caller error to pass index >= length
	const type& operator[](size_t i) const		{ return m_contents[i]; }

	const this_t& subrange(size_t i, size_t n = const_max_int_v<size_t>, unowned_t<this_t>& storage = unowned_t<this_t>().get_unowned()) const
	{
		size_t length = get_length();
		if (i <= length)
		{
			size_t adjustedLength = length - i;
			if (adjustedLength > n)
				adjustedLength = n;
			storage.m_contents.set(m_contents.get_desc(), m_contents.get_raw_ptr() + i, adjustedLength);
		}
		return storage;
	}

	this_t subrange(size_t i, size_t n = const_max_int_v<size_t>) const volatile	{ this_t s(*this, i, n); return s; }


	bool equals(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return (get_length() == 1) && case_insensitive_comparator<type>::equals(get_const_ptr()[0], cmp);
	}

	bool equals(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		this_t tmp(*this);
		return tmp.equals(cmp, caseSensitivity);
	}

	bool equals(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.equals(cmp, n);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool equals(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.equals(cmp, n);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool equals(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return equals(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool equals(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return equals(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool equals(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.equals(cmp.m_contents);
		return m_contents.template equals<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}


	bool operator==(const type& src) const { return equals(src); }

	bool operator==(const type& src) const volatile { return equals(src); }

	bool operator==(const this_t& src) const { return equals(src); }

	bool operator==(const this_t& src) const volatile { return equals(src); }

	bool operator==(const volatile this_t& src) const { return equals(src); }

	bool operator!=(const this_t& src) const { return !equals(src); }

	bool operator!=(const this_t& src) const volatile { return !equals(src); }

	bool operator!=(const volatile this_t& src) const { return !equals(src); }


	int compare(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.compare(cmp, n);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(cmp, n);
	}

	int compare(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.compare(cmp, n);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(cmp, n);
	}

	int compare(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return compare(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	int compare(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return compare(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	int compare(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.compare(cmp.m_contents);
		return m_contents.template compare<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}



	bool starts_with(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return (get_length() >= 1) && case_insensitive_comparator<type>::equals(get_const_ptr()[0], cmp);
	}

	bool starts_with(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		this_t tmp(*this);
		return tmp.starts_with(cmp, caseSensitivity);
	}

	bool starts_with(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.starts_with(cmp, n);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool starts_with(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.starts_with(cmp, n);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool starts_with(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return starts_with(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool starts_with(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return starts_with(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool starts_with(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.starts_with(cmp.m_contents);
		return m_contents.template starts_with<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}



	bool ends_with(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		size_t length = get_length();
		return (length >= 1) && case_insensitive_comparator<type>::equals(get_const_ptr()[length - 1], cmp);
	}

	bool ends_with(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		this_t tmp(*this);
		return tmp.ends_with(cmp, caseSensitivity);
	}

	bool ends_with(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.ends_with(cmp, n);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool ends_with(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.ends_with(cmp, n);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool ends_with(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return ends_with(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool ends_with(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return ends_with(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool ends_with(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.ends_with(cmp.m_contents);
		return m_contents.template ends_with<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}





	bool is_less_than(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_less_than(cmp, n);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool is_less_than(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_less_than(cmp, n);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool is_less_than(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return is_less_than(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool is_less_than(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return is_less_than(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool is_less_than(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_less_than(cmp.m_contents);
		return m_contents.template is_less_than<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}



	bool is_greater_than(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_greater_than(cmp, n);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool is_greater_than(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_greater_than(cmp, n);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool is_greater_than(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		return is_greater_than(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool is_greater_than(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		return is_greater_than(cmp.get_const_ptr(), cmp.get_length(), caseSensitivity);
	}

	bool is_greater_than(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.is_greater_than(cmp.m_contents);
		return m_contents.template is_greater_than<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}



	bool operator>(const this_t& src) const			{ return is_greater_than(src); }

	bool operator>(const volatile this_t& src) const		 			{ return is_greater_than(src); }

	bool operator>(const this_t& src) const volatile	{ return is_greater_than(src); }


	bool operator<(const this_t& src) const 			{ return is_less_than(src); }

	bool operator<(const volatile this_t& src) const					{ return is_less_than(src); }

	bool operator<(const this_t& src) const volatile	{ return is_less_than(src); }


	bool operator>=(const this_t& src) const 			{ return !is_less_than(src); }

	bool operator>=(const volatile this_t& src) const				{ return !is_less_than(src); }

	bool operator>=(const this_t& src) const volatile { return !is_less_than(src); }


	bool operator<=(const this_t& src) const			{ return !is_greater_than(src); }

	bool operator<=(const volatile this_t& src) const				{ return !is_greater_than(src); }

	bool operator<=(const this_t& src) const volatile	{ return !is_greater_than(src); }



	size_t index_of(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp);
	}

	size_t index_of(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{ 
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp);
	}


	size_t index_of_any(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(cmp, n);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, n);
	}

	size_t index_of_any(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(cmp, n);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, n);
	}

	size_t index_of_any(size_t i, const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(i, cmp, n);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, cmp, n);
	}

	size_t index_of_any(size_t i, const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(i, cmp, n);
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(i, cmp, n);
	}


	size_t index_of(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp);
	}

	size_t index_of(size_t i, const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile	
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp);
	}



	size_t index_of(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp, n);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp, n);
	}


	size_t index_of(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile	
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp, n);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp, n);
	}

	size_t index_of(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}

	size_t index_of(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}

	size_t index_of(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}



	size_t index_of(size_t i, const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp, n);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp, n);
	}

	size_t index_of(size_t i, const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp, n);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp, n);
	}

	size_t index_of(size_t i, const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp.m_contents);
	}

	size_t index_of(size_t i, const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp.m_contents);
	}

	size_t index_of(size_t i, const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of(i, cmp.m_contents);
		return m_contents.template index_of<type, case_insensitive_comparator<type> >(i, cmp.m_contents);
	}

	
	bool contains(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains(cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(cmp);
	}
	
	bool contains(const type& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains(cmp);
		return m_contents.template contains<type, case_insensitive_comparator<type> >(cmp);
	}

	bool contains_any(const char& cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(cmp, n) != const_max_int_v<size_t>;
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, n) != const_max_int_v<size_t>;
	}

	bool contains_any(const char& cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.index_of_any(cmp, n) != const_max_int_v<size_t>;
		return m_contents.template index_of_any<type, case_insensitive_comparator<type> >(cmp, n) != const_max_int_v<size_t>;
	}

	bool contains_segment(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains_segment(cmp, n);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp, n);
	}
	
	bool contains_segment(const type* cmp, size_t n, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains_segment(cmp, n);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp, n);
	}

	bool contains_segment(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains_segment(cmp.m_contents);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}

	bool contains_segment(const this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const volatile
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains_segment(cmp.m_contents);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}

	bool contains_segment(const volatile this_t& cmp, is_case_sensitive_t caseSensitivity = is_case_sensitive) const
	{
		if (caseSensitivity == is_case_sensitive)
			return m_contents.contains_segment(cmp.m_contents);
		return m_contents.template contains_segment<type, case_insensitive_comparator<type> >(cmp.m_contents);
	}




	this_t operator+(const type& src) const									{ this_t tmp(*this); tmp += src; return tmp; }

	this_t operator+(const this_t& src) const			{ this_t tmp(*this); tmp += src; return tmp; }

	this_t operator+(const volatile this_t& src) const					{ this_t tmp(*this); tmp += src; return tmp; }

	this_t operator+(const this_t& src) const volatile	{ this_t tmp(*this); tmp += src; return tmp; }


	type* get_ptr()										{ return m_contents.get_ptr(); }

	void set_index(size_t i, const type& src)			{ m_contents.set_index(i, src); }

	void reverse()										{ m_contents.reverse(); }
	
	void set_to_subrange(size_t i)						{ m_contents.set_to_subrange(i); }
	void set_to_subrange(size_t i) volatile				{ m_contents.set_to_subrange(i); }

	void set_to_subrange(size_t i, size_t n)			{ m_contents.set_to_subrange(i, n); }

	void set_to_subrange(size_t i, size_t n) volatile	{ m_contents.set_to_subrange(i, n); }

	void clear()										{ m_contents.clear(); }
	void clear() volatile								{ m_contents.clear(); }

	// clears reserved space as well	
	void reset()										{ m_contents.reset(); }
	void reset() volatile								{ m_contents.reset(); }

	void reserve(size_t n)					{ m_contents.reserve(n); }

	size_t get_capacity()			{ return m_contents.get_capacity(); }
	size_t get_reverse_capacity()	{ return m_contents.get_reverse_capacity(); }

	void assign(const type& src)						{ m_contents.assign(1, src); }
	void assign(const type& src) volatile				{ m_contents.assign(1, src); }
	void assign(size_t n, const type& src)				{ m_contents.assign(n, src); }
	void assign(size_t n, const type& src) volatile		{ m_contents.assign(n, src); }
	void assign(const type* src, size_t n)				{ m_contents.assign(src, n); }
	void assign(const type* src, size_t n) volatile		{ m_contents.assign(src, n); }
	void assign(const this_t& src)						{ m_contents.assign(src.m_contents); }
	void assign(const this_t& src) volatile				{ m_contents.assign(src.m_contents); }
	void assign(const volatile this_t& src)				{ m_contents.assign(src.m_contents); }

	this_t& operator=(const type& src)					{ m_contents.assign(1, src); return *this; }
	this_t operator=(const type& src) volatile			{ this_t tmp(1, src); m_contents.assign(tmp); return tmp; }
	this_t& operator=(const this_t& src)				{ assign(src); return *this; }
	this_t operator=(const this_t& src) volatile		{ assign(src); return src; }
	this_t& operator=(const volatile this_t& src)		{ assign(src); return *this; }

	void append(const type& src)						{ m_contents.append(1, src); }
	void append(size_t n, const type& src)				{ m_contents.append(n, src); }
	void append(const type* src, size_t n)				{ m_contents.append(src, n); }
	void append(const this_t& src)						{ m_contents.append(src.m_contents); }
	void append(const volatile this_t& src)				{ m_contents.append(src.m_contents); }
	
	this_t& operator+=(const type& src)					{ append(src); return *this; }
	this_t& operator+=(const this_t& src)				{ append(src); return *this; }
	this_t& operator+=(const volatile this_t& src)		{ append(src); return *this; }
	
	void prepend(const type& src)						{ m_contents.prepend(1, src); }
	void prepend(size_t n, const type& src)				{ m_contents.prepend(n, src); }
	void prepend(const type* src, size_t n)				{ m_contents.prepend(src, n); }
	void prepend(const this_t& src)						{ m_contents.prepend(src.m_contents); }
	void prepend(const volatile this_t& src)			{ m_contents.prepend(src.m_contents); }

	
	void resize(size_t n)						{ m_contents.resize(n); }

	template <typename type2 = type>
	void resize(size_t n, const type2& src)		{ m_contents.resize(n, src); }

	void erase(size_t i)						{ m_contents.erase(i); }
	void erase(size_t i, size_t n)				{ m_contents.erase(i, n); }

	void advance(size_t n = 1)					{ m_contents.advance(n); }

	void truncate_to(size_t n)					{ m_contents.truncate_to(n); }
	
	void truncate(size_t n)						{ m_contents.truncate(n); }

	void truncate_to_right(size_t n)			{ m_contents.truncate_to_right(n); }

	
	void insert(size_t i, size_t n)						{ m_contents.insert(i, n); }
	void insert(size_t i, size_t n, const type& src)	{ m_contents.insert(i, n, src); }
	void insert(size_t i, const type* src, size_t n)			{ m_contents.insert(i, src, n); }
	void insert(size_t i, const this_t& src)	{ m_contents.insert(i, src.m_contents); }
	void insert(size_t i, const volatile this_t& src)		{ m_contents.insert(i, src.m_contents); }

	void insert(size_t i, const this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert(i, src.subrange(srcIndex, n));
	}

	void insert(size_t i, const volatile this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert(i, src.subrange(srcIndex, n));
	}



	void replace(size_t i, size_t replaceLength, const type& src)			{ m_contents.replace(i, replaceLength, src); }

	void replace(size_t i, const type* src, size_t replaceLength)			{ m_contents.replace(i, src, replaceLength); }

	void replace(size_t i, const this_t& src)		{ m_contents.replace(i, src.m_contents); }

	void replace(size_t i, const volatile this_t& src)			{ m_contents.replace(i, src.m_contents); }


	void replace(size_t i, const this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		replace(i, src.subrange(srcIndex, n));
	}

	void replace(size_t i, const volatile this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		replace(i, src.subrange(srcIndex, n));
	}


	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, const type& src)	{ m_contents.insert_replace(i, replaceLength, insertLength, src); }

	void insert_replace(size_t i, size_t replaceLength, type* src, size_t insertLength)	{ m_contents.insert_replace(i, replaceLength, src, insertLength); }

	void insert_replace(size_t i, size_t replaceLength, const this_t& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }

	void insert_replace(size_t i, size_t replaceLength, const volatile this_t& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }


	void insert_replace(size_t i, size_t replaceLength, const this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert_replace(i, replaceLength, src.subrange(srcIndex, n));
	}

	void insert_replace(size_t i, size_t replaceLength, const volatile this_t& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert_replace(i, replaceLength, src.subrange(srcIndex, n));
	}



	void swap(this_t& wth)					{ m_contents.swap(wth.m_contents); }

	void swap(this_t& wth) volatile			{ m_contents.swap(wth.m_contents); }

	void swap(volatile this_t& wth)			{ wth.swap(*this); }

	this_t split_off_before(size_t i)
	{
		this_t result;
		result.m_contents = m_contents.split_off_before(i);
		return result;
	}

	this_t split_off_before(size_t i) volatile
	{
		this_t result;
		result.m_contents = m_contents.split_off_before(i);
		return result;
	}

	this_t split_off_after(size_t i)
	{
		this_t result;
		result.m_contents = m_contents.split_off_after(i);
		return result;
	}


	this_t split_off_after(size_t i) volatile
	{
		this_t result;
		result.m_contents = m_contents.split_off_after(i);
		return result;
	}

	this_t split_off_front(size_t n)
	{
		this_t result;
		result.m_contents = m_contents.split_off_front(n);
		return result;
	}
	
	this_t split_off_front(size_t n) volatile
	{
		this_t result;
		result.m_contents = m_contents.split_off_front(n);
		return result;
	}
	
	this_t split_off_back(size_t n)
	{
		this_t result;
		result.m_contents = m_contents.split_off_back(n);
		return result;
	}

	this_t split_off_back(size_t n) volatile
	{
		this_t result;
		result.m_contents = m_contents.split_off_back(n);
		return result;
	}
	
	vector<this_t> split_on(const type& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return vector_t::template split_on_any_inner<this_t>(*this, &splitOn, 1, opt);
	}

	vector<this_t> split_on(const type& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return vector_t::template split_on_any_inner<this_t>(tmp, &splitOn, 1, opt);
	}

	vector<this_t> split_on_any(const type* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return vector_t::template split_on_any_inner<this_t>(*this, splitOn, n, opt);
	}

	vector<this_t> split_on_any(const type* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return vector_t::template split_on_any_inner<this_t>(tmp, splitOn, n, opt);
	}


	vector<this_t> split_on_any(const vector<type>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return vector_t::template split_on_any_inner<this_t>(*this, splitOn, opt);
	}

	vector<this_t> split_on_any(const volatile vector<type>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		vector<type> tmp(splitOn);
		return vector_t::template split_on_any_inner<this_t>(*this, tmp, opt);
	}

	vector<this_t> split_on_any(const vector<type>& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return vector_t::template split_on_any_inner<this_t>(tmp, splitOn, opt);
	}

	vector<this_t> split_on_segment(const type* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return vector_t::template split_on_segment_inner<this_t>(*this, splitOn, n, opt);
	}

	vector<this_t> split_on_segment(const type* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return vector_t::template split_on_segment_inner<this_t>(tmp, splitOn, n, opt);
	}

	vector<this_t> split_on_segment(const this_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return vector_t::template split_on_segment_inner<this_t>(*this, splitOn.m_contents, opt);
	}

	vector<this_t> split_on_segment(const volatile this_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		this_t tmp(splitOn);
		return vector_t::template split_on_segment_inner<this_t>(*this, tmp.m_contents, opt);
	}

	vector<this_t> split_on_segment(const this_t& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return vector_t::template split_on_segment_inner<this_t>(tmp, splitOn.m_contents, opt);
	}

	const type* cstr() const
	{
		static constexpr type empty_cstr = (type)0;
		size_t length = get_length();
		if (!length)
			return &empty_cstr;	// rather than allocate a buffer just for a terminator

		// Unforunately, this means that a terminated string literal will be copied if extracted using cstr().
		vector_t& tmp = const_cast<this_t*>(this)->m_contents;
		tmp.reserve(length + 1);
		type* ptr = tmp.get_ptr();
		ptr[length] = 0;
		return ptr;
	}

	static bool is_white_space(const type& c);

	void trim_start()
	{
		size_t length = get_length();
		if (!!length)
		{
			size_t firstNonWhiteSpace = 0;
			for (;;)
			{
				if (!is_white_space(m_contents[firstNonWhiteSpace]))
				{
					set_to_subrange(firstNonWhiteSpace);
					break;
				}
				if (++firstNonWhiteSpace == length)
				{
					clear();
					break;
				}
			}
		}
	}

	void trim_end()
	{
		size_t length = get_length();
		if (!!length)
		{
			size_t lastNonWhiteSpace = length - 1;
			for (;;)
			{
				if (!is_white_space(m_contents[lastNonWhiteSpace]))
				{
					truncate_to(lastNonWhiteSpace + 1);
					break;
				}
				if (!lastNonWhiteSpace)
				{
					clear();
					break;
				}
				--lastNonWhiteSpace;
			}
		}
	}

	void trim()
	{
		size_t length = get_length();
		if (!!length)
		{
			size_t firstNonWhiteSpace = 0;
			for (;;)
			{
				if (!is_white_space(m_contents[firstNonWhiteSpace]))
				{
					set_to_subrange(firstNonWhiteSpace);
					length -= firstNonWhiteSpace;
					size_t lastNonWhiteSpace = length - 1;
					while (is_white_space(m_contents[lastNonWhiteSpace]))
						--lastNonWhiteSpace;
					truncate_to(lastNonWhiteSpace + 1);
					break;
				}
				if (++firstNonWhiteSpace == length)
				{
					clear();
					break;
				}
			}
		}
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
	
	static type get_uppercase(const type& c);
	static type get_lowercase(const type& c);

	this_t get_trimmed_end() volatile
	{
		this_t result(*this);
		result.trim_end();
		return result;
	}

	this_t get_uppercase() const
	{
		this_t result;
		size_t length = get_length();
		if (!!length)
		{
			result.resize(length);
			const type* src = get_const_ptr();
			const type* dst = result.get_ptr();
			for (size_t i = 0; i < length; i++)
				dst[i] = get_uppercase(src[i]);
		}
		return result;
	}


	this_t get_uppercase() const volatile
	{
		this_t result(*this);
		result.to_uppercase();
		return result;
	}

	this_t get_lowercase() const
	{
		this_t result;
		size_t length = get_length();
		if (!!length)
		{
			result.resize(length);
			const type* src = get_const_ptr();
			const type* dst = result.get_ptr();
			for (size_t i = 0; i < length; i++)
				dst[i] = get_lowercase(src[i]);
		}
		return result;
	}

	this_t get_lowercase() const volatile
	{
		this_t result(*this);
		result.to_lowercase();
		return result;
	}

	void to_uppercase()
	{
		size_t length = get_length();
		if (!!length)
		{
			type* ptr = get_ptr();
			for (size_t i = 0; i < length; i++)
			{
				type* c = ptr + i;
				*c = get_uppercase(*c);
			}
		}
	}

	void to_lowercase()
	{
		size_t length = get_length();
		if (!!length)
		{
			type* ptr = get_ptr();
			for (size_t i = 0; i < length; i++)
			{
				type* c = ptr + i;
				*c = get_lowercase(*c);
			}
		}
	}

	template <typename int_t>
	int_t to_int(unsigned int radix = 0) const	// Max radix is 36.  Radix of 0 defaults to dec but auto-detects oct and hex 
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
			if (!tmp.get_length())
				break;
			
			if (tmp.get_const_ptr()[0] == (type)'+')
				tmp.advance();
			else if (tmp.get_const_ptr()[0] == (type)'-')
			{
				neg = !neg;
				tmp.advance();
			}
			else
				break;
		}

		if (!gotRadix && !!tmp.get_length() && (tmp.get_const_ptr()[0] == (type)'0'))
		{
			radix = 8;
			tmp.advance();
			if (!!tmp.get_length() && ((tmp.get_const_ptr()[0] == (type)'x') || (tmp.get_const_ptr()[0] == (type)'X')))
			{
				radix = 16;
				tmp.advance();
			}
		}

		size_t length = tmp.get_length();
		type maxDecimalDigit = (radix < 10) ? (((type)radix - 1) + (type)'0') : (type)'9';
		if (radix > 10)
		{
			type maxAlphaDigitLower = ((type)radix - 11) + (type)'a';
			type maxAlphaDigitUpper = ((type)radix - 11) + (type)'A';
			for (size_t i = 0; i < length; i++)
			{
				type c = tmp.get_const_ptr()[i];
				if (c == (type)'.')	// stop at a deciminal place
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
				else //if (!is_white_space(c))	// unhandleable characters cause 0 to be returned
				{
					result = 0;
					break;
				}
			}
		}
		else
		{
			for (size_t i = 0; i < length; i++)
			{
				type c = tmp.get_const_ptr()[i];
				if (c == (type)'.')	// stop at a deciminal place
					break;
				if ((c >= (type)'0') && (c <= maxDecimalDigit))
				{
					result *= radix;
					result += c - (type)'0';
				}
				else //if (!is_white_space(c))	// unhandleable characters cause 0 to be returned
				{
					result = 0;
					break;
				}
			}
		}

		if (neg)
			result = negate_if_signed<int_t>::get(result);
		return result;
	}
};



template <typename type, typename type2>
class case_insensitive_comparator<string_t<type>, string_t<type2> >
{
public:
	static bool is_less_than(const string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.is_less_than(t2, is_case_insensitive);
	}

	static bool is_greater_than(const string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.is_greater_than(t2, is_case_insensitive);
	}

	static bool equals(const string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.equals(t2, is_case_insensitive);
	}

	static int compare(const string_t<type>& t1, const string_t<type2>& t2)
	{
		return t1.compare(t2, is_case_insensitive);
	}
};



// U+0009–U+000D (control characters, containing Tab, CR and LF)
// U+0020 SPACE
// U+0085 NEL (control character next line)
// U+00A0 NBSP (NO-BREAK SPACE)
// U+1680 OGHAM SPACE MARK
// U+180E MONGOLIAN VOWEL SEPARATOR
// U+2000–U+200A (different sorts of spaces)
// U+2028 LS (LINE SEPARATOR)
// U+2029 PS (PARAGRAPH SEPARATOR)
// U+202F NNBSP (NARROW NO-BREAK SPACE)
// U+205F MMSP (MEDIUM MATHEMATICAL SPACE)
// U+3000 IDEOGRAPHIC SPACE



template <>
inline bool string_t<char>::is_white_space(const char& c)
{
	return (((unsigned char)c == 0x20) || (((unsigned char)c >= 0x09) && ((unsigned char)c <= 0x0D))
		|| ((unsigned char)c == 0x85) || ((unsigned char)c == 0xA0));
}


template <>
inline bool string_t<wchar_t>::is_white_space(const wchar_t& c)
{
	return ((c == 0x0020) || ((c >= 0x0009) && (c <= 0x000D)) || (c == 0x0085) || (c == 0x00A0)
			|| (c == 0x1680) || (c == 0x180E) || ((c >= 0x2000) && (c <= 0x200A)) || (c == 0x2028)
			|| (c == 0x2029) || (c == 0x202F) || (c == 0x205F) || (c == 0x3000));
}


template <> inline char string_t<char>::get_uppercase(const char& c)				{ return toupper(c); }
template <> inline char string_t<char>::get_lowercase(const char& c)				{ return tolower(c); }
template <> inline wchar_t string_t<wchar_t>::get_uppercase(const wchar_t& c)		{ return towupper(c); }
template <> inline wchar_t string_t<wchar_t>::get_lowercase(const wchar_t& c)		{ return towlower(c); }



typedef string_t<char>		cstring;
typedef string_t<wchar_t>	string;

typedef string_t<signed char>		signed_cstring;
typedef string_t<unsigned char>		unsigned_cstring;


inline cstring string_to_cstring(const string& s)
{
	cstring s2;
	s2.resize(s.get_length());
	char* s2Ptr = s2.get_ptr();
	for (size_t i = 0; i < s.get_length(); i++)
		s2Ptr[i] = (char)s[i];
	return s2;
}

inline string cstring_to_string(const cstring& s)
{
	string s2;
	s2.resize(s.get_length());
	wchar_t* s2Ptr = s2.get_ptr();
	for (size_t i = 0; i < s.get_length(); i++)
		s2Ptr[i] = (wchar_t)s[i];
	return s2;
}


template <typename char_t>
inline string_t<char_t> boolean::to_string_t() const
{
	class string_helper
	{
	public:
		static string get_true() { return string::literal(L"true"); }
		static string get_false() { return string::literal(L"false"); }
	};

	class cstring_helper
	{
	public:
		static cstring get_true() { return cstring::literal("true"); }
		static cstring get_false() { return cstring::literal("false"); }
	};

	typedef std::conditional_t<std::is_same_v<char, char_t>, cstring_helper, string_helper> helper;

	return m_bool ? helper::get_true() : helper::get_false();
}

inline string_t<wchar_t> boolean::to_string() const	{ return to_string_t<wchar_t>(); }
inline string_t<char> boolean::to_cstring() const		{ return to_string_t<char>(); }

template <typename char_t>
inline string_t<char_t> boolean::to_string_t() const volatile	{ boolean cpy(*this); return cpy.template to_string_t<char_t>(); }

inline string_t<wchar_t> boolean::to_string() const volatile	{ boolean cpy(*this); return cpy.to_string(); }
inline string_t<char> boolean::to_cstring() const volatile		{ boolean cpy(*this); return cpy.to_cstring(); }






template <bool has_sign, size_t n_bits>
template <typename char_t>
inline string_t<char_t> fixed_integer_native<has_sign, n_bits>::to_string_t(unsigned int radix, size_t minDigits) const
{
	if (!m_int || (radix < 2))
	{
		if (minDigits <= 64)
		{
			// Avoiding an allocation is more efficient, but needs a fixed max # of digits
			static constexpr char_t zeros[] = {
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0',
				(char_t)'0', (char_t)'0', (char_t)'0', (char_t)'0' };
			return string_t<char_t>::contain(zeros, minDigits);
		}
		
		return string_t<char_t>(minDigits, (char_t)'0');
	}

	if (radix > 36)
		radix = 36;

	// Handle negative values using a 2-way table.
	static constexpr char digitsBuffer[] = "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const char* digits = digitsBuffer + 35;
	static constexpr size_t maxLength = (sizeof(int_t) * 8) + 2;	// Enough space for largest possible value, i.e. binary radix

	char tempBuffer[maxLength];

	size_t i = 0;
	int_t remaining = m_int;
	while (!!remaining)
	{
		int digitIndex = remaining % radix;
		remaining /= radix;
		tempBuffer[i++] = digits[digitIndex];
	}

	size_t strLength = i;
	if (strLength < minDigits)
		strLength = minDigits;
	bool isNeg = is_negative();
	if (isNeg)
		strLength++;
	string_t<char_t> str;
	str.resize(strLength);
	char_t* p = str.get_ptr();

	size_t i2 = 0;
	if (isNeg)
		p[i2++] = (char_t)'-';
	size_t i3 = i;
	while (i3 < minDigits)
	{
		p[i2++] = (char_t)'0';
		i3++;
	}

	for (; i > 0;)
		p[i2++] = (char_t)tempBuffer[--i];

	return str;
}

template <bool has_sign, size_t n_bits>
inline string fixed_integer_native<has_sign, n_bits>::to_string(int radix, size_t minDigits) const
{
	return to_string_t<wchar_t>(radix, minDigits);
}

template <bool has_sign, size_t n_bits>
inline cstring fixed_integer_native<has_sign, n_bits>::to_cstring(int radix, size_t minDigits) const
{
	return to_string_t<char>(radix, minDigits);
}

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <typename char_t>
inline string_t<char_t> fixed_integer_native_const<has_sign, bits, value>::to_string_t(unsigned int radix, size_t minDigits) const volatile
{
	non_const_t tmp(*this);
	return tmp.template to_string_t<char_t>(radix, minDigits);
}

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline string fixed_integer_native_const<has_sign, bits, value>::to_string(int radix, size_t minDigits) const volatile
{
	return to_string_t<wchar_t>(radix, minDigits);
}

template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
inline cstring fixed_integer_native_const<has_sign, bits, value>::to_cstring(int radix, size_t minDigits) const volatile
{
	return to_string_t<char>(radix, minDigits);
}


template <typename type>
template <typename char_t>
inline string_t<char_t> vector<type>::to_string_t(const string_t<char_t>& prefix, const string_t<char_t>& delimiter, const string_t<char_t>& postfix) const
{
	string str = prefix;
	size_t sz = get_length();
	if (!!sz)
	{
		size_t i = 0;
		for (;;)
		{
			str += cogs::to_string_t<char_t>((*this)[i]);
			i++;
			if (i == sz)
				break;
			str += delimiter;
		}
	}
	str += postfix;
	return str;
}

template <typename type>
inline string vector<type>::to_string(const string& prefix, const string& delimiter, const string& postfix) const
{
	return to_string_t<wchar_t>(prefix, delimiter, postfix);
}

template <typename type>
inline cstring vector<type>::to_cstring(const cstring& prefix, const cstring& delimiter, const cstring& postfix) const
{
	return to_string_t<char>(prefix, delimiter, postfix);
}


template <typename type>
template <typename char_t>
inline string_t<char_t> vector<type>::to_string_t() const
{
	class string_helper
	{
	public:
		static string get_prefix() { return string::literal(L"{ "); }
		static string get_delimiter() { return string::literal(L", "); }
		static string get_postfix() { return string::literal(L" }"); }
	};

	class cstring_helper
	{
	public:
		static cstring get_prefix() { return cstring::literal("{ "); }
		static cstring get_delimiter() { return cstring::literal(", "); }
		static cstring get_postfix() { return cstring::literal(" }"); }
	};

	typedef std::conditional_t<std::is_same_v<char, char_t>, cstring_helper, string_helper> helper;

	return to_string_t<char>(helper::get_prefix(), helper::get_delimiter(), helper::get_postfix());
}

template <typename type>
inline string vector<type>::to_string() const { return to_string_t<wchar_t>(); }

template <typename type>
inline cstring vector<type>::to_cstring() const { return to_string_t<char>(); }


template <typename type>
template <typename char_t>
inline string_t<char_t> vector<type>::to_string_t(const string_t<char_t>& prefix, const string_t<char_t>& delimiter, const string_t<char_t>& postfix) const volatile { vector<type> cpy(*this); return cpy.template to_string_t<char_t>(prefix, delimiter, postfix); }

template <typename type>
inline string vector<type>::to_string(const string& prefix, const string& delimiter, const string& postfix) const volatile { vector<type> cpy(*this); return cpy.to_string(prefix, delimiter, postfix); }

template <typename type>
inline cstring vector<type>::to_cstring(const cstring& prefix, const cstring& delimiter, const cstring& postfix) const volatile { vector<type> cpy(*this); return cpy.to_cstring(prefix, delimiter, postfix); }


template <typename type>
template <typename char_t>
inline string_t<char_t> vector<type>::to_string_t() const volatile { vector<type> cpy(*this); return cpy.template to_string_t<char_t>(); }

template <typename type>
inline string vector<type>::to_string() const volatile { vector<type> cpy(*this); return cpy.to_string(); }

template <typename type>
inline cstring vector<type>::to_cstring() const volatile { vector<type> cpy(*this); return cpy.to_cstring(); }



#pragma warning(pop)


}


#endif

