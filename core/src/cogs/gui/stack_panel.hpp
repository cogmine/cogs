//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: WorkInProgress

#ifndef COGS_HEADER_GUI_STACK_PANEL
#define COGS_HEADER_GUI_STACK_PANEL

#include "cogs/gui/pane.hpp"

namespace cogs {
namespace gui {

// This implementation of stack_panel is a compromise. Fully supporting all possible sizing behaviors
// for all elements is extremely difficult.

// A fixed_default_size_stack_panel uses default sizes for all children
template <gfx::dimension orientation = gfx::dimension::vertical, bool stretch_other_dimension = false>
class fixed_default_size_stack_panel : public pane, public virtual pane_container
{
private:
	class subpane : public container_pane
	{
	public:
		rcref<override_bounds_frame> m_frame;

		subpane(const rcref<pane>& child, rcref<override_bounds_frame>&& f)
			: container_pane(
				container_pane::options{
					.frames{f},
					.children{child}
				}
			),
			m_frame(std::move(f))
		{
		}
	};

	size m_calculatedDefaultSize;
	range m_calculatedRange;

	void install_detach_handler(const rcref<pane>& child)
	{
		child->get_detach_event() += [](const rcref<pane>& p)
		{
			rcptr<pane> containerPane = p->get_parent();
			if (!!containerPane)
				containerPane->detach();
		};
	}

	virtual void calculate_range()
	{
		pane::calculate_range();
		const pane_list& children = get_children();
		if (children.is_empty())
		{
			m_calculatedDefaultSize.clear();
			m_calculatedRange.set_invalid();
			return;
		}
		double total = 0;
		double largestSecondary = 0;
		bool anyWithSize = false;
		for (auto& child : children)
		{
			subpane& p = *(child.static_cast_to<subpane>());
			override_bounds_frame& f = *p.m_frame;
			f.set_position(orientation, total);
			std::optional<size> defaultSize = p.get_default_size();
			if (!defaultSize.has_value())
				f.set_fixed_size(size{ 0, 0 });
			else
			{
				anyWithSize = true;
				size& sz = *defaultSize;
				total += sz[orientation];
				f.set_fixed_size(sz);
				if (largestSecondary < sz[!orientation])
					largestSecondary = sz[!orientation];
			}
		}

		if (!anyWithSize)
		{
			m_calculatedDefaultSize.clear();
			m_calculatedRange.set_invalid();
			return;
		}

		// Now that we've determined the largest other dimension, we may need
		// to adjust elements that reduce their primary length at that secondary length.
		if constexpr (stretch_other_dimension)
		{
			for (auto& child : children)
			{
				subpane& p = *(child.static_cast_to<subpane>());
				override_bounds_frame& f = *p.m_frame;
				if (f.get_fixed_size(!orientation) < largestSecondary)
				{
					// This is larger than the default size, so we know there is a valid result that is lesser in both dimensions.
					size newSize;
					newSize[orientation] = f.get_fixed_size(orientation);
					newSize[!orientation] = largestSecondary;
					collaborative_sizes tempResult = p.calculate_collaborative_sizes(newSize, range::make_unbounded(), *quadrant_flag::lesser_x_lesser_y);
					const size& resultSize = *tempResult.sizes[0][0];
					total -= (f.get_fixed_size(orientation) - resultSize[orientation]);
					f.set_fixed_size(resultSize);
				}
			}
		}

		m_calculatedDefaultSize.set(orientation, total);
		m_calculatedDefaultSize.set(!orientation, largestSecondary);
		m_calculatedRange.set_fixed(m_calculatedDefaultSize);
	}

	static rcref<subpane> create_subpane(const rcref<pane>& child, double secondaryAlignment)
	{
		alignment a;
		a[orientation] = 0.0;
		a[!orientation] = secondaryAlignment;
		rcref<override_bounds_frame> f = rcnew(override_bounds_frame)(a);
		rcref<subpane> p = rcnew(subpane)(child, std::move(f));
		return p;
	}

public:
	struct options
	{
		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
		frame_list frames;
		pane_list children;
	};

	fixed_default_size_stack_panel()
		: fixed_default_size_stack_panel(options())
	{ }

	explicit fixed_default_size_stack_panel(options&& o)
		: pane({
			.compositingBehavior = o.compositingBehavior,
			.frames = std::move(o.frames)
		})
	{
		for (auto& child : o.children)
			nest_last(child);
	}

	virtual dimension get_primary_flow_dimension() const { return orientation; }

	virtual range get_range() const { return m_calculatedRange; }

	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }

	virtual collaborative_sizes calculate_collaborative_sizes(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<quadrant_mask>& quadrants = std::nullopt,
		const std::optional<dimension>& resizeDimension = std::nullopt) const
	{
		// Bypass pane::propose_size() to avoid default child sizing behavior.
		return cell::calculate_collaborative_sizes(sz, r, quadrants, resizeDimension);
	}

	void nest_last(const rcref<pane>& child, double secondaryAlignment)
	{
		rcref<subpane> p = create_subpane(child, secondaryAlignment);
		pane::nest_last(p);
		install_detach_handler(child);
	}

	void nest_first(const rcref<pane>& child, double secondaryAlignment)
	{
		rcref<subpane> p = create_subpane(child, secondaryAlignment);
		pane::nest_first(p);
		install_detach_handler(child);
	}

	void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child, double secondaryAlignment)
	{
		rcref<subpane> p = create_subpane(child, secondaryAlignment);
		pane::nest_before(beforeThis, p);
		install_detach_handler(child);
	}

	void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child, double secondaryAlignment)
	{
		rcref<subpane> p = create_subpane(child, secondaryAlignment);
		pane::nest_after(afterThis, p);
		install_detach_handler(child);
	}

	void nest(const rcref<pane>& child, double secondaryAlignment = 0.5) { return nest_last(child, secondaryAlignment); }

	virtual void nest_last(const rcref<pane>& child) { return nest_last(child, 0.5); }
	virtual void nest_first(const rcref<pane>& child) { return nest_first(child, 0.5); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { return nest_before(beforeThis, child, 0.5); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { return nest_after(afterThis, child, 0.5); }
};

#if 0

template <gfx::dimension orientation = gfx::dimension::vertical, gfx::sizing_policy policy = gfx::sizing_policy::proportional>
class stack_panel : public pane, public virtual pane_container
{
private:
	range m_calculatedRange;
	std::optional<size> m_calculatedDefaultSize;
	sizing_group<policy> m_sizingGroup;

	class subpane : public container_pane, public sizing_group<policy>::cell, public rbtree_node_t<subpane>
	{
	public:
		rcref<override_bounds_frame> m_frame;
	};

	void install_detach_handler(const rcref<pane>& child)
	{
		child->get_detach_event() += [](const rcref<pane>& p)
		{
			rcptr<pane> containerPane = p->get_parent();
			if (!!containerPane)
				containerPane->detach();
		};
	}

	virtual void detaching_child(const rcref<pane>& child)
	{
		subpane& p = *(child.static_cast_to<subpane>());
		m_sizingGroup.remove_cell(p);
	}

public:
	struct options
	{
		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
		frame_list frames;
		pane_list children;
	};

	stack_panel()
		: stack_panel(options())
	{ }

	explicit stack_panel(options&& o)
		: pane({
			.compositingBehavior = o.compositingBehavior,
			.frames = std::move(o.frames)
			})
	{
		for (auto& child : o.children)
			nest_last(child);
	}

	virtual dimension get_primary_flow_dimension() const { return orientation; }

	void nest(const rcref<pane>& child) { return nest_last(child); }

	virtual void nest_last(const rcref<pane>& child)
	{
		rcref<subpane> p = rcnew(subpane)(child);
		pane::nest_last(p);
		install_detach_handler(child);
	}

	virtual void nest_first(const rcref<pane>& child)
	{
		rcref<subpane> p = rcnew(subpane)(child);
		pane::nest_first(p);
		install_detach_handler(child);
	}

	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child)
	{
		rcref<subpane> p = rcnew(subpane)(child);
		pane::nest_before(beforeThis, p);
		install_detach_handler(child);
	}

	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child)
	{
		rcref<subpane> p = rcnew(subpane)(child);
		pane::nest_after(afterThis, p);
		install_detach_handler(child);
	}

//#We want to change: size::operator!(), range::operator+(), range::operator-().  Probably everything that considers empty and zero.

	virtual void calculating_range()
	{
		m_sizingGroup.clear();
		const pane_list& children = get_children();
		if (!children.is_empty())
		{
			bool anyNotEmpty = false;
			m_calculatedRange.set_fixed(0, 0);
			size_t index = 0;
			for (auto& child : children)
			{
				subpane& p = *(child.static_cast_to<subpane>());
				p.m_index = index;
				++index;
				range childRange = p.get_frame_range();
				if (childRange.is_empty())
//#error Set some internal field to cause it to be ignored
					;// Mark cell as not participating
				else
				{
					std::optional<size> childSizeOpt = p.get_frame_default_size();
					size childSize = childSizeOpt.has_value() ? *childSizeOpt :
					if
						childSize = *childSizeOpt;
					if (!childSize)
						continue;
					anyNotEmpty = true;

					m_calculatedDefaultSize[orientation] += childSize[orientation];
					if (m_calculatedDefaultSize[!orientation] < childSize[!orientation])
						m_calculatedDefaultSize[!orientation] = childSize[!orientation];

					// In the main dimension, we want to sum all ranges.
					// In the other dimension, we want to use the largest min and max values.
					// Cells that cannot be stretched will be aligned instead.
					m_calculatedRange.set(orientation, m_calculatedRange[orientation] + childRange[orientation]);

					geometry::linear::range otherLinearRange = m_calculatedRange[!orientation];
					geometry::linear::range childOtherLinearRange = childRange[!orientation];
					anyNonEmpty |= childRange[!orientation].is_empty();
					if (otherLinearRange.get_min() < childOtherLinearRange.get_min())
						m_calculatedRange.set_min(!orientation, childOtherLinearRange.get_min());

					if (otherLinearRange.has_max())
					{
						if (!childOtherLinearRange.has_max())
							m_calculatedRange.clear_max(!orientation);
						else if (otherLinearRange.get_max() < childOtherLinearRange.get_max())
							m_calculatedRange.set_max(!orientation, childOtherLinearRange.get_max());
					}

					p.reset_cell(childRange[orientation], childSize[orientation]);
					m_sizingGroup.add_cell(p);
				}
			}

			COGS_ASSERT(m_calculatedDefaultSize == propose_size(m_calculatedDefaultSize).find_first_valid_size(orientation));
		}
		set_calculated_default_size(m_calculatedDefaultSize);
		set_calculated_range(m_calculatedRange);
	}

	virtual collaborative_sizes calculate_collaborative_sizes(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<quadrant_mask>& quadrants = std::nullopt,
		const std::optional<dimension>& resizeDimension = std::nullopt) const
	{
	}
};

#endif

//template <gfx::dimension orientation = gfx::dimension::vertical, gfx::sizing_policy policy = gfx::sizing_policy::proportional>
//class stack_panel : public pane, public virtual pane_container
//{
//private:
//	sizing_group<policy> m_sizingGroup;
//
//	class subpane : public container_pane, public sizing_group<policy>::cell, public rbtree_node_t<subpane>
//	{
//	public:
//		rcref<override_bounds_frame> m_frame;
//		propose_size_result m_idealProposedSizeResult;
//		propose_size_result m_lastProposedSizeResult;
//
//		std::optional<size> m_workingSize;
//		std::optional<size> m_workingGreaterSize;
//
//		size m_workingGreaterOtherSize;
//		size m_lastProposed;
//		size_t m_index;
//		double m_key;
//		double m_delta;
//		double m_pendingLength;
//
//		subpane(
//			const rcref<pane>& child,
//			rcref<override_bounds_frame>&& f = rcnew(override_bounds_frame))
//			: container_pane(
//				container_pane::options{
//					.frames{f},
//					.children{child}
//				}
//			),
//			m_frame(std::move(f))
//		{ }
//
//		bool calculate_key(double& currentLength)// , dimension proposedResizeDimension)
//		{
//			// Determines working size and next larger working size.  May be the same.
//			m_workingSize.reset();
//			const std::optional<size>& lesserLesserSize = m_lastProposedSizeResult.get_size(orientation, false, false);
//			const std::optional<size>& lesserGreaterSize = m_lastProposedSizeResult.get_size(orientation, false, true);
//			const std::optional<size>& greaterLesserSize = m_lastProposedSizeResult.get_size(orientation, true, false);
//			const std::optional<size>& greaterGreaterSize = m_lastProposedSizeResult.get_size(orientation, true, true);
//			for (;;)
//			{
//				if (lesserLesserSize.has_value())
//				{
//					if (lesserGreaterSize.has_value())
//					{
//						// use closest in primary/resize dimension, preferring the greater one if equal.
//						if ((*lesserLesserSize)[proposedResizeDimension] > (*lesserGreaterSize)[proposedResizeDimension])
//						{
//							m_workingSize = lesserGreaterSize;
//							break;
//						}
//					}
//					m_workingSize = lesserLesserSize;
//					break;
//				}
//				if (lesserGreaterSize.has_value())
//				{
//					m_workingSize = lesserGreaterSize;
//					break;
//				}
//				if (greaterLesserSize.has_value())
//				{
//					if (greaterGreaterSize.has_value())
//					{
//						// use closest in primary/resize dimension, preferring the lesser one if equal.
//						if ((*greaterLesserSize)[proposedResizeDimension] < (*greaterGreaterSize)[proposedResizeDimension])
//						{
//							m_workingSize = greaterGreaterSize;
//							break;
//						}
//					}
//					m_workingSize = greaterLesserSize;
//					break;
//				}
//				m_workingSize = greaterGreaterSize;
//				if (!m_workingSize.has_value())
//					return false;
//				break;
//			}
//#error
//
//			m_workingSize = m_lastProposedSizeResult.get_size(orientation, false, false);
//			if (!greaterSize.has_value()) // At max length (without exceeding in other dimension)
//				return false;
//			// Known to be >= min in other dimension.
//			if (lesserSize.has_value())
//				currentLength += lesserSize->get(orientation);
//			else
//			{
//				if (!greaterSize.has_value())
//					currentLength += greaterSize->get(orientation);
//				return false;
//			}
//			if (!greaterSize.has_value()) // At max length (without exceeding in other dimension)
//				return false;
//			double greaterLength = greaterSize->get(orientation);
//			if constexpr (policy == sizing_policy::equitable)
//				m_key = greaterLength; // If converging, sort smallest greater-length first.
//			else
//			{
//				double idealLength = sizing_group<policy>::cell::get_sized_length();
//				if constexpr (policy == sizing_policy::proportional)
//				{
//					// If proportional, need to sort by proportional relative distance to ideal size.
//					COGS_ASSERT(!!idealLength);
//					m_key = greaterLength / idealLength; // Will be >= 1
//				}
//				else
//				{
//					// If no sizing policy, use distance from ideal size
//					COGS_ASSERT(length >= idealLength);
//					m_key = greaterLength - idealLength;
//				}
//			}
//			// All that are as-proposed, need to be sorted before those that are not.
//			m_delta = 0;
//			if (lesserSize.has_value() && greaterSize.has_value())
//			{
//				double lesserLength = lesserSize->get(orientation);
//				if (lesserLength != greaterLength)
//				{
//					m_delta = greaterLength - lesserLength;
//					COGS_ASSERT(m_delta > 0);
//				}
//			}
//			return true;
//		}
//	};
//
//	std::pair<std::optional<gfx::size>, std::optional<gfx::size>> calculate_proposed(double proposedLength, double proposedOther, dimension proposedResizeDimension) const
//	{
//		std::pair<std::optional<gfx::size>, std::optional<gfx::size>> result;
//
//		const double fudge = 0.0001; // Prevent perpetually negotiating over a miniscule fraction of a pixel.  Addresses rounding issues.
//		rbtree<std::tuple<double, bool, size_t>, subpane> sorted;
//		double currentLength = 0;
//		const pane_list& children = get_children();
//		for (auto& child : children)
//		{
//			subpane& p = *(child.static_cast_to<subpane>());
//			if (p.calculate_key(currentLength, useGreaterOther))
//				sorted.insert_multi(&p);
//		}
//		if (currentLength < proposedLength + fudge)
//		{
//			double remainingLength = proposedLength - currentLength;
//			while (!sorted.is_empty() && (remainingLength > fudge))
//			{
//				ptr<subpane> noDeltaChain;
//				auto reinsert = [&]()
//				{
//					ptr<subpane> p = noDeltaChain;
//					do {
//						std::optional<size>& lesserSize = p->m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
//						std::optional<size>& greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
//						if (lesserSize.has_value())
//							currentLength -= lesserSize->get(orientation);
//						else
//							currentLength -= greaterSize->get(orientation);
//						size childSize;
//						childSize[orientation] = p->m_pendingLength;
//						childSize[!orientation] = proposedOther;
//						sizing_mask sizingMask;
//						if (orientation == dimension::vertical)
//						{
//							if (useGreaterOther)
//								sizingMask = sizing_type::greater_width_lesser_height | sizing_type::greater_width_greater_height;
//							else
//								sizingMask = sizing_type::lesser_width_lesser_height | sizing_type::lesser_width_greater_height;
//						}
//						else
//						{
//							if (useGreaterOther)
//								sizingMask = sizing_type::greater_width_greater_height | sizing_type::lesser_width_greater_height;
//							else
//								sizingMask = sizing_type::greater_width_lesser_height | sizing_type::lesser_width_lesser_height;
//						}
//						propose_size_result tempResult = p->propose_size(childSize, range::make_unbounded(), orientation, sizingMask);
//						lesserSize = tempResult.get_size(orientation, false, useGreaterOther);
//						greaterSize = tempResult.get_size(orientation, true, useGreaterOther);
//						ptr<subpane> p2 = p;
//						p = p->get_right_link();
//						if (p2->calculate_key(currentLength, useGreaterOther))
//							sorted.insert_multi(p2);
//						remainingLength = proposedLength - currentLength;
//					} while (!!p);
//				};
//
//				double totalIdealLength = 0;
//				double totalUsedLength = 0;
//				size_t noDeltaCount = 0;
//				ptr<subpane> p = sorted.get_first();
//				double previousKey = p->m_key;
//				while (!!p)
//				{
//					double candidateIdealLength = p->get_sized_length();
//					if (!p->m_delta && p->m_key == previousKey)
//					{
//						totalIdealLength += candidateIdealLength;
//						std::optional<size> greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, false);
//						COGS_ASSERT(greaterSize.has_value()); // If it's in the list, there are both lesser and greater sizes, and they are equal.
//						// TODO: could use m_workingSize
//						double usedLength = greaterSize->get(orientation);
//						totalUsedLength += usedLength;
//						++noDeltaCount;
//						previousKey = p->m_key;
//						sorted.remove(p);
//						p->set_right_link(noDeltaChain); // Use as a linked list.
//						noDeltaChain = p;
//						p = sorted.get_first(); // Will have been the first element
//						continue;
//					}
//					if (!!noDeltaCount)
//					{
//#error
//						calculate_no_deltas(*p, remainingLength, totalUsedLength, totalIdealLength, noDeltaCount, noDeltaChain, useGreaterOther);
//						noDeltaCount = 0;
//						reinsert();
//						break;
//					}
//					// If there is enough remaining to promote this (first) item, promote it and re-sort.  Otherwise, remove it.
//					sorted.remove(p);
//					if (p->m_delta <= remainingLength)
//					{
//						std::optional<size>& greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
//						std::optional<size>& lesserSize = p->m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
//						lesserSize = greaterSize;
//						currentLength += p->m_delta;
//						remainingLength -= p->m_delta;
//						p->m_delta = 0;
//						sorted.insert_multi(p);
//					}
//					break;
//				}
//				if (noDeltaCount > 0)
//				{
//					// The only remaining elements have no deltas.  Spread remaining length to all based on sizing policy.
//					calculate_no_deltas_remaining(remainingLength, totalUsedLength, totalIdealLength, noDeltaCount, noDeltaChain, useGreaterOther);
//					reinsert();
//				}
//			}
//		}
//
//		const pane_list& children2 = pane::get_children();
//		bool b = false;
//		double nearestOther = 0;
//		for (auto& child : children2)
//		{
//			subpane& p = *(child.static_cast_to<subpane>());
//			std::optional<size> elementSize = p.m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
//			if (!elementSize.has_value())
//				elementSize = p.m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
//			double otherLength = elementSize->get(!orientation);
//			if (!b || (useGreaterOther && nearestOther > otherLength) || (!useGreaterOther && nearestOther < otherLength))
//			{
//				nearestOther = otherLength;
//				if (nearestOther == otherLength)
//					break;
//				b = true;
//			}
//		}
//
//
//
//
//		return result;
//	}
//
//	void install_detach_handler(const rcref<pane>& child)
//	{
//		child->get_detach_event() += [](const rcref<pane>& p)
//		{
//			rcptr<pane> containerPane = p->get_parent();
//			if (!!containerPane)
//				containerPane->detach();
//		};
//	}
//
//	virtual void detaching_child(const rcref<pane>& child)
//	{
//		subpane& p = *(child.static_cast_to<subpane>());
//		m_sizingGroup.remove_cell(p);
//	}
//
//public:
//	struct options
//	{
//		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
//		frame_list frames;
//		pane_list children;
//	};
//
//	stack_panel()
//		: stack_panel(options())
//	{ }
//
//	explicit stack_panel(options&& o)
//		: pane({
//			.compositingBehavior = o.compositingBehavior,
//			.frames = std::move(o.frames)
//		})
//	{
//		for (auto& child : o.children)
//			nest_last(child);
//	}
//
//	virtual dimension get_primary_flow_dimension() const { return orientation; }
//
//	void nest(const rcref<pane>& child) { return nest_last(child); }
//
//	virtual void nest_last(const rcref<pane>& child)
//	{
//		rcref<subpane> p = rcnew(subpane)(child);
//		pane::nest_last(p);
//		install_detach_handler(child);
//	}
//
//	virtual void nest_first(const rcref<pane>& child)
//	{
//		rcref<subpane> p = rcnew(subpane)(child);
//		pane::nest_first(p);
//		install_detach_handler(child);
//	}
//
//	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child)
//	{
//		rcref<subpane> p = rcnew(subpane)(child);
//		pane::nest_before(beforeThis, p);
//		install_detach_handler(child);
//	}
//
//	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child)
//	{
//		rcref<subpane> p = rcnew(subpane)(child);
//		pane::nest_after(afterThis, p);
//		install_detach_handler(child);
//	}
//
//	virtual void calculating_range()
//	{
//		range calculatedRange;
//		std::optional<size> defaultSize;
//
//		m_sizingGroup.clear();
//		const pane_list& children = get_children();
//		if (!children.is_empty())
//		{
//			calculatedRange.set_fixed(0, 0);
//			size_t index = 0;
//			for (auto& child : children)
//			{
//				subpane& p = *(child.static_cast_to<subpane>());
//				p.m_index = index;
//				++index;
//				range childRange = p.get_frame_range();
//				size childSize = childRange[orientation].get_min();
//				std::optional<size> childSizeOpt = p.get_frame_default_size();
//				if (childSizeOpt.has_value())
//					childSize = *childSizeOpt;
//				if (!childSize)
//					continue;
//
//				defaultSize[orientation] += childSize[orientation];
//				if (defaultSize[!orientation] < childSize[!orientation])
//					defaultSize[!orientation] = childSize[!orientation];
//
//				// In the main dimension, we want to sum all ranges.
//				// In the other dimension, we want to use the largest min and max values.
//				// Cells that cannot be stretched will be aligned instead.
//				calculatedRange.set(orientation, calculatedRange[orientation] + childRange[orientation]);
//
//				geometry::linear::range otherLinearRange = calculatedRange[!orientation];
//				geometry::linear::range childOtherLinearRange = childRange[!orientation];
//				if (otherLinearRange.get_min() < childOtherLinearRange.get_min())
//					calculatedRange.set_min(!orientation, childOtherLinearRange.get_min());
//
//				if (otherLinearRange.has_max())
//				{
//					if (!childOtherLinearRange.has_max())
//						calculatedRange.clear_max(!orientation);
//					else if (otherLinearRange.get_max() < childOtherLinearRange.get_max())
//						calculatedRange.set_max(!orientation, childOtherLinearRange.get_max());
//				}
//
//				p.reset_cell(childRange[orientation], childSize[orientation]);
//				m_sizingGroup.add_cell(p);
//			}
//
//			COGS_ASSERT(defaultSize == propose_size(defaultSize).find_first_valid_size(orientation));
//		}
//		set_calculated_default_size(defaultSize);
//		set_calculated_range(calculatedRange);
//	}
//
//	virtual propose_size_result propose_size(
//		const size& sz,
//		const range& r = range::make_unbounded(),
//		const std::optional<dimension>& resizeDimension = std::nullopt,
//		sizing_mask = all_sizing_types) const
//	{
//		propose_size_result result;
//		range r2 = get_range() & r;
//		if (!r2.is_empty())
//		{
//			const size newSize = r2.get_limit(sz);
//			const double& proposedLength = newSize[orientation];
//			const double& proposedOther = newSize[!orientation];
//			double idealLength = m_sizingGroup.calculate_sizes(proposedLength);
//			COGS_ASSERT(idealLength == proposedLength); // We already ensure it's within range.
//			range r3 = range::make_unbounded();
//			r3.set(!orientation, r2[!orientation]);
//			const pane_list& children = pane::get_children();
//			//dimension proposeResizeDimension = resizeDimension.has_value() ? *resizeDimension : orientation;
//			for (auto& child : children)
//			{
//				subpane& p = *(child.static_cast_to<subpane>());
//				size childSize;
//				childSize[orientation] = p.get_sized_length();
//				childSize[!orientation] = proposedOther;
//
//				// The 2 sizes with lesser secondary lengths are calculated first.
//				// The primary dimension is passed as the resize dimension when proposing sizes for children.
//				// Nearest possible primary lengths are used.
//				// This ensures that any gaps between the nearest lesser and nearest greater secondary lengths
//				// could not possibly result in changes to primary lengths.
//				// The nearest greater secondary child length is used to calculate the 2 sizes with greater secondary lengths.
//				// Although there can be no lesser secondary length, it's possible that secondary length may not
//				// actually be matched exactly, so the process is repeated with the next nearest greater secondary length.
//				//p.m_idealProposedSizeResult = p.propose_size(childSize, r3, orientation);
//				p.m_idealProposedSizeResult = p.propose_size(childSize, r3, !orientation);
//				p.m_lastProposedSizeResult = p.m_idealProposedSizeResult;
//			}
//			std::pair<std::optional<size>, std::optional<size>> result1 = calculate_proposed(proposedLength, proposedOther);// , proposeResizeDimension);
//
//			result.get_size(orientation, false, false) = result1.first;
//			result.get_size(orientation, true, false) = result1.second;
//
//			std::pair<std::optional<size>, std::optional<size>> result2;
//			if (result1.first[!orientation].has_value())
//			{
//				if (*result1.first[!orientation] == proposedOther)
//					result2.first = result1.first;
//				else if (result1.first[!orientation] > proposedOther)
//				{
//					result2.first = result1.first;
//					result1.first.reset();
//				}
//			}
//			if (result1.second[!orientation].has_value())
//			{
//				if (*result1.second[!orientation] == proposedOther)
//					result2.second = result1.second;
//				else if (*result1.second[!orientation] > proposedOther)
//				{
//					result2.second = result1.second;
//					result1.second.reset();
//				}
//			}
//
//			while (!result2.first.has_value() || !result2.second.has_value())
//			{
//				double nearestGreater = 0;
//				for (auto& child : children)
//				{
//					subpane& p = *(child.static_cast_to<subpane>());
//					std::optional<size> childLesserGreater = p.m_lastProposedSizeResult.get_size(orientation, false, true);
//					COGS_ASSERT(!childLesserGreater.has_value() || (*childLesserGreater)[!orientation] != proposedOther);
//					std::optional<size> childGreaterGreater = p.m_lastProposedSizeResult.get_size(orientation, true, true);
//					COGS_ASSERT(!childGreaterGreater.has_value() || (*childGreaterGreater)[!orientation] != proposedOther);
//					double childNearestGreater = 0;
//					if (childLesserGreater.has_value())
//					{
//						if (childGreaterGreater.has_value())
//							childNearestGreater = std::min(*childLesserGreater, *childGreaterGreater);
//						else
//							childNearestGreater = *childLesserGreater;
//					}
//					else if (childGreaterGreater.has_value())
//						childNearestGreater = *childGreaterGreater;
//
//					if (childNearestGreater != 0)
//					{
//						if (nearestGreater == 0)
//							nearestGreater = childNearestGreater;
//						else if (childNearestGreater < nearestGreater)
//							nearestGreater = childNearestGreater;
//					}
//					p.m_lastProposedSizeResult = p.m_idealProposedSizeResult;
//				}
//				if (nearestGreater == 0)
//					break;
//				result1 = calculate_proposed(nearestGreater, proposedOther, proposeResizeDimension);
//				if (result1.first[!orientation] == proposedOther)
//					result2.first[!orientation] = proposedOther;
//				if (result1.second[!orientation] == proposedOther)
//					result2.second[!orientation] = proposedOther;
//			}
//
//			result.set_relative_to(sz, orientation, resizeDimension);
//		}
//		return result;
//	}
//};
//
//
////
////template <gfx::dimension orientation = gfx::dimension::vertical, gfx::sizing_policy policy = gfx::sizing_policy::proportional>
////class stack_panel : public pane, public virtual pane_container
////{
////private:
////	geometry::linear::range m_range;
////	double m_default;
////	sizing_group<policy> m_sizingGroup;
////
////	class subpane : public container_pane, public sizing_group<policy>::cell, public rbtree_node_t<subpane>
////	{
////	public:
////		rcref<override_bounds_frame> m_frame;
////		propose_size_result m_idealProposedSizeResult;
////		propose_size_result m_lastProposedSizeResult;
////		size m_workingSize;
////		std::optional<size> m_workingGreaterSize;
////		size m_workingGreaterOtherSize;
////		size m_lastProposed
////		size_t m_index;
////		double m_key;
////		double m_delta;
////		double m_pendingLength;
////
////		subpane(
////			const rcref<pane>& child,
////			rcref<override_bounds_frame>&& f = rcnew(override_bounds_frame))
////			: container_pane(
////				container_pane::options{
////					.frames{f},
////					.children{child}
////				}
////			),
////			m_frame(std::move(f))
////		{ }
////
////		bool calculate_key(double& currentLength)
////		{
////			const std::optional<size>& lesserSize = m_lastProposedSizeResult.get_size(orientation, false, false);
////			const std::optional<size>& greaterSize = m_lastProposedSizeResult.get_size(orientation, true, false);
////			// Known to be >= min in other dimension.
////			if (lesserSize.has_value())
////				currentLength += lesserSize->get(orientation);
////			else
////			{
////				if (!greaterSize.has_value())
////					currentLength += greaterSize->get(orientation);
////				return false;
////			}
////			if (!greaterSize.has_value()) // At max length (without exceeding in other dimension)
////				return false;
////			double greaterLength = greaterSize->get(orientation);
////			if constexpr (policy == sizing_policy::equitable)
////				m_key = greaterLength; // If converging, sort smallest greater-length first.
////			else
////			{
////				double idealLength = sizing_group<policy>::cell::get_sized_length();
////				if constexpr (policy == sizing_policy::proportional)
////				{
////					// If proportional, need to sort by proportional relative distance to ideal size.
////					COGS_ASSERT(!!idealLength);
////					m_key = greaterLength / idealLength; // Will be >= 1
////				}
////				else
////				{
////					// If no sizing policy, use distance from ideal size
////					COGS_ASSERT(length >= idealLength);
////					m_key = greaterLength - idealLength;
////				}
////			}
////			// All that are as-proposed, need to be sorted before those that are not.
////			m_delta = 0;
////			if (lesserSize.has_value() && greaterSize.has_value())
////			{
////				double lesserLength = lesserSize->get(orientation);
////				if (lesserLength != greaterLength)
////				{
////					m_delta = greaterLength - lesserLength;
////					COGS_ASSERT(m_delta > 0);
////				}
////			}
////			return true;
////		}
////
////		std::tuple<double, bool, size_t> get_key() const { return std::make_tuple(m_key, !m_delta, m_index); }
////	};
////
////	std::optional<size> m_calculatedDefaultSize;
////	range m_calculatedRange;
////
////	void install_detach_handler(const rcref<pane>& child)
////	{
////		child->get_detach_event() += [](const rcref<pane>& p)
////		{
////			rcptr<pane> containerPane = p->get_parent();
////			if (!!containerPane)
////				containerPane->detach();
////		};
////	}
////
////	virtual void detaching_child(const rcref<pane>& child)
////	{
////		subpane& p = *(child.static_cast_to<subpane>());
////		m_sizingGroup.remove_cell(p);
////	}
////
////	void calculate_no_deltas_remaining(double remainingLength, double totalUsedLength, double totalIdealLength, size_t noDeltaCount, const ptr<subpane>& noDeltaChain) const
////	{
////		// Doesn't fit.  Distribute the remaining appropriately.
////		ptr<subpane> p;
////		if constexpr (policy == sizing_policy::equitable)
////		{
////			// Each will be the same size
////			// Take the remaining size and total of calculated size, and split it up.
////			double remainingUsedLength = totalUsedLength + remainingLength;
////			p = noDeltaChain;
////			do {
////				double tempLength = remainingUsedLength / noDeltaCount;
////				remainingUsedLength -= tempLength;
////				--noDeltaCount;
////				p->m_pendingLength = tempLength;
////				p = p->get_right_link();
////			} while (!!p);
////		}
////		else if constexpr (policy == sizing_policy::proportional)
////		{
////			double tempRemainingTotalLength = totalUsedLength + remainingLength;
////			p = noDeltaChain;
////			do {
////				double candidateIdealLength = p->get_sized_length();
////				double tempLength = (candidateIdealLength * tempRemainingTotalLength) / totalIdealLength;
////				tempRemainingTotalLength -= tempLength;
////				totalIdealLength -= candidateIdealLength;
////				p->m_pendingLength = tempLength;
////				p = p->get_right_link();
////			} while (!!p);
////		}
////		else
////		{
////			// Give the same amount of the remaining to each.
////			double tempRemainingLength = remainingLength;
////			p = noDeltaChain;
////			do {
////				double delta = tempRemainingLength / noDeltaCount;
////				tempRemainingLength -= delta;
////				--noDeltaCount;
////				std::optional<size>& greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
////				double usedLength = greaterSize->get(orientation);
////				p->m_pendingLength = usedLength + delta;
////				p = p->get_right_link();
////			} while (!!p);
////		}
////	}
////
////	void calculate_no_deltas(subpane& nextPane, double remainingLength, double totalUsedLength, double totalIdealLength, size_t noDeltaCount, const ptr<subpane>& noDeltaChain, bool useGreaterOther) const
////	{
////		double totalRemainingSpent = 0;
////		ptr<subpane> p = noDeltaChain;
////		// First, calculate based on promoting them all to equivalent to nextPane.
////		do {
////			double newLength;
////			double candidateIdealLength = p->get_sized_length();
////			std::optional<size>& lengthOpt = nextPane.m_lastProposedSizeResult.get_size(orientation, true, false);
////			COGS_ASSERT(lengthOpt.has_value()); // If it was in the list, it should have both lesser and greater values.
////			if constexpr (policy == sizing_policy::equitable)
////				newLength = lengthOpt->get(orientation);
////			else if constexpr (policy == sizing_policy::proportional)
////			{
////				double length = lengthOpt->get(orientation);
////				double idealLength = nextPane.get_sized_length();
////				newLength = (candidateIdealLength * length) / idealLength;
////			}
////			else
////				newLength = candidateIdealLength + nextPane.m_key;
////			p->m_pendingLength = newLength;
////			std::optional<size> candidateGreaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, false);
////			COGS_ASSERT(candidateGreaterSize.has_value()); // If it was in the list, it should have both lesser and greater values.
////			double usedLength = candidateGreaterSize->get(orientation);
////			totalRemainingSpent += newLength - usedLength;
////			p = p->get_right_link();
////#error
////		} while (!!p);
////		// Then, if less or equal to remainingLength, use it.  Otherwise, recalculated based on remainingLength only.
////		if (totalRemainingSpent > remainingLength)
////			calculate_no_deltas_remaining(remainingLength, totalUsedLength, totalIdealLength, noDeltaCount, noDeltaChain, useGreaterOther);
////	}
////
////	void calculate_proposed(propose_size_result& result, double proposedLength, double proposedOther, bool useGreaterOther) const
////	{
////		const double fudge = 0.0001; // Prevent perpetually negotiating over a miniscule fraction of a pixel.  Addresses rounding issues.
////		rbtree<std::tuple<double, bool, size_t>, subpane> sorted;
////		double currentLength = 0;
////		const pane_list& children = get_children();
////		for (auto& child : children)
////		{
////			subpane& p = *(child.static_cast_to<subpane>());
////			if (p.calculate_key(currentLength, useGreaterOther))
////				sorted.insert_multi(&p);
////		}
////		if (currentLength < proposedLength + fudge)
////		{
////			double remainingLength = proposedLength - currentLength;
////			while (!sorted.is_empty() && (remainingLength > fudge))
////			{
////				ptr<subpane> noDeltaChain;
////				auto reinsert = [&]()
////				{
////					ptr<subpane> p = noDeltaChain;
////					do {
////						std::optional<size>& lesserSize = p->m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
////						std::optional<size>& greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
////						if (lesserSize.has_value())
////							currentLength -= lesserSize->get(orientation);
////						else
////							currentLength -= greaterSize->get(orientation);
////						size childSize;
////						childSize[orientation] = p->m_pendingLength;
////						childSize[!orientation] = proposedOther;
////						sizing_mask sizingMask;
////						if (orientation == dimension::vertical)
////						{
////							if (useGreaterOther)
////								sizingMask = sizing_type::greater_width_lesser_height | sizing_type::greater_width_greater_height;
////							else
////								sizingMask = sizing_type::lesser_width_lesser_height | sizing_type::lesser_width_greater_height;
////						}
////						else
////						{
////							if (useGreaterOther)
////								sizingMask = sizing_type::greater_width_greater_height | sizing_type::lesser_width_greater_height;
////							else
////								sizingMask = sizing_type::greater_width_lesser_height | sizing_type::lesser_width_lesser_height;
////						}
////						propose_size_result tempResult = p->propose_size(childSize, range::make_unbounded(), orientation, sizingMask);
////						lesserSize = tempResult.get_size(orientation, false, useGreaterOther);
////						greaterSize = tempResult.get_size(orientation, true, useGreaterOther);
////						ptr<subpane> p2 = p;
////						p = p->get_right_link();
////						if (p2->calculate_key(currentLength, useGreaterOther))
////							sorted.insert_multi(p2);
////						remainingLength = proposedLength - currentLength;
////					} while (!!p);
////				};
////
////				double totalIdealLength = 0;
////				double totalUsedLength = 0;
////				size_t noDeltaCount = 0;
////				ptr<subpane> p = sorted.get_first();
////				double previousKey = p->m_key;
////				while (!!p)
////				{
////					double candidateIdealLength = p->get_sized_length();
////					if (!p->m_delta && p->m_key == previousKey)
////					{
////						totalIdealLength += candidateIdealLength;
////						std::optional<size> greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, false);
////						COGS_ASSERT(greaterSize.has_value()); // If it's in the list, there are both lesser and greater sizes, and they are equal.
////						// TODO: could use m_workingSize
////						double usedLength = greaterSize->get(orientation);
////						totalUsedLength += usedLength;
////						++noDeltaCount;
////						previousKey = p->m_key;
////						sorted.remove(p);
////						p->set_right_link(noDeltaChain); // Use as a linked list.
////						noDeltaChain = p;
////						p = sorted.get_first(); // Will have been the first element
////						continue;
////					}
////					if (!!noDeltaCount)
////					{
////#error
////						calculate_no_deltas(*p, remainingLength, totalUsedLength, totalIdealLength, noDeltaCount, noDeltaChain, useGreaterOther);
////						noDeltaCount = 0;
////						reinsert();
////						break;
////					}
////					// If there is enough remaining to promote this (first) item, promote it and re-sort.  Otherwise, remove it.
////					sorted.remove(p);
////					if (p->m_delta <= remainingLength)
////					{
////						std::optional<size>& greaterSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
////						std::optional<size>& lesserSize = p->m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
////						lesserSize = greaterSize;
////						currentLength += p->m_delta;
////						remainingLength -= p->m_delta;
////						p->m_delta = 0;
////						sorted.insert_multi(p);
////					}
////					break;
////				}
////				if (noDeltaCount > 0)
////				{
////					// The only remaining elements have no deltas.  Spread remaining length to all based on sizing policy.
////					calculate_no_deltas_remaining(remainingLength, totalUsedLength, totalIdealLength, noDeltaCount, noDeltaChain, useGreaterOther);
////					reinsert();
////				}
////			}
////		}
////
////		const pane_list& children2 = pane::get_children();
////		bool b = false;
////		double nearestOther = 0;
////		for (auto& child : children2)
////		{
////			subpane& p = *(child.static_cast_to<subpane>());
////			std::optional<size> elementSize = p.m_lastProposedSizeResult.get_size(orientation, false, useGreaterOther);
////			if (!elementSize.has_value())
////				elementSize = p.m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
////			double otherLength = elementSize->get(!orientation);
////			if (!b || (useGreaterOther && nearestOther > otherLength) || (!useGreaterOther && nearestOther < otherLength))
////			{
////				nearestOther = otherLength;
////				if (nearestOther == otherLength)
////					break;
////				b = true;
////			}
////		}
////
////		std::optional<size>& resultLesserSize = result.get_size(orientation, false, useGreaterOther);
////		std::optional<size>& resultGreaterSize = result.get_size(orientation, true, useGreaterOther);
////		size resultSize;
////		resultSize[orientation] = currentLength;
////		resultSize[!orientation] = nearestOther;
////		resultLesserSize = resultSize;
////		if (fudge + currentLength >= proposedLength)
////			resultGreaterSize = resultLesserSize;
////		else if (!sorted.is_empty())
////		{
////			ptr<subpane> p = sorted.get_first();
////			COGS_ASSERT(!!p->m_delta);
////			std::optional<size>& elementSize = p->m_lastProposedSizeResult.get_size(orientation, true, useGreaterOther);
////			double otherLength = elementSize->get(!orientation);
////			if (useGreaterOther)
////			{
////				if (nearestOther > otherLength)
////					nearestOther = otherLength;
////			}
////			else
////			{
////				if (nearestOther < otherLength)
////					nearestOther = otherLength;
////			}
////			resultGreaterSize->set(orientation, currentLength + p->m_delta);
////			resultGreaterSize->set(!orientation, nearestOther);
////		}
////	}
////
////public:
////	struct options
////	{
////		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
////		frame_list frames;
////		pane_list children;
////	};
////
////	stack_panel()
////		: stack_panel(options())
////	{ }
////
////	explicit stack_panel(options&& o)
////		: pane({
////			.compositingBehavior = o.compositingBehavior,
////			.frames = std::move(o.frames)
////		})
////	{
////		for (auto& child : o.children)
////			nest_last(child);
////	}
////
////	virtual dimension get_primary_flow_dimension() const { return orientation; }
////
////	virtual range get_range() const { return m_calculatedRange; }
////
////	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }
////
////	void nest(const rcref<pane>& child) { return nest_last(child); }
////
////	virtual void nest_last(const rcref<pane>& child)
////	{
////		rcref<subpane> p = rcnew(subpane)(child);
////		pane::nest_last(p);
////		install_detach_handler(child);
////	}
////
////	virtual void nest_first(const rcref<pane>& child)
////	{
////		rcref<subpane> p = rcnew(subpane)(child);
////		pane::nest_first(p);
////		install_detach_handler(child);
////	}
////
////	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child)
////	{
////		rcref<subpane> p = rcnew(subpane)(child);
////		pane::nest_before(beforeThis, p);
////		install_detach_handler(child);
////	}
////
////	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child)
////	{
////		rcref<subpane> p = rcnew(subpane)(child);
////		pane::nest_after(afterThis, p);
////		install_detach_handler(child);
////	}
////
////	virtual void calculate_range()
////	{
////		pane::calculate_range();
////		m_sizingGroup.clear();
////		const pane_list& children = get_children();
////		if (children.is_empty())
////		{
////			m_calculatedDefaultSize.reset();
////			m_calculatedRange.clear();
////			return;
////		}
////		size defaultSize(0, 0);
////		m_calculatedRange.set_fixed(0, 0);
////		size_t index = 0;
////		for (auto& child : children)
////		{
////			subpane& p = *(child.static_cast_to<subpane>());
////			p.m_index = index;
////			++index;
////			range childRange = p.get_frame_range();
////			m_range += childRange[orientation];
////			size childSize = m_range.get_min();
////			std::optional<size> childSizeOpt = p.get_frame_default_size();
////			if (childSizeOpt.has_value())
////				childSize = *childSizeOpt;
////			m_default += childSize[orientation];
////			defaultSize[orientation] += childSize[orientation];
////			if (defaultSize[!orientation] < childSize[!orientation])
////				defaultSize[!orientation] = childSize[!orientation];
////
////			// In the main dimension, we want to sum all ranges.
////			// In the other dimension, we want to use the largest min and max values.
////			// Cells that cannot be stretched will be aligned instead.
////			m_calculatedRange.set(orientation, m_calculatedRange[orientation] + childRange[orientation]);
////
////			geometry::linear::range otherLinearRange = m_calculatedRange[!orientation];
////			geometry::linear::range childOtherLinearRange = childRange[!orientation];
////			if (otherLinearRange.get_min() < childOtherLinearRange.get_min())
////				m_calculatedRange.set_min(!orientation, childOtherLinearRange.get_min());
////
////			if (otherLinearRange.has_max())
////			{
////				if (!childOtherLinearRange.has_max())
////					m_calculatedRange.clear_max(!orientation);
////				else if (otherLinearRange.get_max() < childOtherLinearRange.get_max())
////					m_calculatedRange.set_max(!orientation, childOtherLinearRange.get_max());
////			}
////
////			p.reset_cell(childRange[orientation], childSize[orientation]);
////			m_sizingGroup.add_cell(p);
////		}
////
////		COGS_ASSERT(defaultSize == propose_size(defaultSize).find_first_valid_size(orientation));
////		m_calculatedDefaultSize = defaultSize;
////	}
////
////	virtual propose_size_result propose_size(
////		const size& sz,
////		const range& r = range::make_unbounded(),
////		const std::optional<dimension>& resizeDimension = std::nullopt,
////		sizing_mask = all_sizing_types) const
////	{
////		propose_size_result result;
////		range r2 = get_range() & r;
////		if (!r2.is_empty())
////		{
////			const size newSize = r2.get_limit(sz);
////			const double& proposedLength = newSize[orientation];
////			const double& proposedOther = newSize[!orientation];
////			double idealLength = m_sizingGroup.calculate_sizes(proposedLength);
////			COGS_ASSERT(idealLength == proposedLength); // We already ensure it's within range.
////			range r3 = range::make_unbounded();
////			r3.set(!orientation, r2[!orientation]);
////			const pane_list& children = pane::get_children();
////			for (auto& child : children)
////			{
////				subpane& p = *(child.static_cast_to<subpane>());
////				size childSize;
////				childSize[orientation] = p.get_sized_length();
////				childSize[!orientation] = proposedOther;
////				p.m_idealProposedSizeResult = p.propose_size(childSize, r3, orientation);
////
////				//std::optional<size>& e1 = p.m_idealProposedSize.get_size(orientation, false, false);
////				//if (e1.has_value())
////				//	m_workingSize = *e1;
////				//else
////				//{
////				//	std::optional<size>& e2 = p.m_idealProposedSize.get_size(orientation, true, false);
////				//	COGS_ASSERT(e2.has_value());
////				//	m_workingSize = *e2;
////				//}
////
////				//m_workingGreaterSize = p.m_idealProposedSize.get_size(orientation, true, false);
////
////				//std::optional<size>& e3 = p.m_idealProposedSize.get_size(orientation, false, true);
////				//if (e3.has_value())
////				//	m_workingGreaterOtherSize = *e3;
////				//else
////				//{
////				//	std::optional<size>& e4 = p.m_idealProposedSize.get_size(orientation, true, true);
////				//	COGS_ASSERT(e4.has_value());
////				//}
////
////				p.m_lastProposedSizeResult = p.m_idealProposedSizeResult;
////			}
////			calculate_proposed(result, proposedLength, proposedOther);
////			// , false);
////			for (auto& child : children)
////			{
////				subpane& p = *(child.static_cast_to<subpane>());
////				p.m_lastProposedSizeResult = p.m_idealProposedSizeResult;
////			}
////
////			// TODO: To calculate the next larger size in the other dimension, use the next larger other length.
////			// Due to potent changes in the calculation, it's necessary to continue trying the next larger
////			// other length until one is accepted.  (TODO: Determine if this might be unbounded).
////
////			//////calculate_proposed(result, proposedLength, proposedOther, true);
////
////#error ??
////			if (result[0][0].has_value() && !r2.contains(orientation, result[0][0]->get(orientation)))
////				result[0][0].reset();
////			if (result[0][1].has_value() && !r2.contains(orientation, result[0][1]->get(orientation)))
////				result[0][1].reset();
////			if (result[1][0].has_value() && !r2.contains(orientation, result[1][0]->get(orientation)))
////				result[1][0].reset();
////			if (result[1][1].has_value() && !r2.contains(orientation, result[1][1]->get(orientation)))
////				result[1][1].reset();
////			result.set_relative_to(sz, orientation, resizeDimension);
////		}
////		return result;
////	}
////
////	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
////	{
////		propose_size_result result = propose_size(b.get_size(), range::make_unbounded(), std::nullopt, *sizing_type::lesser_width_lesser_height);
////		COGS_ASSERT(result.has_same()); // reshape() should not be called with a value that was not returned by propose_size().
////		const pane_list& children = pane::get_children();
////		point pt = { 0, 0 };
////		for (auto& child : children)
////		{
////			subpane& p = *(child.static_cast_to<subpane>());
////			std::optional<size> elementSize = p.m_lastProposedSizeResult.find_first_valid_size(orientation, false, false);
////			if (elementSize.has_value())
////				p.m_frame->set_fixed_size(*elementSize);
////			else
////				p.m_frame->set_fixed_size(0);
////			p.m_frame->set_position(pt);
////			pt[orientation] += elementSize->get(orientation);
////		}
////		pane::reshape(b, oldOrigin);
////	}
////};
//
//


}
}


#endif
