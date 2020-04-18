//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_BTREE
#define COGS_HEADER_COLLECTION_BTREE


#include "cogs/collections/tlink.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


/// @brief Tree traversal order
enum class btree_traversal
{
	/// @brief In-order binary tree traversal
	inorder,// = 0x01,

	/// @brief Pre-order binary tree traversal
	preorder,// = 0x02,

	/// @brief Post-order binary tree traversal
	postorder,// = 0x03,

	/// @brief Reverse in-order binary tree traversal
	reverse_inorder,// = 0x04,

	/// @brief Reverse pre-order binary tree traversal
	reverse_preorder,// = 0x05,

	/// @brief Reverse post-order binary tree traversal
	reverse_postorder,// = 0x06
};


/// @ingroup BinaryTrees
/// @brief A base class for intrusive binary trees.
template <class derived_node_t = tlink_t<void, ptr>, template <typename> class ref_type = ptr>
class btree
{
public:
	/// @brief Alias to this type.
	typedef btree<derived_node_t, ref_type> this_t;

	/// @brief Alias to the node type.
	typedef derived_node_t node_t;

	/// @brief The node reference type
	typedef ref_type<node_t> ref_t;

private:
	ref_t m_root;
	ref_t m_leftmost;
	ref_t m_rightmost;

	const ref_t get_sidebottom(bool right, const ref_t& cur) const
	{
		ref_t n = cur;
		ref_t child;
		typename ref_t::locked_t lockedRef;
		for (;;)
		{
			lockedRef = n;
			for (;;)
			{
				child = lockedRef->get_child_link(right);
				if (!child)
					break;
				n = child;
				lockedRef = n;
			}
			child = lockedRef->get_child_link(!right);
			if (!child)
				break;
			n = child;
		}
		return n;
	}

	ref_t get_next_preorder_or_prev_postorder(bool preorder, const ref_t& cur) const
	{
		typename ref_t::locked_t curLocked = cur;
		bool postorder = !preorder;
		ref_t n = curLocked->get_child_link(postorder);
		if (!n)
		{
			n = curLocked->get_child_link(preorder);
			if (!n)
			{
				n = cur;
				ref_t parent = curLocked->get_parent_link();
				typename ref_t::locked_t parentResolved;
				while (!!parent)
				{
					parentResolved = parent;
					if (parentResolved->get_child_link(postorder) == n)
					{
						const ref_t& parentChild = parentResolved->get_child_link(preorder);
						if (!!parentChild)
							return parentChild;
					}
					n = parent;
					parent = parentResolved->get_parent_link();
				}
				ref_t emptyRef;
				return emptyRef;
			}
		}
		return n;
	}

	ref_t get_prev_preorder_or_next_postorder(bool preorder, const ref_t& cur) const
	{
		typename ref_t::locked_t curLocked = cur;
		const ref_t& parent = curLocked->get_parent_link();
		if (!!parent)
		{
			typename ref_t::locked_t parentResolved = parent;
			const ref_t& parentChild = parentResolved->get_child_link(!preorder);
			if ((!!parentChild) && (cur == parentResolved->get_child_link(preorder)))
				return get_sidebottom(preorder, parentChild);
		}
		return parent;
	}

	ref_t get_inorder(bool next, const ref_t& cur) const
	{
		typename ref_t::locked_t lockedRef = cur;
		const ref_t& child = lockedRef->get_child_link(next);
		if (!!child)
		{
			if (next)
				return get_leftmost(child);
			return get_rightmost(child);
		}

		ref_t parent;
		ref_t n = cur;
		for (;;)
		{
			parent = lockedRef->get_parent_link();
			if (!parent)
				break;
			lockedRef = parent;
			if (n != lockedRef->get_child_link(next))
				break;
			n = parent;
		}
		return parent;
	}

public:
	btree() { }

	btree(this_t&& t)
	{
		m_root = std::move(t.m_root);
		m_leftmost = std::move(t.m_leftmost);
		m_rightmost = std::move(t.m_rightmost);
	}

	this_t& operator=(this_t&& t)
	{
		m_root = std::move(t.m_root);
		m_leftmost = std::move(t.m_leftmost);
		m_rightmost = std::move(t.m_rightmost);
		return *this;
	}

	/// @{
	/// @brief Gets a reference to the root node
	/// @return A reference to the root node
	ref_t& get_root() { return m_root; }
	/// @brief Gets a const reference to the root node
	/// @return A const reference to the root node
	const ref_t& get_root() const { return m_root; }
	/// @}

	/// @{
	/// @brief Gets a reference to the leftmost (first) node
	/// @return A reference to the leftmost (first) node
	ref_t& get_leftmost() { return m_leftmost; }
	/// @brief Gets a const reference to the leftmost (first) node
	/// @return A const reference to the leftmost (first) node
	const ref_t& get_leftmost() const { return m_leftmost; }
	/// @brief Gets a reference to the leftmost node below a specified parent node
	/// @param cur Parent node to scan left from
	/// @return A reference to the leftmost node below a specified parent node
	const ref_t get_leftmost(const ref_t& cur) const
	{
		ref_t n = cur;
		typename ref_t::locked_t lockedRef;
		for (;;)
		{
			lockedRef = n;
			const ref_t& n_child = lockedRef->get_left_link();
			if (!n_child)
				break;
			n = n_child;
		}
		return n;
	}
	/// @}

	/// @{
	/// @brief Gets a reference to the rightmost (last) node
	/// @return A reference to the rightmost (last) node
	ref_t& get_rightmost(){ return m_rightmost; }
	/// @brief Gets a const reference to the rightmost (last) node
	/// @return A const reference to the rightmost (last) node
	const ref_t& get_rightmost() const { return m_rightmost; }
	/// @brief Gets a reference to the leftmost node below a specified parent node
	/// @param cur Parent node to scan right from
	/// @return A reference to the leftmost node below a specified parent node
	const ref_t get_rightmost(const ref_t& cur) const
	{
		ref_t n = cur;
		typename ref_t::locked_t lockedRef;
		for (;;)
		{
			lockedRef = n;
			const ref_t& n_child = lockedRef->get_right_link();
			if (!n_child)
				break;
			n = n_child;
		}
		return n;
	}
	/// @}

	/// @{
	/// @brief Sets the root node
	/// @param r Value to set root node to
	void set_root(const ref_t& r) { m_root = r; }
	/// @}

	/// @{
	/// @brief Sets the leftmost (first) node
	/// @param f Value to set leftmost (first) node to
	void set_leftmost(const ref_t& f) { m_leftmost = f; }
	/// @}

	/// @{
	/// @brief Sets the rightmost (last) node
	/// @param l Value to set rightmost (last) node to
	void set_rightmost(const ref_t& l) { m_rightmost = l; }
	/// @}


	/// @{
	/// @brief Clears the root, leftmost and rightmost values maintained by the btree.  Does not delete any elements.
	void clear()
	{
		m_root.release();
		m_leftmost.release();
		m_rightmost.release();
	}
	/// @}

	/// @{
	/// @brief Tests if the tree is empty
	/// @return True if the tree is empty
	bool is_empty() const { return !m_root; }
	/// @}

	/// @{
	/// @brief Swaps the contents of this tree with another.
	/// @param wth The tree to swap with.
	void swap(this_t& wth)
	{
		cogs::swap(m_root, wth.m_root);
		cogs::swap(m_rightmost, wth.m_rightmost);
		cogs::swap(m_leftmost, wth.m_leftmost);
	}
	/// @}

	/// @{
	/// @brief Get the first in-order node
	/// @return The first in-order node
	const ref_t& get_first_inorder() const { return m_leftmost; }
	/// @}

	/// @{
	/// @brief Get the first post-order node
	/// @return The first post-order node
	const ref_t get_first_postorder() const { return (!m_leftmost) ? m_leftmost : get_sidebottom(false, m_leftmost); }
	/// @}

	/// @{
	/// @brief Get the first pre-order node
	/// @return The first pre-order node
	const ref_t& get_first_preorder() const { return m_root; }
	/// @}

	/// @{
	/// @brief Get the last in-order node
	/// @return The last in-order node
	const ref_t& get_last_inorder() const { return m_rightmost; }
	/// @}

	/// @{
	/// @brief Get the last post-order node
	/// @return The last post-order node
	const ref_t& get_last_postorder() const { return m_root; }
	/// @}

	/// @{
	/// @brief Get the last pre-order node
	/// @return The last pre-order node
	const ref_t get_last_preorder() const { return !m_rightmost ? m_rightmost : get_sidebottom(true, m_rightmost); }
	/// @}

	/// @{
	/// @brief Get the first node, based on a constant traversal order
	/// @tparam order The traversal order
	/// @return The first node, based on the specified traversal order.
	template <btree_traversal order>
	const ref_t get_first()
	{
		if (order == btree_traversal::inorder)
			return get_first_inorder();
		else if (order == btree_traversal::preorder)
			return get_first_preorder();
		else if (order == btree_traversal::postorder)
			return get_first_postorder();
		else if (order == btree_traversal::reverse_inorder)
			return get_last_inorder();
		else if (order == btree_traversal::reverse_preorder)
			return get_last_preorder();
		else if (order == btree_traversal::reverse_postorder)
			return get_last_postorder();
	}
	/// @}

	/// @{
	/// @brief Get the last node, based on a constant traversal order
	/// @tparam order The traversal order
	/// @return The last node, based on the specified traversal order.
	template <btree_traversal order>
	const ref_t get_last()
	{
		if (order == btree_traversal::inorder)
			return get_last_inorder();
		else if (order == btree_traversal::preorder)
			return get_last_preorder();
		else if (order == btree_traversal::postorder)
			return get_last_postorder();
		else if (order == btree_traversal::reverse_inorder)
			return get_first_inorder();
		else if (order == btree_traversal::reverse_preorder)
			return get_first_preorder();
		else if (order == btree_traversal::reverse_postorder)
			return get_first_postorder();
	}
	/// @}

	/// @{
	/// @brief Get the next in-order node
	/// @param cur The node to get the next in-order node from
	/// @return The next in-order node
	ref_t get_next_inorder(const ref_t& cur) const { return get_inorder(true, cur); }
	/// @}

	/// @{
	/// @brief Get the previous in-order node
	/// @param cur The node to get the previous in-order node from
	/// @return The previous in-order node
	ref_t get_prev_inorder(const ref_t& cur) const { return get_inorder(false, cur); }
	/// @}

	/// @{
	/// @brief Get the next pre-order node
	/// @param cur The node to get the next pre-order node from
	/// @return The next pre-order node
	ref_t get_next_preorder(const ref_t& cur) const { return get_next_preorder_or_prev_postorder(true, cur); }
	/// @}

	/// @{
	/// @brief Get the previous pre-order node
	/// @param cur The node to get the previous pre-order node from
	/// @return The previous pre-order node
	ref_t get_prev_preorder(const ref_t& cur) const { return get_prev_preorder_or_next_postorder(true, cur); }
	/// @}

	/// @{
	/// @brief Get the next post-order node
	/// @param cur The node to get the next post-order node from
	/// @return The next post-order node
	ref_t get_next_postorder(const ref_t& cur) const { return get_prev_preorder_or_next_postorder(false, cur); }
	/// @}

	/// @{
	/// @brief Get the previous post-order node
	/// @param cur The node to get the previous post-order node from
	/// @return The previous post-order node
	ref_t get_prev_postorder(const ref_t& cur) const { return get_next_preorder_or_prev_postorder(false, cur); }
	/// @}

	/// @{
	/// @brief Get the next node, based on a constant traversal order
	/// @tparam order The traversal order
	/// @param cur The node to get the next node from
	/// @return The next node, based on the specified traversal order.
	template <btree_traversal order>
	ref_t get_next(const ref_t& cur) const
	{
		if (order == btree_traversal::inorder)
			return get_next_inorder(cur);
		else if (order == btree_traversal::preorder)
			return get_next_preorder(cur);
		else if (order == btree_traversal::postorder)
			return get_next_postorder(cur);
		else if (order == btree_traversal::reverse_inorder)
			return get_prev_inorder(cur);
		else if (order == btree_traversal::reverse_preorder)
			return get_prev_preorder(cur);
		else if (order == btree_traversal::reverse_postorder)
			return get_prev_postorder(cur);
	}
	/// @}

	/// @{
	/// @brief Get the previous node, based on a constant traversal order
	/// @tparam order The traversal order
	/// @param cur The node to get the previous node from
	/// @return The previous node, based on the specified traversal order.
	template <btree_traversal order>
	ref_t get_prev(const ref_t& cur) const
	{
		if (order == btree_traversal::inorder)
			return get_prev_inorder(cur);
		else if (order == btree_traversal::preorder)
			return get_prev_preorder(cur);
		else if (order == btree_traversal::postorder)
			return get_prev_postorder(cur);
		else if (order == btree_traversal::reverse_inorder)
			return get_next_inorder(cur);
		else if (order == btree_traversal::reverse_preorder)
			return get_next_preorder(cur);
		else if (order == btree_traversal::reverse_postorder)
			return get_next_postorder(cur);
	}
	/// @}
};


/// @brief Sorted binary tree insert mode
enum class sorted_btree_insert_mode
{
	/// @brief Allow multiple equal keys
	multi,// = 0,

	/// @brief Enforce unique keys, abort if already exists
	unique,// = 1,

	/// @brief Enforce unique keys, replace if already exists
	replace// = 2
};


/// @ingroup BinaryTrees
/// @brief A base class for sorted intrusive binary trees.
///
/// derived_node_t must be derived from tlink_t<>, and  include the equivalent of the following member function:
/// @code{.cpp}
/// const key_t& get_key() const;
/// @endcode
/// @tparam key_t The key type to contain
/// @tparam derived_node_t A class derived from tlink_t.
/// @tparam comparator_t A static comparator class used to compare keys.  Default: default_comparator
/// @tparam ref_type Reference type to use for links.  Default: ptr
template <typename key_t, typename derived_node_t, class comparator_t, template <typename> class ref_type = ptr>
class sorted_btree : protected btree<derived_node_t, ref_type>
{
public:
	/// @brief Alias to this type.
	typedef sorted_btree<key_t, derived_node_t, comparator_t, ref_type> this_t;

	/// @brief Alias to the node type.
	typedef derived_node_t node_t;

	/// @brief The node reference type
	typedef ref_type<node_t> ref_t;

private:
	typedef btree<derived_node_t, ref_type> base_t;

	sorted_btree(const this_t&) = delete;
	this_t& operator=(const this_t&) = delete;

	const ref_t get_rightmost() const { return base_t::get_rightmost(); }
	const ref_t get_leftmost() const { return base_t::get_leftmost(); }
	const ref_t get_rightmost(const ref_t& cur) const { return base_t::get_rightmost(cur); }
	const ref_t get_leftmost(const ref_t& cur) const { return base_t::get_leftmost(cur); }

	void set_leftmost(const ref_t& f) { base_t::set_leftmost(f); }
	void set_rightmost(const ref_t& l) { base_t::set_rightmost(l); }

	template <bool rightSideLarger>
	bool balance_remove_inner(const ref_t& nIn, ref_t& swappedWith, ref_t& liftedChild)
	{
		bool result = false;
		ref_t emptyRef;
		ref_t n = nIn;
		typename ref_t::locked_t lockedRef = n;
		bool wasRightNode = false;
		ref_t* localRoot = &liftedChild;

		swappedWith.release();

		ref_t leftChild = lockedRef->get_left_link(); // Might be null
		ref_t rightChild = lockedRef->get_right_link(); // Might be null
		ref_t parent = lockedRef->get_parent_link(); // Might be null

		typename ref_t::locked_t lockedParent;
		if (!!parent)
		{
			lockedParent = parent;
			wasRightNode = (lockedParent->get_right_link() == n);
		}

		int mode = (!!leftChild ? 1 : 0) + (!!rightChild ? 2 : 0);
		switch (mode)
		{
		case 0: // neither right nor left nodes are present
		{
			liftedChild.release();
			if (get_leftmost() == n)
				set_leftmost(parent);
			if (get_rightmost() == n)
				set_rightmost(parent);
			if (!!parent)
				result = wasRightNode;
		}
		break;
		case 1: // left node is present
		{
			result = wasRightNode;
			liftedChild = leftChild; // leftChild moves up to take its place
			typename ref_t::locked_t lockedChild = leftChild;
			lockedChild->set_parent_link(parent);
			if (get_rightmost() == n)
				set_rightmost(get_rightmost(leftChild));
		}
		break;
		case 2: // right node is present
		{
			result = wasRightNode;
			liftedChild = rightChild; // rightChild moves up to take its place
			typename ref_t::locked_t lockedChild = rightChild;
			lockedChild->set_parent_link(parent);
			if (get_leftmost() == n)
				set_leftmost(get_leftmost(rightChild));
		}
		break;
		case 3: // both are present.
		{
			const ref_t& child = rightSideLarger ? rightChild : leftChild;
			const ref_t& otherChild = rightSideLarger ? leftChild : rightChild;
			localRoot = &swappedWith;

			if (rightSideLarger) // Find node to swap with it.
				swappedWith = get_leftmost(child);
			else
				swappedWith = get_rightmost(child);
			typename ref_t::locked_t lockedSwappedWith = swappedWith;
			liftedChild = lockedSwappedWith->get_child_link(rightSideLarger);

			// Simultaneously swap 2 nodes, remove one, and lift its 1 child (if any)
			typename ref_t::locked_t lockedOtherChild = otherChild;
			lockedOtherChild->set_parent_link(swappedWith);
			lockedSwappedWith->set_child_link(!rightSideLarger, otherChild);
			if (swappedWith == child) // In case swapping with a child node
			{
				lockedRef->set_parent_link(swappedWith);
				result = rightSideLarger;
			}
			else
			{
				ref_t swappedWithParent = lockedSwappedWith->get_parent_link();
				lockedRef->set_parent_link(swappedWithParent); // just for the purpose of reporting back.  Not actually in the list anymore.
				if (!!liftedChild)
				{
					typename ref_t::locked_t lockedLiftedChild = liftedChild;
					lockedLiftedChild->set_parent_link(swappedWithParent);
				}
				typename ref_t::locked_t lockedSwappedWithParent = swappedWithParent;
				result = (lockedSwappedWithParent->get_right_link() == swappedWith);
				lockedSwappedWithParent->set_child_link(!rightSideLarger, liftedChild);
				typename ref_t::locked_t lockedChild = child;
				lockedChild->set_parent_link(swappedWith);
				lockedSwappedWith->set_child_link(rightSideLarger, child);
			}
			lockedSwappedWith->set_parent_link(parent);
		}
		break;
		};

		if (n == get_root())
			set_root(*localRoot);
		else
			lockedParent->set_child_link(wasRightNode, *localRoot);
		return result;
	}

protected:
	sorted_btree()
	{ }

	sorted_btree(this_t&& t)
		: base_t(std::move(t))
	{ }

	this_t& operator=(this_t&& t)
	{
		base_t::operator=(std::move(t));
		return *this;
	}

	/// @{
	/// @brief Get the root node
	const ref_t& get_root() const { return base_t::get_root(); }
	/// @}

	/// @{
	/// @brief Sets the root node
	/// @param r Value to set root node to
	void set_root(const ref_t& r) { base_t::set_root(r); }
	/// @}

	/// @{
	/// @brief Inserts a node into the sorted_btree.
	/// @param n The node to insert.
	/// @param insertMode The mode of the insert operation
	/// @param hint If set, must be the result of a prior call to one of the 'last_equal' or
	/// 'nearest_greater' find() functions passed the value being inserted.  This is useful
	/// if additional work is needed before insert, only if an existing match is or is not found.
	/// @return A reference to an equal node in the case of collision, or an empty node if no collision.
	ref_t insert(const ref_t& n, sorted_btree_insert_mode insertMode, const ref_t& hint = ref_t())
	{
		ref_t emptyRef;
		typename ref_t::locked_t lockedRef = n;
		lockedRef->set_left_link(emptyRef);
		lockedRef->set_right_link(emptyRef);

		// If empty
		bool hasRoot = !!get_root();
		if (!hasRoot)
		{
			lockedRef->set_parent_link(emptyRef);
			set_root(n);
			set_leftmost(n);
			set_rightmost(n);
			return emptyRef;
		}
		const key_t& key = lockedRef->get_key();

		// Prepend optimization
		typename ref_t::locked_t lockedCompareTo = get_rightmost();
		const key_t& rightmostKey = lockedCompareTo->get_key();
		bool isNotLess = !comparator_t::is_less_than(key, rightmostKey);
		if (isNotLess)
		{
			bool isEqual = !comparator_t::is_less_than(rightmostKey, key);
			if (isEqual)
			{
				if (insertMode == sorted_btree_insert_mode::unique)
					return get_rightmost();

				if (insertMode == sorted_btree_insert_mode::replace)
				{
					ref_t& parentNode = lockedCompareTo->get_parent_link();
					lockedRef->set_parent_link(parentNode);

					//Last won't have a right node, but may have a left node
					ref_t& leftNode = lockedCompareTo->get_left_link();
					if (!!leftNode)
					{
						typename ref_t::locked_t lockedLeftNode = leftNode;
						lockedLeftNode->set_parent_link(n);
					}

					lockedRef->set_left_link(leftNode);

					if (!!parentNode)
					{
						typename ref_t::locked_t lockedParentNode = parentNode;
						lockedParentNode->set_child_link(true, n);
					}
					else // if (get_rightmost() == get_root())
						set_root(n);

					return get_rightmost();
				}
			}

			lockedRef->set_parent_link(get_rightmost());
			lockedCompareTo->set_right_link(n);
			ref_t result = isEqual ? get_rightmost() : emptyRef;
			set_rightmost(n);
			return result;
		}

		// Append optimization
		lockedCompareTo = get_leftmost();
		const key_t& leftmostKey = lockedCompareTo->get_key();
		bool isLess = comparator_t::is_less_than(key, leftmostKey);
		if (isLess)
		{
			lockedRef->set_parent_link(get_leftmost());
			lockedCompareTo->set_left_link(n);
			set_leftmost(n);
			return emptyRef;
		}

		// If hint is present, we know it's a valid parent node for the node being inserted.
		ref_t compareTo = !hint ? get_root() : hint;

		bool wasLess = false;
		ref_t parent;
		for (;;)
		{
			lockedCompareTo = compareTo;
			const key_t& cmpKey = lockedCompareTo->get_key();
			isLess = comparator_t::is_less_than(key, cmpKey);

			// Check for an equal node.  If so, we're done.
			if ((insertMode != sorted_btree_insert_mode::multi) && !isLess && !comparator_t::is_less_than(cmpKey, key))
			{
				if (insertMode == sorted_btree_insert_mode::replace)
				{
					lockedRef->set_parent_link(lockedCompareTo->get_parent_link());

					ref_t& rightNode = lockedCompareTo->get_right_link();
					if (!!rightNode)
					{
						typename ref_t::locked_t lockedRightNode = rightNode;
						lockedRightNode->set_parent_link(n);
					}

					lockedRef->set_right_link(rightNode);

					ref_t& leftNode = lockedCompareTo->get_left_link();
					if (!!leftNode)
					{
						typename ref_t::locked_t lockedLeftNode = leftNode;
						lockedLeftNode->set_parent_link(n);
					}

					lockedRef->set_left_link(leftNode);

					if (!!parent)
					{
						typename ref_t::locked_t lockedParent = parent;
						lockedParent->set_child_link(!wasLess, n);
					}
					else // if (compareTo == get_root())
						set_root(n);
				}
				return compareTo;
			}
			wasLess = isLess;
			parent = compareTo;
			compareTo = lockedCompareTo->get_child_link(!isLess);
			if (!compareTo)
			{
				lockedRef->set_parent_link(parent);
				if (isLess)
					lockedCompareTo->set_left_link(n);
				else
					lockedCompareTo->set_right_link(n);
				break;
			}
		}
		return emptyRef;
	}
	/// @}

	/// @{
	/// @brief Rotate a node to the right.
	///
	/// The node will be pushed down and to the right.  Its left child, if any, takes it place.
	/// @param n The node to rotate to the right.
	/// @param updateParent If false, the parent (or the root node) are not updated to reflect the rotation.
	void rotate_right(const ref_t& n, bool updateParent = true)
	{
		typename ref_t::locked_t lockedRef = n;
		ref_t child = lockedRef->get_left_link();

		typename ref_t::locked_t lockedParent = n;
		typename ref_t::locked_t lockedChild = child;
		ref_t childChild = lockedChild->get_right_link();

		ref_t oldParentParent = lockedParent->get_parent_link();
		lockedChild->set_parent_link(oldParentParent);
		lockedChild->set_right_link(n);

		if (updateParent)
		{
			if (n == get_root())
				set_root(child);
			else
			{
				typename ref_t::locked_t lockedParentParent = oldParentParent;
				lockedParentParent->set_child_link((lockedParentParent->get_right_link() == n), child);
			}
		}

		lockedParent->set_parent_link(child);
		lockedParent->set_left_link(childChild);
		if (!!childChild)
			childChild->set_parent_link(n);
	}
	/// @}

	/// @{
	/// @brief Rotate a node to the left.
	///
	/// The node will be pushed down and to the left.  Its right child, if any, takes it place.
	/// @param n The node to rotate to the left.
	/// @param updateParent If false, the parent (or the root node) are not updated to reflect the rotation.
	void rotate_left(const ref_t& n, bool updateParent = true)
	{
		typename ref_t::locked_t lockedRef = n;
		ref_t child = lockedRef->get_right_link();

		typename ref_t::locked_t lockedParent = n;
		typename ref_t::locked_t lockedChild = child;
		ref_t childChild = lockedChild->get_left_link();

		ref_t oldParentParent = lockedParent->get_parent_link();
		lockedChild->set_parent_link(oldParentParent);
		lockedChild->set_left_link(n);

		if (updateParent)
		{
			if (n == get_root())
				set_root(child);
			else
			{
				typename ref_t::locked_t lockedParentParent = oldParentParent;
				lockedParentParent->set_child_link((lockedParentParent->get_right_link() == n), child);
			}
		}

		lockedParent->set_parent_link(child);
		lockedParent->set_right_link(childChild);
		if (!!childChild)
			childChild->set_parent_link(n);
	}
	/// @}

	/// @{
	/// @brief Remove a node, and begin the process of rebalancing the tree.
	///
	/// Removing a node with only 1 child, the operation is fairly simple; Move the child up into its place.
	/// Removing a node with 2 children is only slightly more complicated.  As a rule for sorted binary trees, any
	/// node with 2 children has a next (and previous) in-order node that has 0 or 1 children (i.e. in-order next
	/// may only have a right link, and in-order prev may only have a left link).  The removing and in-order
	/// adjacent nodes can simply be swapped.  The node to be removed then has 0 or 1 child, and can be easily removed.
	///
	/// In order to update balancing information (red/black or avl), we need to return the node swapped with, and
	/// the single child node swapped into place after removal.  The caller needs to handle the following scenarios:
	///
	/// - If swappedWith is null, and liftedChild is null, the removed node had no children.
	///     The removed node will retain a link to its original parent, which the caller may need for repaint.
	///
	/// - If swappedWith is null, and liftedChild is not null, the removed node had 1 child, which was moved into its place.
	///     The removed node will retain a link to its original parent, which the caller may need for repaint.
	///
	/// - If swappedWith is not null, and liftedChild is null, the removed node had 2 children, and was swapped with
	///     a node with 0 children.  The removed node will retain a pointer to parent of the node it was swapped with,
	///     which the caller may need for repainting.
	///
	/// - If swappedWith is not null, and liftedChild is not null, the removed node had 2 children, and was swapped with
	///     a node with 1 child.  The removed node will retain a pointer to the parent of the node it was swapped with.
	///
	/// @returns True if the removed node (after swapped) was its parent's right node.
	bool balance_remove(const ref_t& n, ref_t& swappedWith, ref_t& liftedChild, bool rightSideLarger)
	{
		if (rightSideLarger)
			return balance_remove_inner<true>(n, swappedWith, liftedChild);
		return balance_remove_inner<false>(n, swappedWith, liftedChild);
	}
	/// @}

	/// @{
	/// @brief Swaps the contents of this tree with another.
	/// @param srcDst The tree to swap with.
	void swap(this_t& wth) { base_t::swap(wth); }
	/// @}

public:
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
	const ref_t& get_first() const { return base_t::get_first_inorder(); }
	/// @}

	/// @{
	/// @brief Get the last in-order node
	/// @return The last in-order node
	const ref_t& get_last() const { return base_t::get_last_inorder(); }
	/// @}

	/// @{
	/// @brief Get the next in-order node
	/// @param cur The node to get the next in-order node from
	/// @return The next in-order node
	ref_t get_next(const ref_t& cur) const { return base_t::get_next_inorder(cur); }
	/// @}

	/// @{
	/// @brief Get the previous in-order node
	/// @param cur The node to get the previous in-order node from
	/// @return The previous in-order node
	ref_t get_prev(const ref_t& cur) const { return base_t::get_prev_inorder(cur); }
	/// @}

	/// @{
	/// @brief Get the first post-order node
	/// @return The first post-order node
	const ref_t get_first_postorder() const { return base_t::get_first_postorder(); }
	/// @}

	/// @{
	/// @brief Get the last post-order node
	/// @return The last post-order node
	const ref_t& get_last_postorder() const { return base_t::get_last_postorder(); }
	/// @}

	/// @{
	/// @brief Get the next post-order node
	/// @param cur The node to get the next post-order node from
	/// @return The next post-order node
	ref_t get_next_postorder(const ref_t& cur) const { return base_t::get_next_postorder(cur); }
	/// @}

	/// @{
	/// @brief Get the previous post-order node
	/// @param cur The node to get the previous post-order node from
	/// @return The previous post-order node
	ref_t get_prev_postorder(const ref_t& cur) const { return base_t::get_prev_postorder(cur); }
	/// @}

	/// @{
	/// @brief Find any node matching the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_any_equal(const key_t& criteria) const
	{
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
				break;
		}
		return n;
	}
	/// @}

	/// @{
	/// @brief Find the first in-order node matching the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_first_equal(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else
			{
				lastFound = n;
				// Once we've found an equal node, and go left, all nodes
				// are either smaller or equal to criteria. No need to check
				// if criteria is less than key.
				n = lockedRef->get_left_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(lockedRef->get_key(), criteria))
						n = lockedRef->get_right_link();
					else // they are equal
					{
						lastFound = n;
						n = lockedRef->get_left_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the last in-order node matching the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_last_equal(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
			{
				lastFound = n;
				// Once we've found an equal node, and go right, all nodes
				// are either greater or equal to criteria. No need to check
				// if key is less than criteria.
				n = lockedRef->get_right_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(criteria, lockedRef->get_key()))
						n = lockedRef->get_left_link();
					else // they are equal
					{
						lastFound = n;
						n = lockedRef->get_right_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the nearest node less than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_nearest_less_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(key, criteria))
			{
				lastFound = n;
				n = lockedRef->get_right_link();
			}
			else if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else
			{
				// Once we've found an equal node, and go left, all nodes
				// are either smaller or equal to criteria. No need to check
				// if criteria is less than key.
				n = lockedRef->get_left_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(lockedRef->get_key(), criteria))
					{
						lastFound = n;
						n = lockedRef->get_right_link();
					}
					else // they are equal
						n = lockedRef->get_left_link();
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the nearest node greater than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_nearest_greater_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
			{
				lastFound = n;
				n = lockedRef->get_left_link();
			}
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
			{
				// Once we've found an equal node, and go right, all nodes
				// are either greater or equal to criteria. No need to check
				// if key is less than criteria.
				n = lockedRef->get_right_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(criteria, lockedRef->get_key()))
					{
						lastFound = n;
						n = lockedRef->get_left_link();
					}
					else // they are equal
						n = lockedRef->get_right_link();
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find any node matching, or the nearest node less than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_any_equal_or_nearest_less_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t lastLesser;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(key, criteria))
			{
				lastLesser = n; // lesser
				n = lockedRef->get_right_link();
			}
			else if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else
			{
				lastFound = n; // equal
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find any node matching, or the nearest node greater than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_any_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
			{
				lastFound = n; // greater
				n = lockedRef->get_left_link();
			}
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
			{
				lastFound = n; // equal
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the first in-order node matching, or the nearest node less than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_first_equal_or_nearest_less_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t lastLesser;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(key, criteria))
			{
				lastLesser = n; // lesser
				n = lockedRef->get_right_link();
			}
			else if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else
			{
				lastFound = n; // equal
				// Once we've found an equal node, and go left, all nodes
				// are either smaller or equal to criteria. No need to check
				// if criteria is less than key.
				n = lockedRef->get_left_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(lockedRef->get_key(), criteria))
						n = lockedRef->get_right_link();
					else
					{
						lastFound = n; // equal
						n = lockedRef->get_left_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the first in-order node matching, or the nearest node greater than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_first_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
			{
				lastFound = n; // greater
				n = lockedRef->get_left_link();
			}
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
			{
				lastFound = n; // equal
				// Once we've found an equal node, and go left, all nodes
				// are either lesser or equal to criteria. No need to check
				// if criteria is less than key.
				n = lockedRef->get_left_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(lockedRef->get_key(), criteria))
						n = lockedRef->get_right_link();
					else // they are equal
					{
						lastFound = n; // equal
						n = lockedRef->get_left_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the last in-order node matching, or the nearest node less than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_last_equal_or_nearest_less_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t lastLesser;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(key, criteria))
			{
				lastLesser = n; // lesser
				n = lockedRef->get_right_link();
			}
			else if (comparator_t::is_less_than(criteria, key))
				n = lockedRef->get_left_link();
			else
			{
				lastFound = n; // equal
				// Once we've found an equal node, and go right, all nodes
				// are either greater or equal to criteria. No need to check
				// if key is less than criteria.
				n = lockedRef->get_right_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(criteria, lockedRef->get_key()))
						n = lockedRef->get_left_link();
					else // they are equal
					{
						lastFound = n; // equal
						n = lockedRef->get_right_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}

	/// @{
	/// @brief Find the last in-order node matching, or the nearest node greater than the criteria value
	/// @param criteria The value to match
	/// @return The node found, or empty if not found.
	ref_t find_last_equal_or_nearest_greater_than(const key_t& criteria) const
	{
		ref_t lastFound;
		ref_t n = get_root();
		typename ref_t::locked_t lockedRef;
		while (!!n)
		{
			lockedRef = n;
			const key_t& key = lockedRef->get_key();
			if (comparator_t::is_less_than(criteria, key))
			{
				lastFound = n; // greater
				n = lockedRef->get_left_link();
			}
			else if (comparator_t::is_less_than(key, criteria))
				n = lockedRef->get_right_link();
			else
			{
				lastFound = n; // equal
				// Once we've found an equal node, and go right, all nodes
				// are either greater or equal to criteria. No need to check
				// if key is less than criteria.
				n = lockedRef->get_right_link();
				while (!!n)
				{
					lockedRef = n;
					if (comparator_t::is_less_than(criteria, lockedRef->get_key()))
						n = lockedRef->get_left_link();
					else // they are equal
					{
						lastFound = n; // equal
						n = lockedRef->get_right_link();
					}
				}
				break;
			}
		}
		return lastFound;
	}
	/// @}
};


}


#endif
