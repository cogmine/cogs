////
////  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_GUI_STACK_PANEL
//#define COGS_HEADER_GUI_STACK_PANEL
//
//#include "cogs/gui/pane.hpp"
//
//namespace cogs {
//namespace gui {
//
//
//class stack_panel : public pane, protected virtual pane_container
//{
//private:
//	struct weighted_set
//	{
//		pane_list m_panes;
//		double m_default;
//		double m_allLesserDefault;
//		geometry::linear::range m_range;
//		geometry::linear::range m_allLesserRange = geometry::linear::range::make_fixed();
//		double m_lastProposed;
//
//		void clear()
//		{
//			m_default = 0;
//			m_allLesserDefault = 0;
//			m_range.set_fixed();
//			m_allLesserRange.set_fixed();
//		}
//
//		void calculate(double proposedSize, dimension orientation, sizing_disposition disposition)
//		{
//			auto f = [&](sizing_group_base& g)
//			{
//				for (auto& child : m_panes)
//				{
//					subpane& p = *(child.static_cast_to<subpane>());
//					g.add_cell(p);
//				}
//				m_lastProposed = g.calculate_sizes(proposedSize - m_allLesserDefault);
//			};
//			switch (disposition)
//			{
//				case sizing_disposition::converging:
//				{
//					sizing_group<sizing_disposition::converging> g;
//					f(g);
//					break;
//				}
//				case sizing_disposition::proportional:
//				{
//					sizing_group<sizing_disposition::proportional> g;
//					f(g);
//					break;
//				}
//				//case sizing_disposition::none:
//				default:
//				{
//					sizing_group<sizing_disposition::none> g;
//					f(g);
//					break;
//				}
//			}
//		}
//	};
//
//	typedef nonvolatile_map<int, weighted_set> weighted_map_t;
//
//	class subpane : public container_pane, public sizing_cell
//	{
//	public:
//		rcref<override_bounds_frame> m_frame;
//
//		int m_shrinkWeight;
//		int m_stretchWeight;
//
//		weighted_map_t::iterator m_shrinkMapItor;
//		weighted_map_t::iterator m_stretchMapItor;
//
//		pane_list::remove_token m_shrinkSetRemoveToken;
//		pane_list::remove_token m_stretchSetRemoveToken;
//
//		propose_size_result m_lastProposedSizeResult;
//
//		subpane(
//			int shrinkWeight,
//			int stretchWeight,
//			const weighted_map_t::iterator& shrinkMapItor,
//			const weighted_map_t::iterator& stretchMapItor,
//			const rcref<pane>& child,
//			rcref<override_bounds_frame>&& f = rcnew(override_bounds_frame))
//			: container_pane(container_pane::options{
//				.frames{f},
//				.children{child}
//				}),
//			m_frame(std::move(f)),
//			m_shrinkWeight(shrinkWeight),
//			m_stretchWeight(stretchWeight),
//			m_shrinkMapItor(shrinkMapItor),
//			m_stretchMapItor(stretchMapItor)
//		{ }
//	};
//
//	dimension m_orientation;
//	sizing_disposition m_disposition;
//	size m_calculatedDefaultSize = { 0, 0 };
//	range m_calculatedRange;
//	weighted_map_t m_shrinkMap;
//	weighted_map_t m_stretchMap;
//
//	template <typename F>
//	void nest_inner(const rcref<pane>& child, int shrinkWeight, int stretchWeight, F&& f)
//	{
//		auto shrinkMapItor = m_shrinkMap.find(shrinkWeight);
//		if (!shrinkMapItor)
//			shrinkMapItor = m_shrinkMap.insert_unique_emplace(shrinkWeight).inserted;
//
//		auto stretchMapItor = m_stretchMap.find(stretchWeight);
//		if (!stretchMapItor)
//			stretchMapItor = m_stretchMap.insert_unique_emplace(stretchWeight).inserted;
//
//		rcref<subpane> containerPane = rcnew(subpane)(shrinkWeight, stretchWeight, shrinkMapItor, stretchMapItor, child);
//
//		containerPane->m_shrinkSetRemoveToken = shrinkMapItor->value.m_panes.append(containerPane);
//		containerPane->m_stretchSetRemoveToken = stretchMapItor->value.m_panes.append(containerPane);
//
//		f(containerPane);
//
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
//
//		pane_list& shrinkPanes = p.m_shrinkMapItor->value.m_panes;
//		shrinkPanes.remove(p.m_shrinkSetRemoveToken);
//		if (shrinkPanes.is_empty())
//			m_shrinkMap.remove(p.m_shrinkMapItor);
//		p.m_shrinkMapItor.release();
//
//		pane_list& stretchPanes = p.m_shrinkMapItor->value.m_panes;
//		stretchPanes.remove(p.m_stretchSetRemoveToken);
//		if (stretchPanes.is_empty())
//			m_stretchMap.remove(p.m_shrinkMapItor);
//		p.m_shrinkMapItor.release();
//
//		// calculate_range() will follow.
//	}
//
//public:
//	struct options
//	{
//		dimension orientation = dimension::vertical;
//		sizing_disposition disposition = sizing_disposition::none;
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
//		}),
//		m_orientation(o.orientation),
//		m_disposition(o.disposition)
//	{
//		for (auto& child : o.children)
//			nest_last(child);
//	}
//
//	virtual dimension get_primary_flow_dimension() const { return m_orientation; }
//
//	virtual range get_range() const { return m_calculatedRange; }
//
//	virtual std::optional<size> get_default_size() const { return m_calculatedDefaultSize; }
//
//	void nest(const rcref<pane>& child) { return nest_last(child, 0, 0); }
//	void nest(const rcref<pane>& child, int shrinkWeight, int stretchWeight = 0) { return nest_last(child, shrinkWeight, stretchWeight); }
//
//	virtual void nest_last(const rcref<pane>& child) { nest_last(child, 0, 0); }
//	void nest_last(const rcref<pane>& child, int shrinkWeight, int stretchWeight = 0)
//	{
//		nest_inner(child, -shrinkWeight, -stretchWeight, [&](const rcref<subpane>& p) {
//			pane_container::nest_last(p);
//		});
//	}
//
//	virtual void nest_first(const rcref<pane>& child) { nest_first(child, 0, 0); }
//	void nest_first(const rcref<pane>& child, int shrinkWeight, int stretchWeight = 0)
//	{
//		nest_inner(child, -shrinkWeight, -stretchWeight, [&](const rcref<subpane>& p) {
//			pane_container::nest_first(p);
//		});
//	}
//
//	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { nest_before(beforeThis, child, 0, 0); }
//	void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child, int shrinkWeight, int stretchWeight = 0)
//	{
//		nest_inner(child, -shrinkWeight, -stretchWeight, [&](const rcref<subpane>& p) {
//			pane_container::nest_before(beforeThis, p);
//		});
//	}
//
//	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { nest_after(afterThis, child, 0, 0); }
//	void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child, int shrinkWeight, int stretchWeight = 0)
//	{
//		nest_inner(child, -shrinkWeight, -stretchWeight, [&](const rcref<subpane>& p) {
//			pane_container::nest_after(afterThis, p);
//		});
//	}
//
//	virtual void calculate_range()
//	{
//		pane::calculate_range();
//		m_calculatedDefaultSize.set(0, 0);
//		m_calculatedRange.set_fixed(0, 0);
//
//		for (auto& shrinkSetPair : m_shrinkMap)
//			shrinkSetPair.value.clear();
//
//		for (auto& stretchSetPair : m_stretchMap)
//			stretchSetPair.value.clear();
//
//		for (auto& child : get_children())
//		{
//			subpane& p = *(child.static_cast_to<subpane>());
//			range childRange = p.get_frame_range();
//			weighted_set& shrinkSet = p.m_shrinkMapItor->value;
//			weighted_set& stretchSet = p.m_stretchMapItor->value;
//			shrinkSet.m_range += childRange[m_orientation];
//			stretchSet.m_range += childRange[m_orientation];
//			std::optional<size> childSize = p.get_frame_default_size();
//			shrinkSet.m_default += childSize[m_orientation];
//			stretchSet.m_default += childSize[m_orientation];
//			p.m_shrinkMapItor->value.m_default += childSize[m_orientation];
//			p.m_stretchMapItor->value.m_default += childSize[m_orientation];
//			m_calculatedDefaultSize[m_orientation] += childSize[m_orientation];
//			if (m_calculatedDefaultSize[!m_orientation] < childSize[!m_orientation])
//				m_calculatedDefaultSize[!m_orientation] = childSize[!m_orientation];
//
//			// In the main dimension, we want to sum all ranges.
//			// In the other dimension, we want to use the largest min and max values.
//			// Cells that cannot be stretched will be aligned instead.
//			m_calculatedRange.set(m_orientation, m_calculatedRange[m_orientation] + childRange[m_orientation]);
//
//			geometry::linear::range otherLinearRange = m_calculatedRange[!m_orientation];
//			geometry::linear::range childOtherLinearRange = childRange[!m_orientation];
//			if (otherLinearRange.get_min() < childOtherLinearRange.get_min())
//				m_calculatedRange.set_min(!m_orientation, childOtherLinearRange.get_min());
//
//			if (otherLinearRange.has_max())
//			{
//				if (!childOtherLinearRange.has_max())
//					m_calculatedRange.clear_max(!m_orientation);
//				else if (otherLinearRange.get_max() < childOtherLinearRange.get_max())
//					m_calculatedRange.set_max(!m_orientation, childOtherLinearRange.get_max());
//			}
//
//			p.reset_cell(childRange[m_orientation], childSize[m_orientation]);
//		}
//
//		// Set up m_allLesserRange
//		weighted_map_t::iterator itor = m_shrinkMap.get_first();
//		while (!!itor)
//		{
//			weighted_set& shrinkSet = itor->value;
//			weighted_map_t::iterator itor2 = itor.next();
//			while (!!itor2)
//			{
//				shrinkSet.m_allLesserRange += itor2->value.m_range;
//				++itor2;
//			}
//			++itor;
//		}
//		itor = m_stretchMap.get_first();
//		while (!!itor)
//		{
//			weighted_set& stretchSet = itor->value;
//			weighted_map_t::iterator itor2 = itor.next();
//			while (!!itor2)
//			{
//				stretchSet.m_allLesserRange += itor2->value.m_range;
//				++itor2;
//			}
//			++itor;
//		}
//		COGS_ASSERT(m_calculatedDefaultSize == propose_size(m_calculatedDefaultSize).find_first_valid_size());
//	}
//
//	virtual propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), size_mode = size_mode::both, size_mode = size_mode::both) const
//	{
//		propose_size_result result;
//		range r2 = get_range() & r;
//		if (r2.is_empty())
//			result.set_empty();
//		else
//		{
//			size newSize = r2.limit(sz);
//			double remaining;
//			if (!resizeDimension.has_value() || resizeDimension == m_orientation)
//			{
//				double defaultLength = m_calculatedDefaultSize[m_orientation];
//				double proposedLength = newSize[m_orientation];
//				double proposedOther = newSize[!m_orientation];
//				if (proposedLength == defaultLength) // stretching or shrinking
//				{
//					for (auto& child : get_children())
//					{
//						subpane& p = *(child.static_cast_to<subpane>());
//						p.set_sized_length(p.get_default_size()[m_orientation]);
//					}
//					remaining = 0;
//				}
//				else
//				{
//					struct projection
//					{
//						// remaining == proposedLength - length
//						double length = 0;
//						// If lesser (0) is empty, no lesser length is supported.
//						// If greater (1) is empty, no greater length is supported.
//						// Greater length will not be empty unless no subpane allows a larger size.
//						std::optional<double> other[2];
//						// If true, other greater is the minimum size.  Lesser will be blank.
//						bool otherAtMin = false;
//					};
//					container_dlist<projection> projections;
//					auto currentProjectionItor = projections.append(projection());
//					projection* currentProjection = currentProjectionItor.get();
//
//					auto apply_other = [&](projection& p, const std::optional<double>& otherLesser, const std::optional<double>& otherGreater)
//					{
//						if (p.other[0].has_value())
//						{
//							if (otherLesser.has_value())
//							{
//								p.other[0] = std::max(p.other[0].value(), otherLesser.value());	// Use largest lesser
//								if (otherGreater.has_value())
//								{
//									if (p.other[1].has_value())
//										p.other[1] = std::min(p.other[1].value(), otherGreater.value());	// Use lesser greater
//									else
//										p.other[1] = otherGreater.value();
//								}
//							}
//							else // We are imposting a min where there was not one previously.
//							{
//								p.other[0].reset();
//								COGS_ASSERT(otherGreater.has_value());
//								p.other[1] = otherGreater.value();
//							}
//						}
//						else if (!otherLesser.has_value())
//						{
//							// projected has minimum (no lesser) size. Only change greater if we are imposing a larger minimum size.
//							COGS_ASSERT(p.other[1].has_value());
//							COGS_ASSERT(otherGreater.has_value());
//							p.other[1] = std::max(p.other[1].value(), otherGreater.value());	// Use larger min
//						}
//					};
//
//					// Used to ingest a result known to have the same length in each (usually min, default, or max).
//					// Does not require creating additional projections.  Just needs to ingest the other lesser and greater.
//					auto ingest_one_result = [&]()
//					{
//						COGS_ASSERT(result.has_only(m_orientation, d)); // It should accept its own min/default/max
//						std::optional<double> otherLesser;
//						std::optional<double> otherGreater;
//						if (result.sizes[0].has_value())
//							otherLesser = result.sizes[0].value()[!m_orientation];
//						if (result.sizes[3].has_value())
//							otherGreater = result.sizes[3].value()[!m_orientation];
//
//						apply_other(*currentProjection, otherLesser, otherGreater);
//					};
//
//					auto ingest_result = [&]()
//					{
//						struct adjusted_result
//						{
//							double length;
//							std::optional<double> other[2];
//						};
//						adjusted_result adjustedResults[3];
//						size_t numAdjustedResults = 0;
//
//						auto ingest_other = [&](const std::optional<size>& sz, bool usingOtherGreater)
//						{
//							if (sz.has_value())
//							{
//								double length = sz.value()[m_orientation];
//								double other = sz.value()[!m_orientation];
//								// If we already are tracking an adjusted result with this length, update it.
//								for (int i = 0; i < numAdjustedResults; i++)
//								{
//									if (adjustedResults[i].length == length)
//									{
//										std::optional<double>& otherOpt = adjustedResults[i].other[usingOtherGreater];
//										if (!otherOpt.has_value() || (usingOtherGreater == (otherOpt.value() > other)))
//											otherOpt = other;
//										return;
//									}
//								}
//								adjusted_result& adjustedResult = adjustedResults[numAdjustedResults++];
//								adjustedResult.length = length;
//								adjustedResult.other[usingOtherGreater] = other;
//							}
//						};
//						ingest_other(result.sizes[0], false);
//						ingest_other(result.sizes[1], m_orientation == dimension::horizontal);
//						ingest_other(result.sizes[2], m_orientation == dimension::horizontal);
//						ingest_other(result.sizes[3], true);
//						bool otherAtMin = !result.sizes[0].has_value() && !result.sizes[(m_orientation == dimension::horizontal ? 2 : 1)].has_value();
//
//						// For each different length result, add a new projection.
//						for (int i = 0; i < numAdjustedResults; i++)
//						{
//							adjusted_result& adjustedResult = adjustedResults[i];
//							double newLength = currentProjection->length + adjustedResult.length;
//							// If there is any projection in the list prior to this one with the same length, don't add this one.
//							bool found = false;
//							auto itor = projections.get_first();
//							while (itor != currentProjectionItor)
//							{
//								if (itor->length == newLength)
//								{
//									found = true;
//									break;
//								}
//								++itor;
//							}
//							if (found)
//								continue;
//							itor = projections.insert_after(currentProjectionItor, {});
//							itor->length = newLength;
//							apply_other(*itor, adjustedResult.other[0], adjustedResult.other[1]);
//						}
//					};
//
//					remaining = proposedLength;
//					bool shrinking = proposedLength < defaultLength;
//					if (shrinking)
//					{
//						auto itor = m_shrinkMap.get_first();
//						while (!!itor && itor->value.m_allLesserDefault + itor->value.m_range.get_min() >= remaining)
//						{
//							// If shrinking the entire shrinkSet to min and still not reduced enough
//							// to allow all lesser sets to be default size, immediately use mins
//							// and move on to next set.
//							for (auto& child : itor->value.m_panes)
//							{
//								subpane& p = *(child.static_cast_to<subpane>());
//								double d = p.get_range().get_min()[m_orientation];
//								p.set_sized_length(d);
//								remaining -= d;
//								size proposing;
//								proposing[m_orientation] = d;
//								proposing[!m_orientation] = proposedOther;
//								result = p.propose_size(proposing, m_orientation);
//								if (result.is_empty())
//									return result;
//								ingest_one_result();
//							}
//							++itor;
//						}
//
//						while (!!itor)
//						{
//							auto& shrinkSet = itor->value;
//							// Since subpanes in the same set are interdependent, the whole set is
//							// sized by a sizing group before any sizes are proposed.
//							shrinkSet.calculate(remaining, m_orientation, m_disposition);
//
//							for (auto& child : shrinkSet.m_panes)
//							{
//								subpane& p = *(child.static_cast_to<subpane>());
//								size proposedSize;
//								proposedSize[m_orientation] = p.get_sized_length();
//								proposedSize[!m_orientation] = newSize[!m_orientation];
//								result = p.propose_size(proposedSize, m_orientation);
//								// Apply this result to all projections
//								// Need to track remaining min/max/default, to ensure projection does not become invalid
//							}
//
//							remaining -= shrinkSet.m_lastProposed;
//
//							// Propose sizes
//							for (auto& child : shrinkSet.m_panes)
//							{
//								subpane& p = *(child.static_cast_to<subpane>());
//								size proposedSize;
//								proposedSize[m_orientation] = p.get_sized_length();
//								proposedSize[!m_orientation] = newSize[!m_orientation];
//								p.m_lastProposedSizeResult = p.propose_size(proposedSize, m_orientation);
//							}
//						}
//					}
//					else // stretching
//					{
//						auto itor = m_stretchMap.get_first();
//						while (!!itor && itor->value.m_range.has_max() && itor->value.m_allLesserDefault + itor->value.m_range.get_max() <= remaining)
//						{
//							// If stretching the entire stretchSet to max and still not increased enough
//							// to allow all lower priority sets to be default size, immediately use max
//							// and move on to next set.
//							for (auto& child : itor->value.m_panes)
//							{
//								subpane& p = *(child.static_cast_to<subpane>());
//								COGS_ASSERT(p.get_range().has_max(m_orientation));
//								double d = p.get_range().get_max()[m_orientation];
//								p.set_sized_length(d);
//								remaining -= d;
//								size proposing;
//								proposing[m_orientation] = d;
//								proposing[!m_orientation] = proposedOther;
//								result = p.propose_size(proposing, m_orientation);
//								if (result.is_empty())
//									return result;
//								ingest_one_result();
//							}
//							++itor;
//						}
//
////#error
//						// If stretching, process nodes in order of next available to stretch.
//						for (auto& stretchSetPair : m_stretchMap)
//						{
//							auto& stretchSet = stretchSetPair.value;
//							if (stretchSet.m_range.has_max() && stretchSet.m_allLesserDefault + stretchSet.m_range.get_max() <= remaining)
//							{
//								// If stretching the entire stretchSet to max and still not extended enough
//								// to allow all lesser sets to be default size, immediately use maxes
//								// and move on to next set.
//								for (auto& child : stretchSet.m_panes)
//								{
//									subpane& p = *(child.static_cast_to<subpane>());
//									p.set_sized_length(p.get_range().get_max()[m_orientation]);
//								}
//							}
//							else
//							{
//								// There is enough room for this set to be less than maximum.
//								// And, since we are shrinking, less than defaults.
//								// Put this set in a sizing group and compute the sizing.
//								stretchSet.calculate(remaining, m_orientation, m_disposition);
//							}
//							remaining -= stretchSet.m_lastProposed;
//
//							// Propose sizes
//							for (auto& child : stretchSet.m_panes)
//							{
//								subpane& p = *(child.static_cast_to<subpane>());
//								size proposedSize;
//								proposedSize[m_orientation] = p.get_sized_length();
//								proposedSize[!m_orientation] = newSize[!m_orientation];
//								p.m_lastProposedSizeResult = p.propose_size(proposedSize, resizeDimension);
//							}
//						}
//					}
//				}
//
//				// Ideal widths have now been decided upon.  However, we still need to propose them.
//				// Once we do, we may find we need to make some adjustements.  :(
//				// TBD
//			}
//			else
//			{
//				// Handle resizing of !m_orientation dimension
//			}
//
//			result.set(newSize);
//			result.set_relative_to(sz);
//		}
//		return result;
//	}
//
//	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
//	{
//
//	}
//};
//
//
//}
//}
//
//
//#endif
