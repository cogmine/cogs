//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress, Placeholder

#ifndef COGS_HEADER_MATH_VEC
#define COGS_HEADER_MATH_VEC


#include "cogs/collections/vector.hpp"

	/*
namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


template <size_t n, typename type>
class vec;


template <size_t n, typename type1, typename type2>
class return_type_binary_operator_add<vec<n, type1>, vec<n, type2> >
{
public:
	typedef vec<n, typename return_type_binary_operator_add<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class return_type_binary_operator_subtract<vec<n, type1>, vec<n, type2> >
{
public:
	typedef vec<n, typename return_type_binary_operator_subtract<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class return_type_binary_operator_multiply<vec<n, type1>, vec<n, type2> >
{
public:
	typedef vec<n, typename return_type_binary_operator_multiply<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class return_type_binary_member_function_divide_whole<vec<n, type1>, vec<n, type2> >
{
public:
typedef vec<n, typename return_type_binary_member_function_divide_whole<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class return_type_binary_member_function_divide_whole<vec<n, type1>, vec<n, type2> >
{
public:
typedef vec<n, typename return_type_binary_member_function_divide_whole<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class return_type_binary_operator_modulo<vec<n, type1>, vec<n, type2> >
{
public:
	typedef vec<n, typename return_type_binary_operator_modulo<type1, type2>::type> type;
};

template <size_t n, typename type1, typename type2>
class compatible_type<vec<n, type1>, vec<n, type2> >
{
public:
	typedef vec<n, typename compatible_type<type1, type2>::type> type;
};



template <size_t n, typename type>
class vec
{
public:
	typedef vec<n, type> this_t;

	typedef array<n, type> array_t;
	typedef vector<array_t> vector_t;

private:
	vector_t m_contents;

public:
	vector_t& get() { return m_contents; }
	const vector_t& get() const { return m_contents; }

	/// @brief A vec iterator
	class iterator
	{
	private:
		template <size_t, typename>
		friend class vec;

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

		array_t* get() const { m_iterator.get(); }
		array_t& operator*() const { return *get(); }
		array_t* operator->() const { return get(); }

		size_t get_position() const { m_iterator.get_position(); }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }
	};

	/// @brief A vec const iterator
	class const_iterator
	{
	private:
		template <size_t, typename>
		friend class vec;

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

		const array_t* get() const { m_iterator.get(); }
		const array_t& operator*() const { return *get(); }
		const array_t* operator->() const { return get(); }

		size_t get_position() const { m_iterator.get_position(); }

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }
	};

	iterator get_first_iterator() { return m_contents.get_first_iterator(); }
	iterator get_last_iterator() { return m_contents.get_last_iterator(); }

	const_iterator get_first_const_iterator() const { return m_contents.get_first_const_iterator(); }
	const_iterator get_last_const_iterator() const { return m_contents.get_last_const_iterator(); }

	// contains specified buffer (allocates if ptr is NULL)
	static this_t contain(const array_t* ptr, size_t sz) { return this_t(sz, ptr); }

	vec() { }

	explicit vec(size_t n2) : m_contents(n2) { }

	vec(size_t n2, const array_t& src) : m_contents(n2, src) { }

	template <typename type2> vec(size_t n2, const type2& src) : m_contents(n2, src) { }
	template <typename type2> vec(const type2* src, size_t n2) : m_contents(src, n2) { }

	vec(const this_t& src) : m_contents(src.m_contents) { }
	vec(const this_t& src, size_t i) : m_contents(src.m_contents, i) { }
	vec(const this_t& src, size_t i, size_t n2) : m_contents(src.m_contents, i, n2) { }

	vec(const volatile this_t& src) : m_contents(src.m_contents) { }
	vec(const volatile this_t& src, size_t i) : m_contents(src.m_contents, i) { }
	vec(const volatile this_t& src, size_t i, size_t n2) : m_contents(src.m_contents, i, n2) { }

	template <typename type2> vec(const vec<n, type2>& src) : m_contents(src.m_contents) { }
	template <typename type2> vec(const vec<n, type2>& src, size_t i) : m_contents(src.m_contents, i) { }
	template <typename type2> vec(const vec<n, type2>& src, size_t i, size_t n2) : m_contents(src.m_contents, i, n2) { }
	template <typename type2> vec(const volatile vec<n, type2>& src) : m_contents(src.m_contents) { }
	template <typename type2> vec(const volatile vec<n, type2>& src, size_t i) : m_contents(src.m_contents, i) { }
	template <typename type2> vec(const volatile vec<n, type2>& src, size_t i, size_t n2) : m_contents(src.m_contents, i, n2) { }

	//template <typename type2> vec(const vector<array_t>& src) : m_contents(src) { }
	//template <typename type2> vec(const vector<array_t>& src, size_t i) : m_contents(src, i) { }
	//template <typename type2> vec(const vector<array_t>& src, size_t i, size_t n2) : m_contents(src, i, n2) { }
	//template <typename type2> vec(const volatile vector<array_t>& src) : m_contents(src) { }
	//template <typename type2> vec(const volatile vector<array_t>& src, size_t i) : m_contents(src, i) { }
	//template <typename type2> vec(const volatile vector<array_t>& src, size_t i, size_t n2) : m_contents(src, i, n2) { }

	bool is_unowned() const { return m_contents.is_unowned(); }
	bool is_unowned() const volatile { return m_contents.is_unowned(); }

	bool is_owned() const { return m_contents.is_owned(); }
	bool is_owned() const volatile { return m_contents.is_owned(); }

	bool is_shared() const { return m_contents.is_shared(); }
	bool is_shared() const volatile { return m_contents.is_shared(); }

	size_t get_length() const { return m_contents.get_length(); }
	size_t get_length() const volatile { return m_contents.get_length(); }

	size_t get_capacity() const { return m_contents.get_capacity(); }

	size_t get_reverse_capacity() const { return m_contents.get_reverse_capacity(); }

	bool is_empty() const { return m_contents.is_empty(); }
	bool is_empty() const volatile { return m_contents.is_empty(); }

	bool operator!() const { return !m_contents; }
	bool operator!() const volatile { return !m_contents; }

	const array_t* get_const_ptr() const { return m_contents.get_const_ptr(); }

	// caller error to pass index >= length
	const array_t& operator[](size_t i) const { return m_contents[i]; }

	const array_t& get_first_const() const { return m_contents.get_first_const(); }
	const array_t& get_last_const() const { return m_contents.get_last_const(); }


	const this_t& subrange(size_t i, size_t n2 = const_max_int<size_t>::value, unowned_t<this_t>& storage = unowned_t<this_t>().get_unowned()) const
	{
		size_t length = get_length();
		if (i <= length)
		{
			size_t adjustedLength = length - i;
			if (adjustedLength > n2)
				adjustedLength = n2;
			storage.m_contents.set(m_contents->m_desc, m_contents->m_ptr + i, adjustedLength);
		}
		return storage;
	}

	this_t subrange(size_t i, size_t n2 = const_max_int<size_t>::value) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(i, n2);
		return result;
	}

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	int compare(size_t n2, const array_t2& cmp) const { return m_contents.compare<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	int compare(size_t n2, const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template compare<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	int compare(const array_t2* cmp, size_t n2) const { return m_contents.compare<array_t2, comparator_t>(cmp, n2); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	int compare(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template compare<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = type, class comparator_t = default_comparator>
	int compare(const vec<n, type2>& cmp) const { return compare<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	int compare(const vec<n, type2>& cmp) const volatile { return compare<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	int compare(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return compare<type2, comparator_t>(tmp); }



	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool equals(size_t n2, const array_t2& cmp) const { return m_contents.equals<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool equals(size_t n2, const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template equals<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool equals(const array_t2* cmp, size_t n2) const { return m_contents.equals<array_t2, comparator_t>(cmp, n2); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool equals(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template equals<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool equals(const vec<n, type2>& cmp) const { return equals<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool equals(const vec<n, type2>& cmp) const volatile { return equals<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool equals(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return equals<type2, comparator_t>(tmp); }


	template <typename type2>
	bool operator==(const vec<n, type2>& cmp) const { return equals(cmp); }

	template <typename type2>
	bool operator==(const vec<n, type2>& cmp) const volatile { return equals(cmp); }

	template <typename type2>
	bool operator==(const volatile vec<n, type2>& cmp) const { return equals(cmp); }

	template <typename type2>
	bool operator!=(const vec<n, type2>& cmp) const { return !equals(cmp); }

	template <typename type2>
	bool operator!=(const vec<n, type2>& cmp) const volatile { return !equals(cmp); }

	template <typename type2>
	bool operator!=(const volatile vec<n, type2>& cmp) const { return !equals(cmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool starts_with(size_t n2, const array_t2& cmp) const { return m_contents.starts_with<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool starts_with(size_t n2, const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template starts_with<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool starts_with(const array_t2* cmp, size_t n2) const { return m_contents.starts_with<array_t2, comparator_t>(cmp, n2); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool starts_with(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template starts_with<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool starts_with(const vec<n, type2>& cmp) const { return starts_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool starts_with(const vec<n, type2>& cmp) const volatile { return starts_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool starts_with(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return starts_with<type2, comparator_t>(tmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool ends_with(size_t n2, const array_t2& cmp) const { return m_contents.ends_with<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool ends_with(size_t n2, const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template ends_with<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool ends_with(const array_t2* cmp, size_t n2) const { return m_contents.ends_with<array_t2, comparator_t>(cmp, n2); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool ends_with(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template ends_with<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool ends_with(const vec<n, type2>& cmp) const { return ends_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool ends_with(const vec<n, type2>& cmp) const volatile { return ends_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool ends_with(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return ends_with<type2, comparator_t>(tmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool is_less_than(const array_t2* cmp, size_t n2) const { return m_contents.is_less_than<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool is_less_than(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template is_less_than<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_less_than(const vec<n, type2>& cmp) const { return is_less_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_less_than(const vec<n, type2>& cmp) const volatile { return is_less_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_less_than(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return is_less_than<type2, comparator_t>(tmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool is_greater_than(const array_t2* cmp, size_t n2) const { return m_contents.is_greater_than<array_t2, comparator_t>(n2, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool is_greater_than(const array_t2* cmp, size_t n2) const volatile { this_t tmp(*this); return tmp.template is_greater_than<array_t2, comparator_t>(cmp, n2); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_greater_than(const vec<n, type2>& cmp) const { return is_greater_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_greater_than(const vec<n, type2>& cmp) const volatile { return is_greater_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = array_t, class comparator_t = default_comparator>
	bool is_greater_than(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp(cmp); return is_greater_than<type2, comparator_t>(tmp); }



	template <typename type2>
	bool operator<(const vec<n, type2>& cmp) const { return is_less_than(cmp); }

	template <typename type2>
	bool operator<(const vec<n, type2>& cmp) const volatile { return is_less_than(cmp); }

	template <typename type2>
	bool operator<(const volatile vec<n, type2>& cmp) const { return is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const vec<n, type2>& cmp) const { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const vec<n, type2>& cmp) const volatile { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const volatile vec<n, type2>& cmp) const { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>(const vec<n, type2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2>
	bool operator>(const vec<n, type2>& cmp) const volatile { return is_greater_than(cmp); }

	template <typename type2>
	bool operator>(const volatile vec<n, type2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const vec<n, type2>& cmp) const { return !is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const vec<n, type2>& cmp) const volatile { return !is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const volatile vec<n, type2>& cmp) const { return !is_greater_than(cmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of(const array_t2& cmp) const { return m_contents.template index_of<array_t2, comparator_t>(cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of(const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template index_of<array_t2, comparator_t>(cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of(size_t i, const array_t2& cmp) const { return m_contents.template index_of<array_t2, comparator_t>(i, cmp); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of(size_t i, const array_t2& cmp) const volatile { this_t tmp(*this); return tmp.template index_of<array_t2, comparator_t>(i, cmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_any(const array_t2* cmp, size_t cmpLength) const { return m_contents.template index_of_any<array_t2, comparator_t>(cmp, cmpLength); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_any(const array_t2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_any<array_t2, comparator_t>(cmp, cmpLength); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_any(size_t i, const array_t2* cmp, size_t cmpLength) const { return m_contents.template index_of_any<array_t2, comparator_t>(i, cmp, cmpLength); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_any(size_t i, const array_t2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_any<array_t2, comparator_t>(i, cmp, cmpLength); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_segment(const array_t2* cmp, size_t cmpLength) const { return m_contents.template index_of_segment<array_t2, comparator_t>(cmp, cmpLength); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_segment(const array_t2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_segment<array_t2, comparator_t>(cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(const vec<n, type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(const vec<n, type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp; return index_of_segment<type2, comparator_t>(tmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_segment(size_t i, const array_t2* cmp, size_t cmpLength) const { return m_contents.template index_of_segment<array_t2, comparator_t>(i, cmp, cmpLength); }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	size_t index_of_segment(size_t i, const array_t2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_segment<array_t2, comparator_t>(i, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(size_t i, const vec<n, type2>& cmp) const { return index_of_segment<type2, comparator_t>(i, cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(size_t i, const vec<n, type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(i, cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator>
	size_t index_of_segment(size_t i, const volatile vec<n, type2>& cmp) const { vec<n, type2> tmp; return index_of_segment<type2, comparator_t>(i, tmp); }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains(const array_t2& cmp) const { return index_of<array_t2, comparator_t>(cmp) != const_max_int<size_t>::value; }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains(const array_t2& cmp) const volatile { return index_of<array_t2, comparator_t>(cmp) != const_max_int<size_t>::value; }



	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains_any(const array_t2* cmp, size_t cmpLength) const { return index_of_any<array_t2, comparator_t>(cmp, cmpLength) != const_max_int<size_t>::value; }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains_any(const array_t2* cmp, size_t cmpLength) const volatile { return index_of_any<array_t2, comparator_t>(cmp, cmpLength) != const_max_int<size_t>::value; }


	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains_segment(const array_t2* cmp, size_t cmpLength) const { return index_of_segment<array_t2, comparator_t>(cmp, cmpLength) != const_max_int<size_t>::value; }

	template <typename array_t2 = array_t, class comparator_t = default_comparator>
	bool contains_segment(const array_t2* cmp, size_t cmpLength) const volatile { return index_of_segment<array_t2, comparator_t>(cmp, cmpLength) != const_max_int<size_t>::value; }

	template <typename type2 = type, class comparator_t = default_comparator>
	bool contains_segment(const vec<n, type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp) != const_max_int<size_t>::value; }

	template <typename type2 = type, class comparator_t = default_comparator>
	bool contains_segment(const vec<n, type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(cmp) != const_max_int<size_t>::value; }

	template <typename type2 = type, class comparator_t = default_comparator>
	bool contains_segment(const volatile vec<n, type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp) != const_max_int<size_t>::value; }


	array_t* get_ptr() { return m_contents.get_ptr(); }
	array_t& get_first() { return m_contents.get_first(); }
	array_t& get_last() { return m_contents.get_last(); }

	template <typename array_t2 = array_t> void set_index(size_t i, const array_t2& src) { m_contents.set_index(i, src); }

	void reverse() { reverse(); }

	void set_to_subrange(size_t i) { m_contents.set_to_subrange(i); }
	void set_to_subrange(size_t i) volatile { m_contents.set_to_subrange(i); }

	void set_to_subrange(size_t i, size_t n2) { m_contents.set_to_subrange(i, n2); }
	void set_to_subrange(size_t i, size_t n2) volatile { m_contents.set_to_subrange(i, n2); }

	void clear() { m_contents.clear(); }
	void clear() volatile { m_contents.clear(); }

	// clears reserved space as well
	void reset() { m_contents.reset(); }

	// clears reserved space as well
	void reset() volatile { m_contents.reset(); }

	void reserve(size_t n2) { m_contents.reserve(n2); }

	void assign(size_t n2, const array_t& src) { m_contents.assign(n2, src); }
	void assign(size_t n2, const array_t& src) volatile { m_contents.assign(n2, src); }

	template <typename array_t2 = array_t> void assign(size_t n2, const array_t2& src) { m_contents.assign(n2, src); }
	template <typename array_t2 = array_t> void assign(size_t n2, const array_t2& src) volatile { m_contents.assign(n2, src); }
	template <typename array_t2 = array_t> void assign(const array_t2* src, size_t n2) { m_contents.assign(src, n2); }
	template <typename array_t2 = array_t> void assign(const array_t2* src, size_t n2) volatile { m_contents.assign(src, n2); }

	void assign(const vec<n, type>& src) { m_contents.assign(src.m_contents); }

	template <typename type2 = type> void assign(const vec<n, type2>& src) { m_contents.assign(src.m_contents); }
	template <typename type2 = type> void assign(const vec<n, type2>& src) volatile { m_contents.assign(src.m_contents); }
	template <typename type2 = type> void assign(const volatile vec<n, type2>& src) { m_contents.assign(src.m_contents); }

	this_t& operator=(const this_t& src) { m_contents.assign(src.m_contents); return *this; }
	template <typename type2 = type> this_t& operator=(const vec<n, type2>& src) { m_contents.assign(src.m_contents); return *this; }
	this_t operator=(const this_t& src) volatile { m_contents.assign(src.m_contents); return src; }
	template <typename type2 = type> this_t operator=(const vec<n, type2>& src) volatile { this_t tmp(src); m_contents.assign(src.m_contents); return tmp; }
	template <typename type2 = type> this_t& operator=(const volatile vec<n, type2>& src) { m_contents.assign(src.m_contents); return *this; }

	void append(size_t n2 = 1) { m_contents.append(n2); }
	void append(size_t n2, const array_t& src) { m_contents.append(n2, src); }

	template <typename array_t2 = array_t> void append(size_t n2, const array_t2& src) { m_contents.append(n2, src); }
	template <typename array_t2 = array_t> void append(const array_t2* src, size_t n2) { m_contents.append(src, n2); }
	template <typename type2 = type> void append(const vec<n, type2>& src) { m_contents.append(src.m_contents); }
	template <typename type2 = type> void append(const volatile vec<n, type2>& src) { m_contents.append(src.m_contents); }

	//this_t& operator+=(const array_t& src) { m_contents.append(src); return *this; }
	//template <typename type2 = type> this_t& operator+=(const vec<n, type2>& src) { m_contents.append(src.m_contents); return *this; }
	//template <typename type2 = type> this_t& operator+=(const volatile vec<n, type2>& src) { m_contents.append(src.m_contents); return *this; }

	void prepend(size_t n2 = 1) { m_contents.prepend(n2); }
	void prepend(size_t n2, const array_t& src) { m_contents.prepend(n2, src); }

	template <typename array_t2 = array_t> void prepend(size_t n2, const array_t2& src) { m_contents.prepend(n2, src); }
	template <typename array_t2 = array_t> void prepend(const array_t2* src, size_t n2) { m_contents.prepend(src, n2); }
	template <typename type2 = type> void prepend(const vec<n, type2>& src) { m_contents.prepend(src.m_contents); }
	template <typename type2 = type> void prepend(const volatile vec<n, type2>& src) { m_contents.prepend(src.m_contents); }

	void resize(size_t n2) { m_contents.resize(n2); }

	template <typename array_t2 = array_t>
	void resize(size_t n2, const array_t2& src) { m_contents.resize(n2, src); }

	void erase(size_t i) { m_contents.erase(i); }
	void erase(size_t i, size_t n2) { m_contents.erase(i, n2); }

	void advance(size_t n2 = 1) { m_contents.advance(n2); }

	void truncate_to(size_t n2) { m_contents.truncate_to(n2); }

	void truncate(size_t n2) { m_contents.truncate(n2); }

	void truncate_to_right(size_t n2) { m_contents.truncate_to_right(n2); }

	void insert(size_t i, size_t n2) { m_contents.insert(i, n2); }
	void insert(size_t i, size_t n2, const array_t& src) { m_contents.insert(i, n2, src); }

	template <typename array_t2 = array_t> void insert(size_t i, size_t n2, const array_t2& src) { m_contents.insert(i, n2, src); }
	template <typename array_t2 = array_t> void insert(size_t i, const array_t2* src, size_t n2) { m_contents.insert(i, src, n2); }
	template <typename type2 = type> void insert(size_t i, const vec<n, type2>& src) { m_contents.insert(i, src.m_contents); }
	template <typename type2 = type> void insert(size_t i, const volatile vec<n, type2>& src) { m_contents.insert(i, src.m_contents); }

	template <typename array_t2 = array_t> void replace(size_t i, size_t replaceLength, const array_t2& src) { m_contents.replace(i, replaceLength, src); }
	template <typename array_t2 = array_t> void replace(size_t i, const array_t2* src, size_t replaceLength) { m_contents.replace(i, src, replaceLength); }
	template <typename type2 = type> void replace(size_t i, const vec<n, type2>& src) { m_contents.replace(i, src.m_contents); }
	template <typename type2 = type> void replace(size_t i, const volatile vec<n, type2>& src) { m_contents.replace(i, src.m_contents); }

	template <typename array_t2 = array_t>
	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, const array_t2& src)
	{ m_contents.insert_replace(i, replaceLength, insertLength, src); }

	template <typename array_t2 = array_t>
	void insert_replace(size_t i, size_t replaceLength, const array_t2* src, size_t insertLength)
	{ m_contents.insert_replace(i, replaceLength, src, insertLength); }

	template <typename type2 = type>
	void insert_replace(size_t i, size_t replaceLength, const vec<n, type2>& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }

	template <typename type2 = type>
	void insert_replace(size_t i, size_t replaceLength, const volatile vec<n, type2>& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }

	template <typename type2 = type>
	void insert_replace(size_t i, const vec<n, type2>& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }

	template <typename type2 = type>
	void insert_replace(size_t i, const volatile vec<n, type2>& src)
	{ m_contents.insert_replace(i, replaceLength, src.m_contents); }


	void exchange(this_t& wth) { m_contents.exchange(wth.m_contents); }
	void exchange(this_t& wth) volatile { m_contents.exchange(wth.m_contents); }
	void exchange(volatile this_t& wth) { wth.exchange(*this); }


	this_t split_off_before(size_t i) { return m_contents.split_off_before(i); }
	this_t split_off_before(size_t i) volatile { return m_contents.split_off_before(i); }

	this_t split_off_after(size_t i) { return m_contents.split_off_after(i); }
	this_t split_off_after(size_t i) volatile { return m_contents.split_off_after(i); }

	this_t split_off_front(size_t n2) { return split_off_before(n2); }
	this_t split_off_front(size_t n2) volatile { return split_off_before(n2); }

	this_t split_off_back(size_t n2) { return m_contents.split_off_back(n2); }
	this_t split_off_back(size_t n2) volatile { return m_contents.split_off_back(n2); }


	vector<this_t> split_on(const array_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return array_t::template split_on_any_inner<this_t>(*this, &splitOn, 1, opt);
	}

	vector<this_t> split_on(const array_t& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return array_t::template split_on_any_inner<this_t>(tmp, &splitOn, 1, opt);
	}

	vector<this_t> split_on_any(const array_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return array_t::template split_on_any_inner<this_t>(*this, splitOn, n, opt);
	}

	vector<this_t> split_on_any(const array_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return array_t::template split_on_any_inner<this_t>(tmp, splitOn, n, opt);
	}

	vector<this_t> split_on_any(const vector<array_t>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return array_t::template split_on_any_inner<this_t>(*this, splitOn, opt);
	}

	vector<this_t> split_on_any(const volatile vector<array_t>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		vector<array_t> tmp(splitOn);
		return array_t::template split_on_any_inner<this_t>(*this, tmp, opt);
	}

	vector<this_t> split_on_any(const vector<array_t>& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return array_t::template split_on_any_inner<this_t>(tmp, splitOn, opt);
	}

	vector<this_t> split_on_segment(const array_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return array_t::template split_on_segment_inner<this_t>(*this, splitOn, n, opt);
	}

	vector<this_t> split_on_segment(const array_t* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return array_t::template split_on_segment_inner<this_t>(tmp, splitOn, n, opt);
	}

	vector<this_t> split_on_segment(const this_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return array_t::template split_on_segment_inner<this_t>(*this, splitOn.m_contents, opt);
	}

	vector<this_t> split_on_segment(const volatile this_t& splitOn, split_options opt = split_includes_empty_segments) const
	{
		this_t tmp(splitOn.m_contents);
		return array_t::template split_on_segment_inner<this_t>(*this, tmp, opt);
	}

	vector<this_t> split_on_segment(const this_t& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return array_t::template split_on_segment_inner<this_t>(tmp, splitOn.m_contents, opt);
	}


	template <typename type2 = type>
	void add(const array<n, type2>* src, size_t n2)
	{
		size_t srcLength = n2;
		size_t dstLength = m_contents->get_length();
		if (srcLength > dstLength)
		{
			m_contents->resize(srcLength);
			array_t* dstPtr = m_contents->get_ptr();
			size_t i = 0;
			for (; i < dstLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] += src[i][j];
			for (; i < srcLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src[i][j];
		}
		else
		{
			array_t* dstPtr = m_contents->get_ptr();
			size_t i = 0;
			for (; i < srcLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] += src[i][j];
		}
	}

	template <typename type2 = type> this_t& operator+=(const vec<n, type2>& src)			{ add(src.get_const_ptr(), src.get_length()); return *this; }
	template <typename type2 = type> this_t& operator+=(const volatile vec<n, type2>& src)	{ vec<n, type2> tmp(src); return *this += tmp; }
	
	template <typename type2 = type>
	typename return_type_binary_operator_add<vec<n, type>, vec<n, type2> >::type operator+(const vec<n, type2>& src) const
	{
		typename return_type_binary_operator_add<vec<n, type>, vec<n, type2> >::type result;
		size_t src1Length = m_contents->get_length();
		size_t src2Length = src.get_length();
		const array_t* src1Ptr = m_contents->get_const_ptr();
		const array_t* src2Ptr = src.get_const_ptr();
		if (src1Length <= src2Length)
		{
			result.resize(src2Length);
			array_t* dstPtr = result.get_ptr();
			size_t i = 0;
			for (; i < src1Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j] + src2Ptr[i][j];
			for (; i < src2Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src2Ptr[i][j];
		}
		else
		{
			result.resize(src1Length);
			array_t* dstPtr = result.get_ptr();
			size_t i = 0;
			for (; i < src2Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j] + src2Ptr[i][j];
			for (; i < src1Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j];
		}
		return result;
	}	
	
	template <typename type2 = type> typename return_type_binary_operator_add<vec<n, type>, vec<n, type2> >::type operator+(const volatile vec<n, type2>& src) const { vec<n, type2> tmp(src); return operator+(tmp); }
	template <typename type2 = type> typename return_type_binary_operator_add<vec<n, type>, vec<n, type2> >::type operator+(const vec<n, type2>& src) const volatile { vec<n, type2> tmp(*this); return tmp + src; }

	template <typename array_t2 = array_t>
	void subtract(const array_t2* src, size_t n2)
	{
		size_t srcLength = n2;
		size_t dstLength = m_contents->get_length();
		if (srcLength > dstLength)
		{
			m_contents->resize(srcLength);
			array_t* dstPtr = m_contents->get_ptr();
			size_t i = 0;
			for (; i < dstLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] -= src[i][j];
			for (; i < srcLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = -(src[i][j]);
		}
		else
		{
			array_t* dstPtr = m_contents->get_ptr();
			size_t i = 0;
			for (; i < srcLength; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] -= src[i][j];
		}
	}

	template <typename type2 = type> this_t& operator-=(const vec<n, type2>& src) { subtract(src.get_const_ptr(), src.get_length()); return *this; }
	template <typename type2 = type> this_t& operator-=(const volatile vec<n, type2>& src) { vec<n, type2> tmp(src); return *this -= tmp; }

	template <typename type2 = type>
	typename return_type_binary_operator_subtract<vec<n, type>, vec<n, type2> >::type operator+(const vec<n, type2>& src) const
	{
		typename return_type_binary_operator_subtract<vec<n, type>, vec<n, type2> >::type result;
		size_t src1Length = m_contents->get_length();
		size_t src2Length = src.get_length();
		const array_t* src1Ptr = m_contents->get_const_ptr();
		const array_t* src2Ptr = src.get_const_ptr();
		if (src1Length <= src2Length)
		{
			result.resize(src2Length);
			array_t* dstPtr = result.get_ptr();
			size_t i = 0;
			for (; i < src1Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j] - src2Ptr[i][j];
			for (; i < src2Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = -(src2Ptr[i][j]);
		}
		else
		{
			result.resize(src1Length);
			array_t* dstPtr = result.get_ptr();
			size_t i = 0;
			for (; i < src2Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j] + src2Ptr[i][j];
			for (; i < src1Length; i++)
				for (size_t j = 0; j < n; j++)
					dstPtr[i][j] = src1Ptr[i][j];
		}
		return result;
	}

	template <typename type2 = type> typename return_type_binary_operator_subtract<vec<n, type>, vec<n, type2> >::type operator-(const volatile vec<n, type2>& src) const { vec<n, type2> tmp(src); return operator-(tmp); }
	template <typename type2 = type> typename return_type_binary_operator_subtract<vec<n, type>, vec<n, type2> >::type operator-(const vec<n, type2>& src) const volatile { vec<n, type2> tmp(*this); return tmp - src; }

	//-----------


};



#pragma warning(pop)


typedef vec<2, bool> bvec2;
typedef vec<3, bool> bvec3;
typedef vec<4, bool> bvec4;

typedef vec<2, int32_t> ivec2;
typedef vec<3, int32_t> ivec3;
typedef vec<4, int32_t> ivec4;

typedef vec<2, uint32_t> uvec2;
typedef vec<3, uint32_t> uvec3;
typedef vec<4, uint32_t> uvec4;

typedef vec<2, float> vec2;
typedef vec<3, float> vec3;
typedef vec<4, float> vec4;

typedef vec<2, double> dvec2;
typedef vec<3, double> dvec3;
typedef vec<4, double> dvec4;




}
*/



#endif
