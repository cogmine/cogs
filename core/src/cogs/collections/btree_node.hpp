//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_BTREE_NODE
#define COGS_HEADER_COLLECTION_BTREE_NODE

#include <type_traits>

#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @ingroup CollectionAccessorMixIns
/// @brief Provides a default btree_node accessor mix-in type which leverages accessors in the intrusive element.
/// @tparam link_t The link type to wrap access to.
/// @tparam ref_type Type used to reference elements.  Default: ptr
template <class link_t, template <typename> class ref_type = ptr>
class default_btree_node_accessor
{
public:
	typedef ref_type<link_t> ref_t;

	static const ref_t& get_parent(const link_t& l) { return l.get_parent_link(); }
	static const volatile ref_t& get_parent(const volatile link_t& l) { return l.get_parent_link(); }

	static const ref_t& get_left(const link_t& l) { return l.get_left_link(); }
	static const volatile ref_t& get_left(const volatile link_t& l) { return l.gget_left_link(); }

	static const ref_t& get_right(const link_t& l) { return l.get_right_link(); }
	static const volatile ref_t& get_right(const volatile link_t& l) { return l.get_right_link(); }

	static void set_parent(link_t& l, const link_t& src) { l.set_parent_link(src); }
	static void set_parent(volatile link_t& l, const link_t& src) { l.set_parent_link(src); }

	static void set_left(link_t& l, const link_t& src) { l.set_left_link(src); }
	static void set_left(volatile link_t& l, const link_t& src) { l.set_left_link(src); }

	static void set_right(link_t& l, const link_t& src) { l.set_right_link(src); }
	static void set_right(volatile link_t& l, const link_t& src) { l.set_right_link(src); }
};


template <class link_t, template <typename> class ref_type = ptr, class link_accessor = default_btree_node_accessor<link_t, ref_type> >
class btree_node_methods
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

private:
	static const ref_t& get_child(const link_t& l, bool right) { return right ? get_right(l) : get_left(l); }
	static const volatile ref_t& get_child(const volatile link_t& l, bool right) { return right ? get_right(l) : get_left(l); }

	static void set_child(link_t& l, bool right, const ref_t& src)
	{
		if (right)
			set_right(l, src);
		else
			set_left(l, src);
	}

	static void set_child(volatile link_t& l, bool right, const ref_t& src)
	{
		if (right)
			set_right(l, src);
		else
			set_left(l, src);
	}

	static ref_t get_next_preorder_or_prev_postorder(const link_t& l, bool preorder)
	{
		bool postorder = !preorder;
		ref_t n = get_child(l, postorder);
		if (!n)
		{
			n = get_child(l, preorder);
			if (!n)
			{
				ref_t parent = get_parent(l);
				typename ref_t::lock_t parentResolved;
				if (!!parent)
				{
					bool b = get_child(*parentResolved, postorder) == &l;
					for (;;)
					{
						if (b)
						{
							const ref_t& parentChild = get_child(*parentResolved, preorder);
							if (!!parentChild)
								return parentChild;
						}
						n = parent;
						parent = get_parent(*parentResolved);
						if (!parent)
							break;
						parentResolved = parent;
						b = get_child(*parentResolved, postorder) == n;
					}
				}
				ref_t emptyRef;
				return emptyRef;
			}
		}
		return n;
	}

	static ref_t get_prev_preorder_or_next_postorder(const link_t& l, bool preorder)
	{
		const ref_t& parent = get_parent(l);
		if (!!parent)
		{
			typename ref_t::lock_t parentResolved = parent;
			const ref_t& parentChild = get_child(*parentResolved, !preorder);
			if (!!parentChild && get_child(*parentResolved, preorder) == &l)
				return get_sidebottom(parentChild, preorder);
		}
		return parent;
	}

public:
	static ref_t get_next(const link_t& l) { return get_inorder(l, true); }
	static ref_t get_prev(const link_t& l) { return get_inorder(l, false); }

	static ref_t get_next_inorder(const link_t& l) { return get_inorder(l, true); }
	static ref_t get_prev_inorder(const link_t& l) { return get_inorder(l, false); }

	static ref_t get_next_preorder(const link_t& l) { return get_next_preorder_or_prev_postorder(l, true); }
	static ref_t get_prev_preorder(const link_t& l) { return get_prev_preorder_or_next_postorder(l, true); }

	static ref_t get_next_postorder(const link_t& l) { return get_prev_preorder_or_next_postorder(l, false); }
	static ref_t get_prev_postorder(const link_t& l) { return get_next_preorder_or_prev_postorder(l, false); }

	static ref_t get_inorder(const link_t& l, bool next)
	{
		const ref_t& child = get_child(l, next);
		if (!!child)
		{
			typename ref_t::lock_t lockedChildRef = child;
			ref_t child2;
			if (next)
				child2 = get_leftmost(*lockedChildRef);
			else
				child2 = get_rightmost(*lockedChildRef);
			if (!child2)
				return child;
			return child2;
		}
		ref_t parent = get_parent(l);
		if (!!parent)
		{
			typename ref_t::lock_t lockedRef = parent;
			if (get_child(*lockedRef, next) == &l)
			{
				for (;;)
				{
					ref_t n = parent;
					parent = get_parent(*lockedRef);
					if (!parent)
						break;
					lockedRef = parent;
					if (n != get_child(*lockedRef, next))
						break;
				}
			}
		}
		return parent;
	}

	static const ref_t get_leftmost(const link_t& l)
	{
		ref_t n = get_left(l);
		if (!!n)
		{
			for (;;)
			{
				typename ref_t::lock_t lockedRef = n;
				const ref_t& n_child = get_left(*lockedRef);
				if (!n_child)
					break;
				n = n_child;
			}
		}
		return n;
	}

	static const ref_t get_rightmost(const link_t& l)
	{
		ref_t n = get_right(l);
		if (!!n)
		{
			for (;;)
			{
				typename ref_t::lock_t lockedRef = n;
				const ref_t& n_child = get_right(*lockedRef);
				if (!n_child)
					break;
				n = n_child;
			}
		}
		return n;
	}

	static ref_t get_inorder(const ref_t& l, bool next)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_inorder(*lockedRef, next);
		}
		return n;
	}

	static ref_t get_next(const ref_t& l) { return get_next_inorder(l); }
	static ref_t get_prev(const ref_t& l) { return get_prev_inorder(l); }

	static ref_t get_next_inorder(const ref_t& l) { return get_inorder(l, true); }
	static ref_t get_prev_inorder(const ref_t& l) { return get_inorder(l, false); }

	static ref_t get_next_preorder(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_next_preorder(*lockedRef);
		}
		return n;
	}

	static ref_t get_prev_preorder(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_prev_preorder(*lockedRef);
		}
		return n;
	}

	static ref_t get_next_postorder(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_next_postorder(*lockedRef);
		}
		return n;
	}

	static ref_t get_prev_postorder(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_prev_postorder(*lockedRef);
		}
		return n;
	}

	static const ref_t get_leftmost(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_leftmost(*lockedRef);
			if (!n)
				n = l;
		}
		return n;
	}

	static const ref_t get_rightmost(const ref_t& l)
	{
		ref_t n;
		if (!!l)
		{
			typename ref_t::lock_t lockedRef = l;
			n = get_rightmost(*lockedRef);
			if (!n)
				n = l;
		}
		return n;
	}

	static const ref_t get_sidebottom(const ref_t& l, bool right)
	{
		COGS_ASSERT(!!l);
		ref_t n = l;
		ref_t child;
		typename ref_t::lock_t lockedRef;
		for (;;)
		{
			lockedRef = n;
			for (;;)
			{
				child = get_child(*lockedRef, right);
				if (!child)
					break;
				n = child;
				lockedRef = n;
			}
			child = get_child(*lockedRef, !right);
			if (!child)
				break;
			n = child;
		}
		return n;
	}
};


/// @ingroup BinaryTrees
/// @brief Base class for a triple-link (tree) element.  Does not include storage or link accessor methods.
/// @tparam derived_t Derived type of this class.
/// This <a href="https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern">curiously recurring template pattern</a>
/// allows links to refer to the derived type.  Storage and access can be defined in the derived type.
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Mix-in type providing access to the link.  Default: default_link_accessor<derived_t, ref_type>
template <class derived_t, template <typename> class ref_type = ptr, class link_accessor = default_btree_node_accessor<derived_t, ref_type> >
class btree_node_base
{
public:
	typedef btree_node_base<derived_t, ref_type, link_accessor> this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t> link_t;
	typedef ref_type<link_t> ref_t;
	typedef btree_node_methods<link_t, ref_type, link_accessor> dlink_methods_t;

	ref_t get_next() const { return dlink_methods_t::get_inorder(*(const link_t*)this, true); }
	ref_t get_prev() const { return dlink_methods_t::get_inorder(*(const link_t*)this, false); }

	ref_t get_next_inorder() const { return dlink_methods_t::get_inorder(*(const link_t*)this, true); }
	ref_t get_prev_inorder() const { return dlink_methods_t::get_inorder(*(const link_t*)this, false); }

	ref_t get_next_preorder() const { return dlink_methods_t::get_next_preorder_or_prev_postorder(*(const link_t*)this, true); }
	ref_t get_prev_preorder() const { return dlink_methods_t::get_prev_preorder_or_next_postorder(*(const link_t*)this, true); }

	ref_t get_next_postorder() const { return dlink_methods_t::get_prev_preorder_or_next_postorder(*(const link_t*)this, false); }
	ref_t get_prev_postorder() const { return dlink_methods_t::get_next_preorder_or_prev_postorder(*(const link_t*)this, false); }

	ref_t get_inorder(bool next) const { return dlink_methods_t::get_inorder(next); }

	ref_t get_leftmost() const { return dlink_methods_t::get_leftmost(*this); }
	ref_t get_rightmost() const { return dlink_methods_t::get_rightmost(*this); }

	static ref_t get_next(const ref_t& l) { return dlink_methods_t::get_next(l); }
	static ref_t get_prev(const ref_t& l) { return dlink_methods_t::get_prev(l); }

	static ref_t get_next_inorder(const ref_t& l) { return dlink_methods_t::get_next_inorder(l); }
	static ref_t get_prev_inorder(const ref_t& l) { return dlink_methods_t::get_prev_inorder(l); }

	static ref_t get_next_preorder(const ref_t& l) { return dlink_methods_t::get_next_preorder(l); }
	static ref_t get_prev_preorder(const ref_t& l) { return dlink_methods_t::get_prev_preorder(l); }

	static ref_t get_next_postorder(const ref_t& l) { return dlink_methods_t::get_next_postorder(l); }
	static ref_t get_prev_postorder(const ref_t& l) { return dlink_methods_t::get_prev_postorder(l); }

	static ref_t get_inorder(const ref_t& l, bool next) { return dlink_methods_t::get_inorder(l, next); }

	static const ref_t get_leftmost(const ref_t& l) { return dlink_methods_t::get_leftmost(l); }
	static const ref_t get_rightmost(const ref_t& l) { return dlink_methods_t::get_rightmost(l); }
	static const ref_t get_sidebottom(const ref_t& l, bool right) { return dlink_methods_t::get_sidebottom(l, right); }

};


/// @ingroup BinaryTrees
/// @brief Base class for a triple-link (tree) element.
/// @tparam derived_t Derived type of this class.
/// This <a href="https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern">curiously recurring template pattern</a>
/// allows links to refer to the derived type.
/// If void is specified, links will point to btree_node_t<void, ref_type, link_accessor>.  Default: void
/// @tparam ref_type Type used to reference elements.  Default: ptr
/// @tparam link_accessor Mix-in type providing access to the link.  Default: default_btree_node_accessor<derived_t, ref_type>
template <class derived_t = void, template <typename> class ref_type = ptr, class link_accessor = default_btree_node_accessor<derived_t, ref_type> >
class btree_node_t : public btree_node_base<derived_t, ref_type, link_accessor>
{
public:
	typedef btree_node_t<derived_t, ref_type> this_t;
	typedef std::conditional_t<std::is_void_v<derived_t>, this_t, derived_t> link_t;
	typedef ref_type<link_t> ref_t;

private:
	ref_t m_parent;
	ref_t m_left;
	ref_t m_right;

	btree_node_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	btree_node_t()
	{ }

	template <typename P, typename L, typename R>
	btree_node_t(P&& p, L&& l, R&& r)
		: m_parent(std::forward<P>(p)),
		m_left(std::forward<L>(l)),
		m_right(std::forward<R>(r))
	{ }

	btree_node_t(this_t&& t)
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
class default_btree_node_accessor<void, ref_type> : public default_btree_node_accessor<btree_node_t<void, ref_type, default_btree_node_accessor<void, ref_type> > >
{
};

typedef btree_node_t<void> btree_node;

}

#endif
