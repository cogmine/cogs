//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_RBTREE
#define COGS_HEADER_COLLECTION_RBTREE


#include "cogs/operators.hpp"
#include "cogs/collections/btree.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @ingroup BinaryTrees
/// @brief Base classes for (intrusive) nodes of a rbtree.
/// @tparam derived_t Derived type of this class.  Allows links to be returned as references to the derived type, without requiring a cast.
/// If void is specified, links will point to rbtree_node_t<void, ref_type, link_accessor>.  Default: void
/// @tparam ref_type Reference type to use for links.  Default: ptr
/// @tparam link_accessor Helper type providing functions to get and set links.  Default: default_tlink_accessor<derived_t, ref_type>
template <class derived_t, template <typename> class ref_type = ptr, class link_accessor = default_tlink_accessor<derived_t, ref_type> >
class rbtree_node_t : public tlink_t<derived_t, ref_type, link_accessor>
{
private:
	typedef rbtree_node_t<derived_t, ref_type, link_accessor> this_t;
	typedef tlink_t<derived_t, ref_type, link_accessor> base_t;

	bool m_color;

	rbtree_node_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	rbtree_node_t()
	{ }

	template <typename P, typename L, typename R>
	rbtree_node_t(bool isRed, P&& p, L&& l, R&& r)
		: base_t(std::forward<P>(p), std::forward<L>(l), std::forward<R>(r)),
		m_color(isRed)
	{ }

	rbtree_node_t(this_t&& t)
		: base_t(std::move(t)),
		m_color(t.m_color)
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		m_color = t.m_color;
		return *this;
	}

	static constexpr bool red = true;
	static constexpr bool black = false;

	bool get_color() const { return m_color; }
	void set_color(bool color) { m_color = color; }

	bool is_red() const { return m_color; }
	void set_red(bool useRed) { m_color = useRed; }

	// Must be defined in a derived class:
	// const key_t& get_key() const; or  key_t get_key() const;
};


template <template <typename> class ref_type, class link_accessor>
class rbtree_node_t<void, ref_type, link_accessor> : public tlink_t<rbtree_node_t<void, ref_type>, ref_type, link_accessor>
{
private:
	typedef rbtree_node_t<void, ref_type, link_accessor> this_t;
	typedef tlink_t<rbtree_node_t<void, ref_type>, ref_type, link_accessor> base_t;

	bool m_color;

	rbtree_node_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	rbtree_node_t()
	{ }

	template <typename P, typename L, typename R>
	rbtree_node_t(bool isRed, P&& p, L&& l, R&& r)
		: base_t(std::forward<P>(p), std::forward<L>(l), std::forward<R>(r)),
		m_color(isRed)
	{ }

	rbtree_node_t(this_t&& t)
		: base_t(std::move(t)),
		m_color(t.m_color)
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		m_color = t.m_color;
		return *this;
	}

	static constexpr bool red = true;
	static constexpr bool black = false;

	bool get_color() const { return m_color; }
	void set_color(bool color) { m_color = color; }

	bool is_red() const { return m_color; }
	void set_red(bool useRed) { m_color = useRed; }
};


/// @brief An alias to rbtree_node_t<void,ptr>
typedef rbtree_node_t<void,ptr> rbtree_node;


/// @ingroup BinaryTrees
/// @brief An intrusive red-black binary tree.
///
/// derived_node_t must be derived from rbtree_node_t, and  include the equivalent of the following member function:
/// @code{.cpp}
/// const key_t& get_key() const;
/// @endcode
/// or:
/// @code{.cpp}
/// key_t get_key() const;
/// @endcode
/// @tparam key_t The key type to contain
/// @tparam derived_node_t A class derived from the intrusive node base, rbtree_node_t.  Default: rbtree_node
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
/// @tparam ref_type Reference type to use for links.  Default: ptr
template <typename key_t, class derived_node_t, class comparator_t = default_comparator, template <typename> class ref_type = ptr>
class rbtree : public sorted_btree<key_t, derived_node_t, comparator_t, ref_type>
{
public:
	/// @brief Alias to this type.
	typedef rbtree<key_t, derived_node_t, comparator_t, ref_type> this_t;

	/// @brief Alias to the node type.
	typedef derived_node_t node_t;

	/// @brief The node reference type
	typedef ref_type<node_t> ref_t;

private:
	typedef sorted_btree<key_t, derived_node_t, comparator_t, ref_type> base_t;

	rbtree(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	// Balances and re-paints the tree after an insert
	void balance(const ref_t& nIn)
	{
		ref_t n(nIn);

		// We know that parent is valid, because balance would not be called if n were already the root
		ref_t parent;
		ref_t parentParent;
		ref_t other;
		typename ref_t::locked_t lockedRef = n;
		typename ref_t::locked_t lockedParent;
		typename ref_t::locked_t lockedParentParent;
		typename ref_t::locked_t lockedOther;
		for (;;)
		{
			parent = lockedRef->get_parent_link();
			lockedParent = parent;
			if (!lockedParent->is_red()) // no need to check for root  (???)
				break;

			parentParent = lockedParent->get_parent_link();
			lockedParentParent = parentParent; //??
			bool wasRightNode = (lockedParentParent->get_right_link() == parent);

			other = lockedParentParent->get_child_link(!wasRightNode);
			if (!!other)
			{
				lockedOther = other;
				if (lockedOther->is_red()) // red uncle node
				{
					lockedOther->set_red(false);
					lockedParent->set_red(false);
					if (parentParent == get_root())
						break;
					lockedParentParent->set_red(true);
					n = parentParent;
					lockedRef = lockedParentParent;
					continue;
				}
			}

			// black uncle node
			lockedParentParent->set_red(true);
			if (n != lockedParent->get_child_link(!wasRightNode)) // Cant do second rotate if nodes are in inverse directions.  Quick rotate to change direction of lower one.
				lockedParent->set_red(false);
			else
			{
				lockedRef->set_red(false); // Actually parent due to the swap

				// rotate such that x moves up to become parentParent's new child
				//rotate_quick(wasRightNode, parent, n);
				//inline to remove some redundant steps and maximum use of locked references
				ref_t childChild = lockedRef->get_child_link(wasRightNode);

				lockedRef->set_parent_link(lockedParent->get_parent_link());
				lockedRef->set_child_link(wasRightNode, parent);
				lockedParent->set_parent_link(n);

				lockedParent->set_child_link(!wasRightNode, childChild);
				if (!!childChild)
				{
					typename ref_t::locked_t lockedChildChild = childChild;
					lockedChildChild->set_parent_link(parent);
				}

				if (n == get_root())
					base_t::set_root(n);
				else
					lockedParentParent->set_child_link(wasRightNode, n);
				parent = n;
				lockedParent = lockedRef;
			}

			//rotate(!wasRightNode, parentParent); // aka: rotate_quick(!wasRightNode, parentParent, parent);//lockedParentParent->get_child_link(wasRightNode)); // aka:
			//inline to remove some redundant steps and maximum use of locked references

			ref_t childChild = lockedParent->get_child_link(!wasRightNode);
			ref_t parentParentParent = lockedParentParent->get_parent_link();
			lockedParent->set_parent_link(parentParentParent);
			lockedParent->set_child_link(!wasRightNode, parentParent);
			lockedParentParent->set_parent_link(parent);

			lockedParentParent->set_child_link(wasRightNode, childChild);
			if (!!childChild)
			{
				typename ref_t::locked_t lockedChildChild = childChild;
				lockedChildChild->set_parent_link(parentParent);
			}

			if (parentParent == get_root())
				base_t::set_root(parent);
			else
			{
				typename ref_t::locked_t lockedParentParentParent = parentParentParent;
				lockedParentParentParent->set_child_link((lockedParentParentParent->get_right_link() == parentParent), parent);
			}
			break;
		}
	}

	ref_t insert(const ref_t& n, sorted_btree_insert_mode insertMode)
	{
		typename ref_t::locked_t lockedRef = n;
		bool wasEmpty = is_empty();
		ref_t existing = base_t::insert(n, insertMode);
		if (!!existing)
		{
			if (insertMode == sorted_btree_insert_mode::replace)
			{
				typename ref_t::locked_t lockedExisting = existing;
				lockedRef->set_red(lockedExisting->is_red());
			}
			// if sorted_btree_insert_mode::replace, do nothing.  Don't get here if insert_multiple.  So do nothing.
		}
		else
		{
			// Inserted nodes are at maximum depth (no children), and are always created as red.
			lockedRef->set_red(!wasEmpty); // except for the root node, which is always black.
			if (!wasEmpty)
				balance(n);
		}
		return existing;
	}

	const ref_t& get_root() const { return base_t::get_root();  }

public:
	rbtree()
	{ }

	rbtree(this_t&& t)
		: base_t(std::move(t))
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		return *this;
	}

	/// @{
	/// @brief Swaps the contents of this tree with another.
	/// @param srcDst The tree to swap with.
	void swap(this_t& srcDst) { base_t::swap(srcDst); }
	/// @}

	/// @{
	/// @brief Tests if the tree is empty
	/// @return True if the tree is empty
	bool is_empty() const { return base_t::is_empty(); }
	/// @}

	/// @{
	/// @brief Clears the root, leftmost and rightmost values maintained by the tree.  Does not delete any elements.
	void clear() { base_t::clear(); }
	/// @}

	/// @{
	/// @brief Get the first in-order node
	/// @return The first in-order node
	const ref_t& get_first() const { return base_t::get_first(); }
	/// @}

	/// @{
	/// @brief Get the last in-order node
	/// @return The last in-order node
	const ref_t& get_last() const { return base_t::get_last(); }
	/// @}

	/// @{
	/// @brief Insert a node, allowing duplicates
	/// @param n Node to insert
	/// @return A reference to an equal node in the case of collision, or an empty node if no collision.
	ref_t insert_multi(const ref_t& n) { return insert(n, sorted_btree_insert_mode::multi); }
	/// @}

	/// @{
	/// @brief Insert a node, replacing a duplicate
	/// @param n Node to insert
	/// @return A reference to the replaced node, or an empty node if no collision.
	ref_t insert_replace(const ref_t& n) { return insert(n, sorted_btree_insert_mode::replace); }
	/// @}

	/// @{
	/// @brief Insert a node, if an equal node is not already present
	/// @param n Node to insert
	/// @return A reference to an equal node in the case of collision, or an empty node if no collision.
	ref_t insert_unique(const ref_t& n) { return insert(n, sorted_btree_insert_mode::unique); }
	/// @}

	/// @{
	/// @brief Remove a node
	void remove(const ref_t& n)
	{
		ref_t x;
		ref_t xParent;
		ref_t swappedWith;

		typename ref_t::locked_t lockedRef = n;

		//bool was_right_child =
		base_t::balance_remove(n, swappedWith, x, true);
		xParent = lockedRef->get_parent_link();

		if (!!swappedWith)
		{
			const bool tmp_red = lockedRef->is_red(); // swap color
			lockedRef->set_red(swappedWith->is_red());
			swappedWith->set_red(tmp_red);
		}

		// repaint the tree

		// x might be empty
		// xParent will not be empty unless n was root
		if (!lockedRef->is_red())
		{
			typename ref_t::locked_t lockedX;
			typename ref_t::locked_t lockedXParent;
			if (!!x)
				lockedX = x;
			while (x != get_root()) // root will never be red
			{
				if (!!x)
				{
					if (lockedX->is_red())
						break;
				}

				// x may be empty, and will not be red
				lockedXParent = xParent;
				ref_t y = lockedXParent->get_right_link();
				const bool was_right_child = (x == y);
				if (was_right_child)
					y = lockedXParent->get_left_link();
					// if was_right_child y is left side
					// if !was_right_child y is right side

				// y is xParent's other child, and might be empty
				if (!!y)
				{
					typename ref_t::locked_t lockedY = y;
					for (;;)
					{
						if (lockedY->is_red())
						{
							lockedY->set_red(false);
							lockedXParent->set_red(true);


							if (was_right_child)
								base_t::rotate_right(xParent);
							else
								base_t::rotate_left(xParent);

							y = lockedXParent->get_child_link(!was_right_child); // y updates to remain xParent's other child
							if (!y)
								break;
							lockedY = y;
						}

						bool are_red[2] = { false, false };
						ref_t y_children[2] = { lockedY->get_left_link(), lockedY->get_right_link() };
						typename ref_t::locked_t lockedYChildren[2] = { y_children[0], y_children[1] };

						if (!!y_children[0])
							are_red[0] = lockedYChildren[0]->is_red();
						if (!!y_children[1])
							are_red[1] = lockedYChildren[1]->is_red();

						if (are_red[0] || are_red[1])
						{
							if (!are_red[!was_right_child]) // so are_red[was_right_child] must be true, child[was_right_child] must be valid
							{
								lockedYChildren[was_right_child]->set_red(false);
								lockedY->set_red(true);
								if (!was_right_child)
									base_t::rotate_right(y, false);
								else
									base_t::rotate_left(y, false);
								y_children[!was_right_child] = y;
								lockedYChildren[!was_right_child] = y;
								y = y_children[was_right_child];
								lockedY = lockedYChildren[was_right_child]; //lockedY = y;
								//y = elem_t::rotate(!was_right_child, y); // moves up one of y's children to take its place
								// y will be become y_children[was_right_child] always
								lockedXParent->set_child_link(!was_right_child, y);
								// y will not be null
								// y's left child will be valid (the old y)
							}
							// else // are_red[!was_right_child] is red, so y's child[!was_right_child] is valid
							lockedY->set_red(lockedXParent->is_red());
							lockedXParent->set_red(false);
							lockedYChildren[!was_right_child]->set_red(false);

							if (was_right_child)
								base_t::rotate_right(xParent);
							else
								base_t::rotate_left(xParent);

							if (!!x)
								lockedX->set_red(false);
							return;
						}
						lockedY->set_red(true);
						break;
					}
				}
				x = xParent;
				lockedX = x;
				xParent = lockedX->get_parent_link();
			}
			// x is root, or x is red
			if (!!x)
				lockedX->set_red(false);
		}
	}
	/// @}
};


}


#endif
