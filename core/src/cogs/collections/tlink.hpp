//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_TLINK
#define COGS_HEADER_COLLECTION_TLINK

#include <type_traits>

#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @ingroup CollectionAccessorMixIns
/// @brief Provides a default tlink accessor mix-in type which leverages accessors in the intrusive element.
/// @tparam link_t The link type to wrap access to.
/// @tparam ref_type Type used to reference elements.  Default: ptr
template <class link_t, template <typename> class ref_type = ptr>
class default_tlink_accessor
{
public:
	typedef ref_type<link_t> ref_t;

	static const ref_t& get_parent(const link_t& l) { return l.get_prev_link(); }
	static const volatile ref_t& get_parent(const volatile link_t& l) { return l.get_prev_link(); }

	static const ref_t& get_left(const link_t& l) { return l.get_prev_link(); }
	static const volatile ref_t& get_left(const volatile link_t& l) { return l.get_prev_link(); }

	static const ref_t& get_right(const link_t& l) { return l.get_prev_link(); }
	static const volatile ref_t& get_right(const volatile link_t& l) { return l.get_prev_link(); }


	static void set_parent(link_t& l, const link_t& src) { l.set_prev_link(src); }
	static void set_parent(volatile link_t& l, const link_t& src) { l.set_prev_link(src); }

	static void set_left(link_t& l, const link_t& src) { l.set_prev_link(src); }
	static void set_left(volatile link_t& l, const link_t& src) { l.set_prev_link(src); }

	static void set_right(link_t& l, const link_t& src) { l.set_prev_link(src); }
	static void set_right(volatile link_t& l, const link_t& src) { l.set_prev_link(src); }
};


template <class link_t, template <typename> class ref_type = ptr, class link_accessor = default_tlink_accessor<link_t, ref_type> >
class tlink_methods
{
public:
	typedef ref_type<link_t> ref_t;

	static const ref_t& get_parent(const link_t& l) { return link_accessor::get_parent(l); }
	static const volatile ref_t& get_parent(const volatile link_t& l) { return link_accessor::get_parent(l); }
	static const ref_t& get_left(const link_t& l) { return link_accessor::get_left(l); }
	static const volatile ref_t& get_left(const volatile link_t& l) { return link_accessor::get_left(l); }
	static const ref_t& get_right(const link_t& l) { return link_accessor::get_right(l); }
	static const volatile ref_t& get_right(const volatile link_t& l) { return link_accessor::get_right(l); }

	static void set_parent(link_t& l, const ref_t& src) { link_accessor::set_parent(l, src); }
	static void set_parent(volatile link_t& l, const ref_t& src) { link_accessor::set_parent(l, src); }
	static void set_left(link_t& l, const ref_t& src) { link_accessor::set_left(l, src); }
	static void set_left(volatile link_t& l, const ref_t& src) { link_accessor::set_left(l, src); }
	static void set_right(link_t& l, const ref_t& src) { link_accessor::set_right(l, src); }
	static void set_right(volatile link_t& l, const ref_t& src) { link_accessor::set_right(l, src); }

	// Methods?
	// TBD
};


/// @ingroup BinaryTrees
/// @brief Base class for a triple-link (tree) element.  Does not include storage or link accessor methods.
/// @tparam derived_t Derived type of this class.
/// This <a href="https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern">curiously recurring template pattern</a>
/// allows links to refer to the derived type.  Storage and access can be defined in the derived type.
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Mix-in type providing access to the link.  Default: default_link_accessor<derived_t, ref_type>
template <class derived_t, template <typename> class ref_type = ptr, class link_accessor = default_tlink_accessor<derived_t, ref_type> >
class tlink_base
{
public:
	typedef tlink_base<derived_t, ref_type, link_accessor> this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t> link_t;
	typedef ref_type<link_t> ref_t;
	typedef tlink_methods<link_t, ref_type, link_accessor> dlink_methods_t;

	static const ref_t& get_parent(const link_t& l) { return link_accessor::get_parent(l); }
	static const volatile ref_t& get_parent(const volatile link_t& l) { return link_accessor::get_parent(l); }
	static const ref_t& get_left(const link_t& l) { return link_accessor::get_left(l); }
	static const volatile ref_t& get_left(const volatile link_t& l) { return link_accessor::get_left(l); }
	static const ref_t& get_right(const link_t& l) { return link_accessor::get_right(l); }
	static const volatile ref_t& get_right(const volatile link_t& l) { return link_accessor::get_right(l); }

	static void set_parent(link_t& l, const ref_t& src) { link_accessor::set_next(l, src); }
	static void set_parent(volatile link_t& l, const ref_t& src) { link_accessor::set_next(l, src); }
	static void set_left(link_t& l, const ref_t& src) { link_accessor::set_prev(l, src); }
	static void set_left(volatile link_t& l, const ref_t& src) { link_accessor::set_prev(l, src); }
	static void set_right(link_t& l, const ref_t& src) { link_accessor::set_prev(l, src); }
	static void set_right(volatile link_t& l, const ref_t& src) { link_accessor::set_prev(l, src); }
};


/// @ingroup BinaryTrees
/// @brief Base class for a triple-link (tree) element.
/// @tparam derived_t Derived type of this class.
/// This <a href="https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern">curiously recurring template pattern</a>
/// allows links to refer to the derived type.
/// If void is specified, links will point to tlink_t<void, ref_type, link_accessor>.  Default: void
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Mix-in type providing access to the link.  Default: default_tlink_accessor<derived_t, ref_type>


/// @ingroup BinaryTrees
/// @brief A triple-link list (tree) element.
/// @tparam derived_t Derived type of this class.  Allows links to be returned as references to the derived type, without requiring a cast.
/// If void is specified, links will point to tlink_t<void, ref_type, link_accessor>.  Default: void
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_tlink_accessor
template <class derived_t = void, template <typename> class ref_type = ptr, class link_accessor = default_tlink_accessor<derived_t, ref_type> >
class tlink_t : public tlink_base<derived_t, ref_type, link_accessor>
{
public:
	typedef tlink_t<derived_t, ref_type> this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t> link_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_parent;
	ref_t m_left;
	ref_t m_right;

	tlink_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	tlink_t()
	{ }

	template <typename P, typename L, typename R>
	tlink_t(P&& p, L&& l, R&& r)
		: m_parent(std::forward<P>(p)),
		m_left(std::forward<L>(l)),
		m_right(std::forward<R>(r))
	{ }

	tlink_t(this_t&& t)
		: m_parent(std::move(t.m_parent)),
		m_left(std::move(t.m_left)),
		m_right(std::move(t.m_right))
	{ }

	this_t& operator=(this_t&& t)
	{
		m_parent = std::move(t.m_parent);
		m_left = std::move(t.m_left);
		m_right = std::move(t.m_right);
		return *this;
	}

	ref_t& get_parent_link() { return m_parent; }
	const ref_t& get_parent_link() const { return m_parent; }
	volatile ref_t& get_parent_link() volatile { return m_parent; }
	const volatile ref_t& get_parent_link() const volatile { return m_parent; }

	ref_t& get_left_link() { return m_left; }
	const ref_t& get_left_link() const { return m_left; }
	volatile ref_t& get_left_link() volatile { return m_left; }
	const volatile ref_t& get_left_link() const volatile { return m_left; }

	ref_t& get_right_link() { return m_right; }
	const ref_t& get_right_link() const { return m_right; }
	volatile ref_t& get_right_link() volatile { return m_right; }
	const volatile ref_t& get_right_link() const volatile { return m_right; }

	ref_t& get_child_link(bool right) { return right ? get_right_link() : get_left_link(); }
	const ref_t& get_child_link(bool right) const { return right ? get_right_link() : get_left_link(); }
	volatile ref_t& get_child_link(bool right) volatile { return right ? get_right_link() : get_left_link(); }
	const volatile ref_t& get_child_link(bool right) const volatile { return right ? get_right_link() : get_left_link(); }

	void set_parent_link(const ref_t& n) { m_parent = n; }
	void set_parent_link(const volatile ref_t& n) { m_parent = n; }
	void set_parent_link(const ref_t& n) volatile { m_parent = n; }
	void set_parent_link(const volatile ref_t& n) volatile { m_parent = n; }

	void set_left_link(const ref_t& n) { m_left = n; }
	void set_left_link(const volatile ref_t& n) { m_left = n; }
	void set_left_link(const ref_t& n) volatile { m_left = n; }
	void set_left_link(const volatile ref_t& n) volatile { m_left = n; }

	void set_right_link(const ref_t& n) { m_right = n; }
	void set_right_link(const volatile ref_t& n) { m_right = n; }
	void set_right_link(const ref_t& n) volatile { m_right = n; }
	void set_right_link(const volatile ref_t& n) volatile { m_right = n; }

	void set_child_link(bool right, const ref_t& n) { if (right) set_right_link(n); else set_left_link(n); }
	void set_child_link(bool right, const volatile ref_t& n) { if (right) set_right_link(n); else set_left_link(n); }
	void set_child_link(bool right, const ref_t& n) volatile { if (right) set_right_link(n); else set_left_link(n); }
	void set_child_link(bool right, const volatile ref_t& n) volatile { if (right) set_right_link(n); else set_left_link(n); }
};


template <template <typename> class ref_type>
class default_tlink_accessor<void, ref_type> : public default_tlink_accessor<tlink_t<void, ref_type, default_tlink_accessor<void, ref_type> > >
{
};

typedef tlink_t<void> tlink;

}

#endif
