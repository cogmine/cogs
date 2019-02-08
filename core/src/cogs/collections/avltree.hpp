//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_AVLTREE
#define COGS_AVLTREE


#include "cogs/collections/btree.hpp"
#include "cogs/operators.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {

/// @defgroup BinaryTrees Binary Trees
/// @{
/// @ingroup Collections
/// @brief Binary Tree classes
/// @}


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified


/// @ingroup BinaryTrees
/// @brief Base classes for (intrusive) nodes of an avltree.
/// @tparam derived_t Derived type of this class.  Allows links to be returned as references to the derived type, without requiring a cast.
/// If void is specified, links will point to avltree_node_t<void, ref_type, link_iterator>.  Default: void
/// @tparam ref_type Reference type to use for links.  Default: ptr
/// @tparam link_iterator Helper type providing functions to get and set links.  Default: default_tlink_iterator<derived_t, ref_type>
template <class derived_t = void, template <typename> class ref_type = ptr, class link_iterator = default_tlink_iterator<derived_t, ref_type> >
class avltree_node_t : public tlink_t<derived_t, ref_type, link_iterator>
{
private:
	typedef avltree_node_t<derived_t, ref_type, link_iterator> this_t;
	typedef tlink_t<derived_t, ref_type, link_iterator> base_t;

	int m_factor;

	avltree_node_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	avltree_node_t()
	{ }

	template <typename P, typename L, typename R>
	avltree_node_t(int f, P&& p, L&& l, R&& r)
		: base_t(std::forward<P>(p), std::forward<L>(l), std::forward<R>(r)),
		m_factor(f)
	{ }

	avltree_node_t(this_t&& t)
		: base_t(std::move(t)),
		m_factor(t.m_factor)
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		m_factor = t.m_factor;
		return *this;
	}

	/// @brief Gets the AVL factor value of this node
	/// @return The AVL factor value of this node
	int get_factor() const	{ return m_factor; }

	/// @brief Sets the AVL factor value of this node
	/// @param f The value to set as the AVL factor of this node
	void set_factor(int f)	{ m_factor = f; }
};


template <template <typename> class ref_type, class link_iterator>
class avltree_node_t<void, ref_type, link_iterator> : public tlink_t<avltree_node_t<void, ref_type>, ref_type, link_iterator>
{
private:
	typedef avltree_node_t<void, ref_type, link_iterator> this_t;
	typedef tlink_t<avltree_node_t<void, ref_type>, ref_type, link_iterator> base_t;

	int m_factor;

	avltree_node_t(const this_t& t) = delete;
	this_t& operator=(const this_t& t) = delete;

public:
	avltree_node_t()
	{ }

	template <typename P, typename L, typename R>
	avltree_node_t(int f, P&& p, L&& l, R&& r)
		: base_t(std::forward<P>(p), std::forward<L>(l), std::forward<R>(r)),
		m_factor(f)
	{ }

	avltree_node_t(this_t&& t)
		: base_t(std::move(t)),
		m_factor(t.m_factor)
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		m_factor = t.m_factor;
		return *this;
	}

	int get_factor() const	{ return m_factor; }
	void set_factor(int f)	{ m_factor = f; }
};


/// @brief An alias to avltree_node_t<void,ptr>
typedef avltree_node_t<void,ptr>	avltree_node;


/// @ingroup BinaryTrees
/// @brief An intrusive AVL ("Adelson-Velsky-Landis") binary tree.
///
/// derived_node_t must be derived from avltree_node_t, and  include the equivalent of the following member function:
/// @code{.cpp}
///		const key_t& get_key() const;
/// @endcode
/// or:
/// @code{.cpp}
///		key_t get_key() const;
/// @endcode
/// @tparam key_t The key type to contain
/// @tparam derived_node_t A class derived from the intrusive node base, avltree_node_t.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
/// @tparam ref_type Reference type to use for links.  Default: ptr
template <typename key_t, class derived_node_t, class comparator_t = default_comparator, template <typename> class ref_type = ptr>
class avltree : public sorted_btree<key_t, derived_node_t, comparator_t, ref_type>
{
public:
	/// @brief Alias to this type.
	typedef avltree<key_t, derived_node_t, comparator_t, ref_type>		this_t;

	/// @brief Alias to the node type.
	typedef derived_node_t node_t;

	/// @brief The node reference type
	typedef ref_type<node_t> ref_t;

private:
	typedef sorted_btree<key_t, derived_node_t, comparator_t, ref_type> base_t;

	avltree(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	// rebalance balances and repaints the tree after an insert or remove.
	//
	//	- When inserting, the inserted node is passed as n and will be at max depth (no children,
	//		and factor should already be set to zero).  parent and isRightChild are needed for
	//		parity with removal (determined automatically by rebalance_after_insert()).
	//
	//	- When removing a node, the node will always be at the bottom of the tree, and will have
	//		0 or 1 children.  If there was a child moved into it's place, it is passed in n, otherwise
	//		n is null.  Since the tree was previously balanced, we know that n has no children and should
	//		already have a zero factor.  The parent of the removed node (and now of n, if present) is passed in parent.
	//		isRightChild is passed in to indicate which side of the parent node the removed node had been on
	//		(since it may be null now, and cannot be automatically determined).
	//
	template <bool isInsert>
	void rebalance(const ref_t& nIn, const ref_t& parentIn, bool isRightChildIn)
	{
		if (!parentIn)
			return;

		bool isRightChild	= isRightChildIn;
		ref_t n				= nIn;
		ref_t parent		= parentIn;
		ref_t parentParent;
		ref_t child;
		typename ref_t::locked_t lockedRef;
		typename ref_t::locked_t lockedParent;
		typename ref_t::locked_t lockedChild;

		if (isInsert)	// if (!!n)
			lockedRef = n;

		lockedParent = parent;
		parentParent = lockedParent->get_parent_link();

		int childFactor = 0;
		int factor     = 0;

		for (;;)
		{
			const int side_convert[2]	= { -1, 1 };
			const int parentFactor		= lockedParent->get_factor();
			const int side				=  side_convert[isRightChild ^ !isInsert];
			const int nSide				= -side;
			if (parentFactor == side)	// If the parent has a factor that indicates it's lopsided to the other side.
			{
				int newFactor[2];	// index 0 is newFactor, index 1 is newParentFactor
				ref_t* localRoot;

				if (!isInsert)		// figure out new n.  When removing and rotating, we need to use the excess node on the other side as N.
				{
					n = lockedParent->get_child_link(!isRightChild);
					lockedRef = n;
					factor = lockedRef->get_factor();
				}

				bool direction = (isInsert == isRightChild);
				if (factor == nSide)	// If inverted factors,
				{						// double rotate
					if (!isInsert)		// figure out new child, since we wont already know it when inserting.
					{
						child = lockedRef->get_child_link(isRightChild);
						lockedChild = child;
						childFactor = lockedChild->get_factor();
					}

					//balancing_double_rotate();
					// Fix up parents
					lockedChild->set_parent_link(parentParent);
					lockedRef->set_parent_link(child);
					lockedParent->set_parent_link(child);

					ref_t childChild1 = lockedChild->get_child_link(direction);
					if (!!childChild1)
					{
						typename ref_t::locked_t lockedChildChild1 = childChild1;
						lockedChildChild1->set_parent_link(n);
					}
					ref_t childChild2 = lockedChild->get_child_link(!direction);
					if (!!childChild2)
					{
						typename ref_t::locked_t lockedChildChild2 = childChild2;
						lockedChildChild2->set_parent_link(parent);
					}

					// fix up children
					lockedRef->set_child_link(!direction, childChild1);
					lockedParent->set_child_link(direction, childChild2);
					lockedChild->set_child_link(direction, n);
					lockedChild->set_child_link(!direction, parent);

					// fixup factors
					newFactor[0] = newFactor[1] = 0;
					if (!!childFactor)
					{
						lockedChild->set_factor(0);
						newFactor[(childFactor == side)] = -childFactor;
					}

					localRoot = &child;
				}
				else						// single rotate
				{
					if (!isInsert)
					{
						child = lockedRef->get_child_link(direction);
						lockedChild = child;
					}

					//balancing_single_rotate();
					// Fix up parents
					const ref_t& otherChild = lockedRef->get_child_link(!direction);
					lockedParent->set_parent_link(n);
					lockedRef->set_parent_link(parentParent);
					if (!!otherChild)
					{
						typename ref_t::locked_t lockedOtherChild = otherChild;
						lockedOtherChild->set_parent_link(parent);
					}
					
					// fix up children
					lockedParent->set_child_link(direction, otherChild);
					lockedRef->set_child_link(!direction, parent);	// set n's child last, so as not to stomp on otherChild ref

					// fixup factors
					newFactor[0] = factor - side;
					newFactor[1] = -(newFactor[0]);

					localRoot = &n;
				}
				lockedRef->set_factor(newFactor[0]);
				lockedParent->set_factor(newFactor[1]);

				// tree parent fixup
				if (parent == get_root())
				{
					set_root(*localRoot);
					break;
				}
				lockedParent = parentParent;	// Moving lockedParentParent into lockedParent
				isRightChild = (lockedParent->get_right_link() == parent);
				lockedParent->set_child_link(isRightChild, *localRoot);
				if (isInsert)	// If inserting, a single or double rotate means we won't need to pass anything up.
					break;
				if (parentFactor == newFactor[1])
					break;
				// If removing, we only need to pass up valid values for parent (and lockedParent), and parentParent.  N and child are computed
				parent = parentParent;
				parentParent = lockedParent->get_parent_link();
			}
			else
			{
				int newParentFactor = parentFactor + side;
				lockedParent->set_factor(newParentFactor);
				if (parent == get_root())
					break;
				if (isInsert)	// If Inserting, and no need to rotate, we stop if just changed the factor TO 0.
				{
					if (newParentFactor == 0)	//if (parentFactor == nSide)
						break;
				}
				else	// If removing, we stop if we've change a factor FROM 0.
				{
					if (parentFactor == 0) // if (!parentFactor)
						break;
				}
				
				child = n;
				lockedChild = lockedRef;
				childFactor = factor;
				n = parent;
				lockedRef = lockedParent;
				factor = newParentFactor;

				parent = parentParent;
				lockedParent = parent;
				parentParent = lockedParent->get_parent_link();
				isRightChild = (n == lockedParent->get_right_link());
			}
		}
	}

	ref_t insert(const ref_t& n, sorted_btree_insert_mode insertMode)
	{
		bool wasEmpty = is_empty();
		typename ref_t::locked_t lockedRef = n;
		ref_t existing = base_t::insert(n, insertMode);
		if (!!existing)
		{
			if (insertMode == sorted_btree_insert_mode::replace)
			{
				typename ref_t::locked_t lockedExisting = existing;
				lockedRef->set_factor(lockedExisting->get_factor());
			}
		}
		else
		{
			lockedRef->set_factor(0);
			if (!wasEmpty)
			{
				ref_t parent = lockedRef->get_parent_link();
				typename ref_t::locked_t lockedParent = parent;
				bool isRightChild = (n == lockedParent->get_right_link());
				rebalance<true>(n, parent, isRightChild);
			}
		}
		return existing;
	}

	const ref_t& get_root() const		{ return base_t::get_root();  }
	void set_root(const ref_t& r) { base_t::set_root(r); }

public:
	avltree()
	{ }

	avltree(this_t&& t)
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
	ref_t insert_multi(const ref_t& n)						{ return insert(n, sorted_btree_insert_mode::multi); }
	/// @}

	/// @{
	/// @brief Insert a node, replacing a duplicate
	/// @param n Node to insert
	/// @return A reference to the replaced node, or an empty node if no collision.
	ref_t insert_replace(const ref_t& n)					{ return insert(n, sorted_btree_insert_mode::replace); }
	/// @}

	/// @{
	/// @brief Insert a node, if an equal node is not already present
	/// @param n Node to insert
	/// @return A reference to an equal node in the case of collision, or an empty node if no collision.
	ref_t insert_unique(const ref_t& n)						{ return insert(n, sorted_btree_insert_mode::unique); }
	/// @}

	/// @{
	/// @brief Remove a node
	void remove(const ref_t& n)
	{
		ref_t liftedChild;
		ref_t swappedWith;
		typename ref_t::locked_t lockedRef = n;
		const int factor = lockedRef->get_factor();

		bool wasRightChild = base_t::balance_remove(n, swappedWith, liftedChild, (factor == 1));

		if (!!get_root())
		{
			for (;;)
			{
				ref_t parent = lockedRef->get_parent_link();
				typename ref_t::locked_t lockedSwappedWith;
				if (!swappedWith)	// If no swap occured, we know the removed node has 0 or 1 children.
				{					// Go ahead and set its parent's factor accordingly, and enter rebalance one level up.
					if (!parent)	// if the root is being removed, we know it's now empty, or has just 1 element.
					{
						if (!!liftedChild)	// If just 1 element, set its factor to 0 and clear the parent
						{
							typename ref_t::locked_t lockedLiftedChild = liftedChild;
							lockedLiftedChild->set_factor(0);
							ref_t emptyRef;
							lockedLiftedChild->set_parent_link(emptyRef);
						}
						break;
					}
				}
				else
				{
					lockedSwappedWith = swappedWith;
					lockedSwappedWith->set_factor(factor);	// If a swap occured, trade factors with it.
				}
				rebalance<false>(liftedChild, parent, wasRightChild);
				break;
			}
		}
	}
	/// @}
};



#pragma warning(pop)


}


#endif
