//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress, NeedsToBeSplitUp


#ifndef COGS_HEADER_GEOMETRY_SIZING_GROUPS
#define COGS_HEADER_GEOMETRY_SIZING_GROUPS


#include "cogs/collections/multimap.hpp"
#include "cogs/collections/dlink.hpp"
#include "cogs/collections/dlist.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/collections/rbtree.hpp"
#include "cogs/geometry/proportion.hpp"
#include "cogs/geometry/range.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace geometry {


// sizing_group is a utility class for rects with subrects arranged in rows or columns, which need to grow
// or shrink according to a set of rules.

enum class sizing_policy
{
	// The default proportions of nodes are retained
	proportional,

	// An equal amount of space is added to or removed from each node
	equal,

	// Smaller nodes are given additional space first.  Larger nodes have space removed first.
	equitable
};


template <sizing_policy policy = sizing_policy::proportional, bool consider_range = false>
class sizing_group
{
};


template <>
class sizing_group<sizing_policy::proportional, false>
{
public:
	class cell : public dlink_t<cell>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_default;
		double m_sizedLength;

	public:
		double get_sized_length() const { return m_sizedLength; }
	};

private:
	dlist_t<cell> m_list;
	size_t m_count = 0;
	double m_default = 0.0;

public:
	double get_default() const { return m_default; }

	void clear()
	{
		m_list.clear();
		m_count = 0;
		m_default = 0.0;
	}

	void add_cell(cell& c, double defaultLength)
	{
		c.m_default = defaultLength;
		m_default += defaultLength;
		m_list.append(&c);
		++m_count;
	}

	void remove_cell(cell& c)
	{
		m_default -= c.m_default;
		m_list.remove(&c);
		--m_count;
	}

	double calculate_sizes(double proposedSize) const
	{
		ptr<cell> c = m_list.get_first();
		if (!c)
			return 0;
		COGS_ASSERT(m_count > 0);
		if (proposedSize == m_default)
		{
			do {
				c->m_sizedLength = c->m_default;
				c = c->get_next_link();
			} while (!!c);
		}
		else
		{
			double remainingProposed = proposedSize;
			double remainingDefault = m_default;
			do {
				double proportion = c->m_default / remainingDefault;
				c->m_sizedLength = remainingProposed * proportion;
				remainingProposed -= c->m_sizedLength;
				remainingDefault -= c->m_default;
				c = c->get_next_link();
			} while (!!c);
		}
		return proposedSize;
	}
};


template <>
class sizing_group<sizing_policy::equal, false>
{
public:
	class cell : public dlink_t<cell>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_default;
		double m_sizedLength;

	public:
		double get_sized_length() const { return m_sizedLength; }
	};

private:
	dlist_t<cell> m_list;
	size_t m_count = 0;
	double m_default = 0.0;

public:
	double get_default() const { return m_default; }

	void clear()
	{
		m_list.clear();
		m_count = 0;
		m_default = 0.0;
	}

	void add_cell(cell& c, double defaultLength)
	{
		c.m_default = defaultLength;
		m_default += defaultLength;
		m_list.append(&c);
		++m_count;
	}

	void remove_cell(cell& c)
	{
		m_default -= c.m_default;
		m_list.remove(&c);
		--m_count;
	}

	double calculate_sizes(double proposedSize) const
	{
		ptr<cell> c = m_list.get_first();
		if (!c)
			return 0;
		COGS_ASSERT(m_count > 0);
		if (proposedSize == m_default)
		{
			do {
				c->m_sizedLength = c->m_default;
				c = c->get_next_link();
			} while (!!c);
		}
		else
		{
			size_t n = m_count;
			double remaining = proposedSize - m_default;
			do {
				double d = remaining / n;
				remaining -= d;
				c->m_sizedLength = c->m_default + d;
				--n;
				c = c->get_next_link();
			} while (!!c);
		}
		return proposedSize;
	}
};


template <>
class sizing_group<sizing_policy::equitable, false>
{
public:
	class cell : public rbtree_node_t<cell>
	{
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_default;
		double m_sizedLength;

	public:
		double get_key() const { return m_default; }

		double get_sized_length() const { return m_sizedLength; }
	};

private:
	rbtree<double, cell> m_sorted;
	double m_default = 0.0;

	template <bool grow>
	void calculate_size2(double proposedSize) const
	{
		double remaining = proposedSize - m_default;
		ptr<cell> c = grow ? m_sorted.get_first() : m_sorted.get_last();
		double defaultLength = c->m_default;
		size_t n = 1;
		for (;;)
		{
			remaining += defaultLength;
			ptr<cell> next = grow ? c->get_next() : c->get_prev();
			if (!!next)
			{
				double nextDefaultLength = next->m_default;
				if (defaultLength == nextDefaultLength)
				{
					++n;
					c = next;
					continue;
				}
				if ((grow && (remaining > n * nextDefaultLength)) || (!grow && (remaining < n * nextDefaultLength)))
				{
					defaultLength = nextDefaultLength;
					c = next;
					continue;
				}
			}
			c = grow ? m_sorted.get_first() : m_sorted.get_last();
			do {
				double d = remaining / n;
				c->m_sizedLength = d;
				remaining -= d;
				--n;
				c = grow ? c->get_next() : c->get_prev();
			} while (c != next);
			break;
		}
	}

public:
	double get_default() const { return m_default; }

	void clear()
	{
		m_sorted.clear();
		m_default = 0.0;
	}

	void add_cell(cell& c, double defaultLength)
	{
		c.m_default = defaultLength;
		m_default += defaultLength;
		m_sorted.insert_multi(&c);
	}

	void remove_cell(cell& c)
	{
		m_default -= c.m_default;
		m_sorted.remove(&c);
	}

	void calculate_sizes(double proposedSize) const
	{
		ptr<cell> c = m_sorted.get_last();
		if (!c)
			return;
		if (proposedSize == m_default)
		{
			do {
				c->m_sizedLength = c->m_default;
				c = c->get_next();
			} while (!!c);
		}
		else if (proposedSize > m_default)
			calculate_size2<true>(proposedSize);
		else
			calculate_size2<false>(proposedSize);
	}
};


template <>
class sizing_group<sizing_policy::proportional, true>
{
private:
	class min_node : public rbtree_node_t<min_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_minProportions;

	public:
		double get_key() const { return m_minProportions; }
	};

	class max_node : public rbtree_node_t<max_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_maxProportions;

	public:
		double get_key() const { return m_maxProportions; }
	};

public:
	class cell : public min_node, public max_node
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		linear::range m_range;
		double m_default;
		double m_sizedLength;

	public:
		double get_sized_length() const { return m_sizedLength; }
	};

private:
	linear::range m_range;
	double m_default = 0.0;
	rbtree<double, min_node> m_minProportions;
	rbtree<double, max_node> m_maxProportions;

public:
	const linear::range& get_range() const { return m_range; }
	double get_default() const { return m_default; }

	sizing_group()
	{
		m_range.set_fixed();
	}

	void clear()
	{
		m_default = 0;
		m_range.set_fixed();
		m_minProportions.clear();
		m_maxProportions.clear();
	}

	void add_cell(cell& c, double defaultLength, const linear::range& range)
	{
		c.m_default = defaultLength;
		c.m_range = range;
		if (defaultLength <= 0)
			c.m_sizedLength = 0;
		else
		{
			m_default += defaultLength;
			m_range += range;
			COGS_ASSERT(m_range.contains(defaultLength));
			c.m_maxProportions = c.m_range.has_max() ? (c.m_range.get_max() / defaultLength) : 0;
			c.m_minProportions = c.m_range.get_min() / defaultLength;
			m_maxProportions.insert_multi(&c);
			m_minProportions.insert_multi(&c);
		}
	}

	void remove_cell(cell& c)
	{
		if (c.m_default > 0)
		{
			m_default -= c.m_default;
			m_minProportions.remove(&c);
			m_maxProportions.remove(&c);
		}
	}

	double calculate_sizes(double proposedSize)
	{
		ptr<cell> c = m_minProportions.get_last().static_cast_to<cell>();
		if (!c)
			return 0;
		if (proposedSize == m_default) // size to default
		{
			do {
				c->m_sizedLength = c->m_default;
				c = c->min_node::get_prev().static_cast_to<cell>();
			} while (!!c);
			return m_default;
		}
		if (proposedSize <= m_range.get_min()) // size to min
		{
			do {
				c->m_sizedLength = c->m_range.get_min();
				c = c->min_node::get_prev().static_cast_to<cell>();
			} while (!!c);
			return m_range.get_min();
		}
		double remaining = proposedSize;
		double remainingDefault = m_default;
		if (proposedSize < m_default) // shrink proportionally to default size
		{
			do {
				double newSize;
				double targetProportion = remaining / remainingDefault;
				ptr<cell> next = c->min_node::get_prev().static_cast_to<cell>();
				if (targetProportion < c->m_minProportions)
					newSize = (c->m_range.get_min());
				else
					newSize = !next ? remaining : (c->m_default * targetProportion);
				remaining -= newSize;
				remainingDefault -= c->m_default;
				c->m_sizedLength = newSize;
				c = next;
			} while (!!c);
			return proposedSize - remaining;
		}
		//else // if (proposedSize > m_default) // grow proportionally to default size
		c = m_maxProportions.get_last().static_cast_to<cell>();
		do {
			double newSize;
			double targetProportion = remaining / remainingDefault;
			ptr<cell> next = c->max_node::get_prev().static_cast_to<cell>();
			if (c->m_range.has_max() && (targetProportion > c->m_maxProportions))
				newSize = (c->m_range.get_max());
			else
				newSize = !next ? remaining : (c->m_default * targetProportion);
			remaining -= newSize;
			remainingDefault -= c->m_default;
			c->m_sizedLength = newSize;
			c = next;
		} while (!!c);
		return proposedSize - remaining;
	}
};


template <>
class sizing_group<sizing_policy::equal, true>
{
private:
	class min_node : public rbtree_node_t<min_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_minDefaultGap;

	public:
		double get_key() const { return m_minDefaultGap; }
	};

	class max_node : public rbtree_node_t<max_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		linear::range m_defaultMaxRange;

	public:
		const linear::range& get_key() const { return m_defaultMaxRange; }
	};


public:
	class cell : public min_node, public max_node
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		linear::range m_range;
		double m_default;
		double m_sizedLength;

	public:
		double get_sized_length() const { return m_sizedLength; }
	};

private:
	linear::range m_range;
	double m_default = 0.0;
	rbtree<double, min_node> m_sortedMin;
	rbtree<linear::range, max_node> m_sortedMax;
	size_t m_count = 0;

public:
	const linear::range& get_range() const { return m_range; }
	double get_default() const { return m_default; }

	sizing_group()
	{
		m_range.set_fixed();
	}

	void clear()
	{
		m_default = 0.0;
		m_range.set_fixed();
		m_sortedMin.clear();
		m_sortedMax.clear();
		m_count = 0;
	}

	void add_cell(cell& c, double defaultLength, const linear::range& range)
	{
		c.m_default = defaultLength;
		c.m_range = range;
		c.m_defaultMaxRange = linear::range(defaultLength, range.get_max(), range.has_max());
		c.m_minDefaultGap = defaultLength - range.get_min();
		m_sortedMax.insert_multi(&c);
		m_sortedMin.insert_multi(&c);
		++m_count;
	}

	void remove_cell(cell& c)
	{
		m_default -= c.m_default;
		m_sortedMin.remove(&c);
		m_sortedMax.remove(&c);
		--m_count;
	}

	double calculate_sizes(double proposedSize)
	{
		size_t n = m_count;
		if (!n)
			return 0;
		if (proposedSize == m_default) // use default size
		{
			// doesn't matter which map we use to iterate through all cells
			ptr<cell> c = m_sortedMin.get_first().static_cast_to<cell>();
			do
			{
				c->m_sizedLength = c->m_default;
				c = c->min_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_default;
		}
		if (proposedSize <= m_range.get_min()) // use min size
		{
			ptr<cell> c = m_sortedMin.get_first().static_cast_to<cell>();
			do
			{
				c->m_sizedLength = c->m_range.get_min();
				c = c->min_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_range.get_min();
		}
		if (m_range.has_max() && (m_range.get_max() <= proposedSize)) // use max size
		{
			ptr<cell> c = m_sortedMin.get_first().static_cast_to<cell>();
			do
			{
				c->m_sizedLength = c->m_range.get_max();
				c = c->min_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_range.get_min();
		}
		if (proposedSize > m_default) // stretching beyond default
		{
			double growBy = proposedSize;
			growBy -= m_default;
			ptr<cell> c = m_sortedMax.get_first().static_cast_to<cell>();
			do
			{
				double targetGrowBy = growBy;
				targetGrowBy /= n;
				--n;
				double targetSize(c->m_default);
				targetSize += targetGrowBy;
				if (c->m_range.has_max() && (targetSize >= c->m_range.get_max()))
				{
					c->m_sizedLength = c->m_range.get_max();
					growBy += c->m_default;
					growBy -= c->m_range.get_max();
				}
				else
				{
					for (;;)
					{
						c->m_sizedLength = targetSize;
						growBy -= targetGrowBy;
						c = c->max_node::get_next().static_cast_to<cell>();
						if (!c)
							break;
						targetGrowBy = growBy;
						targetGrowBy /= n;
						--n;
						targetSize = c->m_default;
						targetSize += targetGrowBy;
					}
					break;
				}
				c = c->max_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_default;
		}
		double shrinkBy = m_default;
		shrinkBy -= proposedSize;
		ptr<cell> c = m_sortedMin.get_first().static_cast_to<cell>();
		do {
			double targetShrinkBy = shrinkBy;
			targetShrinkBy /= n;
			--n;
			if (targetShrinkBy >= c->m_minDefaultGap)
			{
				c->m_sizedLength = c->m_range.get_min();
				shrinkBy += c->m_range.get_min();
				shrinkBy -= c->m_default;
			}
			else
			{
				for (;;)
				{
					c->m_sizedLength = c->m_default;
					c->m_sizedLength -= targetShrinkBy;
					shrinkBy -= targetShrinkBy;
					c = c->min_node::get_next().static_cast_to<cell>();
					if (!c)
						break;
					targetShrinkBy = shrinkBy;
					targetShrinkBy /= n;
					--n;
				}
				break;
			}
			c = c->min_node::get_next().static_cast_to<cell>();
		} while (!!c);
		return proposedSize;
	}
};


template <>
class sizing_group<sizing_policy::equitable, true>
{
private:
	class cell_base
	{
	protected:
		template <sizing_policy, bool>
		friend class sizing_group;

		linear::range m_range;
		double m_default;
	};

	class min_node : public rbtree_node_t<min_node>, public virtual cell_base
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

	public:
		const linear::range& get_key() const { return m_range; }
	};

	class max_node : public rbtree_node_t<max_node>, public virtual cell_base
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

	public:
		const linear::range& get_key() const { return m_range; }
	};

	class default_node : public rbtree_node_t<default_node>, public virtual cell_base
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

	public:
		double get_key() const { return m_default; }
	};

	class low_node : public rbtree_node_t<low_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_low;

	public:
		double get_key() const { return m_low; }
	};

	class high_node : public rbtree_node_t<high_node>
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_high;

	public:
		double get_key() const { return m_high; }
	};

public:
	class cell : public min_node, public max_node, public default_node, public high_node, public low_node
	{
	private:
		template <sizing_policy, bool>
		friend class sizing_group;

		double m_sizedLength;

		double m_high;
		double m_low;

		size_t m_position;
		size_t m_reversePosition;

	public:
		double get_sized_length() const { return m_sizedLength; }
	};

private:
	linear::range m_range;
	double m_default = 0.0;

	rbtree<linear::range, min_node, typename linear::range::minimum_comparator> m_sortedMin;
	rbtree<linear::range, max_node, typename linear::range::maximum_comparator> m_sortedMax;
	rbtree<double, default_node> m_sortedDefault;
	rbtree<double, low_node> m_sortedLow;
	rbtree<double, high_node> m_sortedHigh;
	bool m_needsUpdate = false;

public:
	const linear::range& get_range() const { return m_range; }
	double get_default() const { return m_default; }

	sizing_group()
	{
		m_range.set_fixed();
	}

	void clear()
	{
		m_default = 0;
		m_range.set_fixed();
		m_sortedMin.clear();
		m_sortedMax.clear();
		m_sortedDefault.clear();
		m_needsUpdate = false;
	}

	void add_cell(cell& c, double defaultLength, const linear::range& range)
	{
		COGS_ASSERT(range.contains(defaultLength));
		c.m_default = defaultLength;
		c.m_range = range;
		m_default += defaultLength;
		m_range += range;
		m_sortedDefault.insert_multi(&c);
		m_sortedMin.insert_multi(&c);
		m_sortedMax.insert_multi(&c);
		m_sortedLow.clear();
		m_sortedHigh.clear();
		m_needsUpdate = true;
	}

	double calculate_sizes(double proposedSize)
	{
		ptr<cell> c = m_sortedDefault.get_first().static_cast_to<cell>();
		if (!c)
			return 0;
		if (proposedSize == m_default) // use default size
		{
			// doesn't matter which map we use to iterate through all cells
			do {
				c->m_sizedLength = c->m_default;
				c = c->default_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_default;
		}
		if (proposedSize <= m_range.get_min()) // use min size
		{
			// doesn't matter which map we use to iterate through all cells
			do {
				c->m_sizedLength = c->m_range.get_min();
				c = c->default_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_range.get_min();
		}
		if (m_range.has_max() && (m_range.get_max() <= proposedSize)) // use max size
		{
			// doesn't matter which map we use to iterate through all cells
			do {
				c->m_sizedLength = c->m_range.get_max();
				c = c->default_node::get_next().static_cast_to<cell>();
			} while (!!c);
			return m_range.get_max();
		}

		if (m_needsUpdate)
		{
			size_t cellPosition = 0;
			double prevSize = 0;
			double numElementsBefore = 0;
			double prevHighWaterTotal = 0;
			ptr<cell> maxCell = m_sortedMax.get_first().static_cast_to<cell>();
			do {
				double additionalSpace = 0;
				if (numElementsBefore > 0)
				{
					additionalSpace = c->m_default;
					additionalSpace -= prevSize;
					additionalSpace *= numElementsBefore;
					do {
						if (!maxCell->m_range.has_max())
							break;
						double max = maxCell->m_range.get_max();
						if (c->m_default < max)
							break;
						double gap = c->m_default;
						gap -= max;
						if (gap >= additionalSpace)
							additionalSpace = 0;
						else
							additionalSpace -= gap;
						--numElementsBefore;
						maxCell = maxCell->max_node::get_next().static_cast_to<cell>();
					} while (!!additionalSpace);
				}
				c->m_position = ++cellPosition;
				prevHighWaterTotal += additionalSpace;
				c->m_high = prevHighWaterTotal;
				m_sortedHigh.insert_multi(c);
				prevSize = c->m_default;
				++numElementsBefore;
				c = c->default_node::get_next().static_cast_to<cell>();
			} while (!!c);

			cellPosition = 0;
			prevSize = 0;
			numElementsBefore = 0;
			double prevLowWaterTotal = 0;
			c = m_sortedDefault.get_last().static_cast_to<cell>();
			ptr<cell> minCell = m_sortedMin.get_last().static_cast_to<cell>();
			do {
				double additionalSpace = 0;
				if (numElementsBefore > 0)
				{
					additionalSpace = prevSize;
					additionalSpace -= c->m_default;
					additionalSpace *= numElementsBefore;
					do {
						double min = minCell->m_range.get_min();
						if (c->m_default > min)
							break;
						double gap = min;
						gap -= c->m_default;
						if (gap >= additionalSpace)
							additionalSpace = 0;
						else
							additionalSpace -= gap;
						--numElementsBefore;
						minCell = minCell->min_node::get_prev().static_cast_to<cell>();
					} while (!!additionalSpace);
				}
				c->m_reversePosition = ++cellPosition;
				prevLowWaterTotal += additionalSpace;
				c->m_low = prevLowWaterTotal;
				m_sortedLow.insert_multi(c);
				prevSize = c->m_default;
				++numElementsBefore;
				c = c->default_node::get_prev().static_cast_to<cell>();
			} while (!!c);
			m_needsUpdate = false;
		}

		double remaining = proposedSize;
		if (proposedSize > m_default) // stretching beyond default
		{
			double growBy = proposedSize;
			growBy -= m_default;

			c = m_sortedHigh.find_first_equal_or_nearest_greater_than(growBy).static_cast_to<cell>();
			if (!c)
				c = m_sortedHigh.get_last().static_cast_to<cell>();
			else
			{
				ptr<cell> c2 = c;
				c = c->high_node::get_prev().static_cast_to<cell>();
				COGS_ASSERT(!!c);
				do {
					c2->m_sizedLength = c2->m_default;
					remaining -= c2->m_default;
					c2 = c2->high_node::get_next().static_cast_to<cell>();
				} while (!!c2);
			}
			size_t numLeft = c->m_position;
			ptr<cell> maxCell = m_sortedMax.get_first().static_cast_to<cell>();
			do {
				const linear::range& r = maxCell->m_range;
				if (!r.has_max())
					break;
				auto targetSize = (remaining / numLeft);
				if (targetSize <= r.get_max())
					break;
				maxCell->m_sizedLength = r.get_max();
				remaining -= r.get_max();
				--numLeft;
				maxCell = maxCell->max_node::get_next().static_cast_to<cell>();
			} while (!!maxCell);
			do {
				double targetSize = remaining;
				targetSize /= numLeft;
				if (!c->m_range.has_max() || (c->m_range.get_max() > targetSize))
				{
					c->m_sizedLength = targetSize;
					if (!--numLeft)
						break;
					remaining -= targetSize;
				}
				c = c->high_node::get_prev().static_cast_to<cell>();
			} while (!!c);
			return proposedSize;
		}
		//else // if (proposedSize < m_default)
		double shrinkBy = m_default;
		shrinkBy -= proposedSize;
		c = m_sortedLow.find_first_equal_or_nearest_greater_than(shrinkBy).static_cast_to<cell>();
		if (!c)
			c = m_sortedLow.get_last().static_cast_to<cell>();
		else
		{
			ptr<cell> c2 = c;
			c = c->low_node::get_prev().static_cast_to<cell>();
			COGS_ASSERT(!!c);
			do {
				c2->m_sizedLength = c2->m_default;
				remaining -= c2->m_default;
				c2 = c2->low_node::get_next().static_cast_to<cell>();
			} while (!!c2);
		}
		size_t numLeft = c->m_reversePosition;
		ptr<cell> minCell = m_sortedMax.get_first().static_cast_to<cell>();
		do {
			const linear::range& r = minCell->m_range;
			if (r.get_min() == 0)
				break;
			auto targetSize = (remaining / numLeft);
			if (targetSize >= r.get_min())
				break;
			minCell->m_sizedLength = r.get_min();
			remaining -= r.get_min();
			--numLeft;
			minCell = minCell->min_node::get_prev().static_cast_to<cell>();
		} while (!!minCell);
		do {
			double targetSize = remaining;
			targetSize /= numLeft;
			if (targetSize <= c->m_range.get_min())
				targetSize = c->m_range.get_min();
			c->m_sizedLength = targetSize;
			if (!--numLeft)
				break;
			remaining -= targetSize;
			c = c->high_node::get_prev().static_cast_to<cell>();
		} while (!!c);
		return proposedSize;
	}
};

}
}


#endif
