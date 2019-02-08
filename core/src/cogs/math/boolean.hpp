//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_BOOLEAN
#define COGS_BOOLEAN

#include <type_traits>

#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/int_types.hpp"
#include "cogs/operators.hpp"


namespace cogs {

/// @defgroup Math Mathematical
/// @{
/// @brief Mathematical
/// @}

#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


	// forward declated.  Defined in string.hpp
template <typename type>
class string_t;


/// @ingroup Math
/// @brief A boolean class.  Adds atomic operations.
class boolean
{
private:
	bool m_bool;

public:
	boolean()
		: m_bool(false)
	{ }

	boolean(bool b)
		: m_bool(b)
	{ }

	boolean(const boolean& b)
		: m_bool(b.m_bool)
	{ }

	boolean(const volatile boolean& b)
		: m_bool(b.get())
	{ }

	bool get() const				{ return m_bool; }
	bool get() const volatile		{ bool b; atomic::load(m_bool, b); return b; }

	void set(const          boolean& b)				{ m_bool = b.get(); }
	void set(const volatile boolean& b)				{ m_bool = b.get(); }
	void set(const          boolean& b) volatile	{ atomic::store(m_bool, b.get()); }
	void set(const volatile boolean& b) volatile	{ atomic::store(m_bool, b.get()); }
	void set(bool b)								{ m_bool = b; }
	void set(bool b) volatile						{ atomic::store(m_bool, b); }

	operator bool() const			{ return get(); }
	operator bool() const volatile	{ return get(); }

	bool operator!() const			{ return !get(); }
	bool operator!() const volatile	{ return !get(); }

	boolean& operator=(const          boolean& b)					{ set(b); return *this; }
	boolean& operator=(const volatile boolean& b)					{ set(b); return *this; }
	volatile boolean& operator=(const          boolean& b) volatile	{ set(b); return *this; }
	volatile boolean& operator=(const volatile boolean& b) volatile	{ set(b); return *this; }
	boolean& operator=(bool b)										{ set(b); return *this; }
	volatile boolean& operator=(bool b) volatile					{ set(b); return *this; }

	bool operator==(const          boolean& b) const			{ return get() == b.get(); }
	bool operator==(const volatile boolean& b) const			{ return get() == b.get(); }
	bool operator==(const          boolean& b) const volatile	{ return get() == b.get(); }
	bool operator==(const volatile boolean& b) const volatile	{ return get() == b.get(); }
	bool operator==(bool b) const								{ return get() == b; }
	bool operator==(bool b) const volatile						{ return get() == b; }

	bool operator!=(const          boolean& b) const			{ return !operator==(b); }
	bool operator!=(const volatile boolean& b) const			{ return !operator==(b); }
	bool operator!=(const          boolean& b) const volatile	{ return !operator==(b); }
	bool operator!=(const volatile boolean& b) const volatile	{ return !operator==(b); }
	bool operator!=(bool b) const								{ return !operator==(b); }
	bool operator!=(bool b) const volatile						{ return !operator==(b); }


	// overloading would break! short-circuiting!
//	bool operator&&(const          boolean& b)			{ return get() && b.get(); }
//	bool operator&&(const volatile boolean& b)			{ return get() && b.get(); }
//	bool operator&&(const          boolean& b) volatile	{ return get() && b.get(); }
//	bool operator&&(const volatile boolean& b) volatile	{ return get() && b.get(); }
//	bool operator&&(bool b)				{ return get() && b; }
//	bool operator&&(bool b) volatile	{ return get() && b; }
//
//	bool operator||(const          boolean& b)			{ return get() || b.get(); }
//	bool operator||(const volatile boolean& b)			{ return get() || b.get(); }
//	bool operator||(const          boolean& b) volatile	{ return get() || b.get(); }
//	bool operator||(const volatile boolean& b) volatile	{ return get() || b.get(); }
//	bool operator||(bool b)								{ return get() || b; }
//	bool operator||(bool b) volatile					{ return get() || b; }

	bool operator&(const          boolean& b)			{ return get() & b.get(); }
	bool operator&(const volatile boolean& b)			{ return get() & b.get(); }
	bool operator&(const          boolean& b) volatile	{ return get() & b.get(); }
	bool operator&(const volatile boolean& b) volatile	{ return get() & b.get(); }
	bool operator&(bool b)								{ return get() & b; }
	bool operator&(bool b) volatile						{ return get() & b; }

	bool operator|(const          boolean& b)			{ return get() | b.get(); }
	bool operator|(const volatile boolean& b)			{ return get() | b.get(); }
	bool operator|(const          boolean& b) volatile	{ return get() | b.get(); }
	bool operator|(const volatile boolean& b) volatile	{ return get() | b.get(); }
	bool operator|(bool b)								{ return get() | b; }
	bool operator|(bool b) volatile						{ return get() | b; }

	bool operator^(const          boolean& b)			{ return get() ^ b.get(); }
	bool operator^(const volatile boolean& b)			{ return get() ^ b.get(); }
	bool operator^(const          boolean& b) volatile	{ return get() ^ b.get(); }
	bool operator^(const volatile boolean& b) volatile	{ return get() ^ b.get(); }
	bool operator^(bool b)								{ return get() ^ b; }
	bool operator^(bool b) volatile						{ return get() ^ b; }


	boolean& operator&=(const          boolean& b)						{ cogs::assign_bit_and(m_bool, b.get()); return *this; }
	boolean& operator&=(const volatile boolean& b)						{ cogs::assign_bit_and(m_bool, b.get()); return *this; }
	volatile boolean& operator&=(const          boolean& b) volatile	{ cogs::assign_bit_and(m_bool, b.get()); return *this; }
	volatile boolean& operator&=(const volatile boolean& b) volatile	{ cogs::assign_bit_and(m_bool, b.get()); return *this; }
	boolean& operator&=(bool b)											{ cogs::assign_bit_and(m_bool, b); return *this; }
	volatile boolean& operator&=(bool b) volatile						{ cogs::assign_bit_and(m_bool, b); return *this; }

	const boolean& pre_bit_and(const          boolean& b)				{ return cogs::pre_assign_bit_and(m_bool, b.get()); }
	const boolean& pre_bit_and(const volatile boolean& b)				{ return cogs::pre_assign_bit_and(m_bool, b.get()); }
	boolean pre_bit_and(const          boolean& b) volatile				{ return cogs::pre_assign_bit_and(m_bool, b.get()); }
	boolean pre_bit_and(const volatile boolean& b) volatile				{ return cogs::pre_assign_bit_and(m_bool, b.get()); }
	const boolean& pre_bit_and(bool b)									{ return cogs::pre_assign_bit_and(m_bool, b); }
	boolean pre_bit_and(bool b) volatile								{ return cogs::pre_assign_bit_and(m_bool, b); }

	boolean post_bit_and(const          boolean& b)						{ return cogs::post_assign_bit_and(m_bool, b.get()); }
	boolean post_bit_and(const volatile boolean& b)						{ return cogs::post_assign_bit_and(m_bool, b.get()); }
	boolean post_bit_and(const          boolean& b) volatile			{ return cogs::post_assign_bit_and(m_bool, b.get()); }
	boolean post_bit_and(const volatile boolean& b) volatile			{ return cogs::post_assign_bit_and(m_bool, b.get()); }
	boolean post_bit_and(bool b)										{ return cogs::post_assign_bit_and(m_bool, b); }
	boolean post_bit_and(bool b) volatile								{ return cogs::post_assign_bit_and(m_bool, b); }


	boolean& operator|=(const          boolean& b)						{ cogs::assign_bit_or(m_bool, b.get()); return *this; }
	boolean& operator|=(const volatile boolean& b)						{ cogs::assign_bit_or(m_bool, b.get()); return *this; }
	volatile boolean& operator|=(const          boolean& b) volatile	{ cogs::assign_bit_or(m_bool, b.get()); return *this; }
	volatile boolean& operator|=(const volatile boolean& b) volatile	{ cogs::assign_bit_or(m_bool, b.get()); return *this; }
	boolean& operator|=(bool b)											{ cogs::assign_bit_or(m_bool, b); return *this; }
	volatile boolean& operator|=(bool b) volatile						{ cogs::assign_bit_or(m_bool, b); return *this; }

	const boolean& pre_bit_or(const          boolean& b)				{ return cogs::pre_assign_bit_or(m_bool, b.get()); }
	const boolean& pre_bit_or(const volatile boolean& b)				{ return cogs::pre_assign_bit_or(m_bool, b.get()); }
	boolean pre_bit_or(const          boolean& b) volatile				{ return cogs::pre_assign_bit_or(m_bool, b.get()); }
	boolean pre_bit_or(const volatile boolean& b) volatile				{ return cogs::pre_assign_bit_or(m_bool, b.get()); }
	const boolean& pre_bit_or(bool b)									{ return cogs::pre_assign_bit_or(m_bool, b); }
	boolean pre_bit_or(bool b) volatile									{ return cogs::pre_assign_bit_or(m_bool, b); }

	boolean post_bit_or(const          boolean& b)						{ return cogs::post_assign_bit_or(m_bool, b.get()); }
	boolean post_bit_or(const volatile boolean& b)						{ return cogs::post_assign_bit_or(m_bool, b.get()); }
	boolean post_bit_or(const          boolean& b) volatile				{ return cogs::post_assign_bit_or(m_bool, b.get()); }
	boolean post_bit_or(const volatile boolean& b) volatile				{ return cogs::post_assign_bit_or(m_bool, b.get()); }
	boolean post_bit_or(bool b)											{ return cogs::post_assign_bit_or(m_bool, b); }
	boolean post_bit_or(bool b) volatile								{ return cogs::post_assign_bit_or(m_bool, b); }



	boolean& operator^=(const          boolean& b)						{ cogs::assign_bit_xor(m_bool, b.get()); return *this; }
	boolean& operator^=(const volatile boolean& b)						{ cogs::assign_bit_xor(m_bool, b.get()); return *this; }
	volatile boolean& operator^=(const          boolean& b) volatile	{ cogs::assign_bit_xor(m_bool, b.get()); return *this; }
	volatile boolean& operator^=(const volatile boolean& b) volatile	{ cogs::assign_bit_xor(m_bool, b.get()); return *this; }
	boolean& operator^=(bool b)											{ cogs::assign_bit_xor(m_bool, b); return *this; }
	volatile boolean& operator^=(bool b) volatile						{ cogs::assign_bit_xor(m_bool, b); return *this; }

	
	const boolean& pre_bit_xor(const          boolean& b)				{ return cogs::pre_assign_bit_xor(m_bool, b.get()); }
	const boolean& pre_bit_xor(const volatile boolean& b)				{ return cogs::pre_assign_bit_xor(m_bool, b.get()); }
	boolean pre_bit_xor(const          boolean& b) volatile				{ return cogs::pre_assign_bit_xor(m_bool, b.get()); }
	boolean pre_bit_xor(const volatile boolean& b) volatile				{ return cogs::pre_assign_bit_xor(m_bool, b.get()); }
	const boolean& pre_bit_xor(bool b)									{ return cogs::pre_assign_bit_xor(m_bool, b); }
	boolean pre_bit_xor(bool b) volatile								{ return cogs::pre_assign_bit_xor(m_bool, b); }

	boolean post_bit_xor(const          boolean& b)						{ return cogs::post_assign_bit_xor(m_bool, b.get()); }
	boolean post_bit_xor(const volatile boolean& b)						{ return cogs::post_assign_bit_xor(m_bool, b.get()); }
	boolean post_bit_xor(const          boolean& b) volatile			{ return cogs::post_assign_bit_xor(m_bool, b.get()); }
	boolean post_bit_xor(const volatile boolean& b) volatile			{ return cogs::post_assign_bit_xor(m_bool, b.get()); }
	boolean post_bit_xor(bool b)										{ return cogs::post_assign_bit_xor(m_bool, b); }
	boolean post_bit_xor(bool b) volatile								{ return cogs::post_assign_bit_xor(m_bool, b); }


	void swap(boolean& b) { cogs::swap(m_bool, b.m_bool); }
	void swap(boolean& b) volatile { cogs::swap(m_bool, b.m_bool); }
	void swap(volatile boolean& b) { cogs::swap(m_bool, b.m_bool); }

	template <typename T2>
	void swap(T2& wth) { cogs::swap(m_bool, wth); }

	template <typename T2>
	void swap(T2& wth) volatile { cogs::swap(m_bool, wth); }


	void exchange(const boolean& src, boolean& rtn) { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const boolean& src, boolean& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const volatile boolean& src, boolean& rtn) { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const volatile boolean& src, boolean& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const boolean& src, volatile boolean& rtn) { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const boolean& src, volatile boolean& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const volatile boolean& src, volatile boolean& rtn) { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }
	void exchange(const volatile boolean& src, volatile boolean& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn.m_bool); }



	template <typename T2>
	void exchange(const boolean& src, T2& rtn) { cogs::exchange(m_bool, src.m_bool, rtn); }

	template <typename T2>
	void exchange(const boolean& src, T2& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn); }

	template <typename T2>
	void exchange(const volatile boolean& src, T2& rtn) { cogs::exchange(m_bool, src.m_bool, rtn); }

	template <typename T2>
	void exchange(const volatile boolean& src, T2& rtn) volatile { cogs::exchange(m_bool, src.m_bool, rtn); }

	bool exchange(const boolean& src) { return cogs::exchange(m_bool, src.m_bool); }
	bool exchange(const boolean& src) volatile { return cogs::exchange(m_bool, src.m_bool); }
	bool exchange(const volatile boolean& src) { return cogs::exchange(m_bool, src.m_bool); }
	bool exchange(const volatile boolean& src) volatile { return cogs::exchange(m_bool, src.m_bool); }


	bool compare_exchange(const boolean& src, const boolean& cmp) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const boolean& src, const boolean& cmp) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool); }

	bool compare_exchange(const boolean& src, const boolean& cmp, boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const boolean& cmp, boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const boolean& cmp, volatile boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const boolean& cmp, volatile boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, volatile boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, volatile boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, volatile boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, volatile boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, volatile boolean& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, volatile boolean& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn.m_bool); }

	template <typename T2>
	bool compare_exchange(const boolean& src, const boolean& cmp, T2& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }
	template <typename T2>
	bool compare_exchange(const boolean& src, const boolean& cmp, T2& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }

	template <typename T2>
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, T2& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }
	template <typename T2>
	bool compare_exchange(const volatile boolean& src, const boolean& cmp, T2& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }

	template <typename T2>
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, T2& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }
	template <typename T2>
	bool compare_exchange(const boolean& src, const volatile boolean& cmp, T2& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }

	template <typename T2>
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, T2& rtn) { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }
	template <typename T2>
	bool compare_exchange(const volatile boolean& src, const volatile boolean& cmp, T2& rtn) volatile { return cogs::compare_exchange(m_bool, src.m_bool, cmp.m_bool, rtn); }




	template <typename char_t>
	string_t<char_t> to_string_t() const;

	string_t<wchar_t> to_string() const;
	string_t<char> to_cstring() const;

	template <typename char_t>
	string_t<char_t> to_string_t() const volatile;

	string_t<wchar_t> to_string() const volatile;
	string_t<char> to_cstring() const volatile;
};


#pragma warning(pop)


}


#endif
