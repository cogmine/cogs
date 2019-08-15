//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_SCROLL_BAR
#define COGS_HEADER_GUI_SCROLL_BAR


#include "cogs/dependency_property.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace gui {


class scroll_bar_state
{
public:
	double m_max;
	double m_thumbSize;

	scroll_bar_state()
		:	m_max(0),
			m_thumbSize(0)
	{ }

	scroll_bar_state(const scroll_bar_state& s)
		:	m_max(s.m_max),
			m_thumbSize(s.m_thumbSize)
	{ }

	scroll_bar_state(double max, double thumbSize)
		:	m_max(max),
			m_thumbSize(thumbSize)
	{ }

	scroll_bar_state& operator=(const scroll_bar_state& s)	{ m_max = s.m_max; m_thumbSize = s.m_thumbSize; return *this; }

	bool operator==(const scroll_bar_state& s) const		{ return m_max == s.m_max && m_thumbSize == s.m_thumbSize; }
	bool operator!=(const scroll_bar_state& s) const		{ return m_max != s.m_max || m_thumbSize != s.m_thumbSize; }
};


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI scroll bar.
class scroll_bar_interface
{
public:
	virtual bool can_overlay() const = 0;
};

/// @ingroup GUI
/// @brief A GUI scroll bar
class scroll_bar : public pane_bridge
{
public:
	typedef function<void(const rcref<scroll_bar>&)>	scrolled_delegate_t;

private:
	rcptr<scroll_bar_interface> m_nativeScrollBar; 
	const dimension m_dimension;
	const bool m_isHiddenWhenInactive;
	
	typedef transactable<scroll_bar_state> transactable_t;
	volatile transactable_t m_state;
	volatile double m_pos;

	delayed_construction<delegated_dependency_property<scroll_bar_state> > m_stateProperty;
	delayed_construction<delegated_dependency_property<double> > m_positionProperty;

public:
	explicit scroll_bar(const ptr<rc_obj_base>& desc, dimension d = dimension::vertical, bool isHiddenWhenInactive = false, const scroll_bar_state& s = scroll_bar_state(0, 0), double pos = 0 )
		: pane_bridge(desc),
		m_dimension(d),
		m_isHiddenWhenInactive(isHiddenWhenInactive),
		m_state(typename transactable_t::construct_embedded_t(), s),
		m_pos(pos)
	{
		auto stateGetter = [this]()
		{
			return *(m_state.begin_read());
		};

		auto stateSetter = [this](const scroll_bar_state& state)
		{
			scroll_bar_state newState = state;
			scroll_bar_state oldState = newState;
			m_state.swap_contents(oldState);
			if (newState != oldState)
				m_stateProperty.get().changed();
			m_stateProperty.get().set_complete();
		};

		placement_rcnew(&m_stateProperty.get(), this_desc, *this, std::move(stateGetter), std::move(stateSetter));

		auto positionGetter = [this]()
		{
			return atomic::load(m_pos);
		};

		auto positionSetter = [this](double d)
		{
			double newPos = d;
			double oldPos = atomic::exchange(m_pos, newPos);
			if (newPos != oldPos)
				m_positionProperty.get().changed();
			m_positionProperty.get().set_complete();
		};

		placement_rcnew(&m_positionProperty.get(), this_desc, *this, std::move(positionGetter), std::move(positionSetter));
	}
	
	dimension get_dimension() const	{ return m_dimension; }
	bool is_hidden_when_inactive() const	{ return m_isHiddenWhenInactive; }

	virtual bool can_overlay() const
	{
		return m_nativeScrollBar->can_overlay();
	}

	virtual void installing()
	{
		auto nativeScrollBar = get_subsystem()->create_scroll_bar();
		m_nativeScrollBar = std::move(nativeScrollBar.second);
		pane_bridge::install_bridged(std::move(nativeScrollBar.first));
	}
	
	virtual void uninstalling()
	{
		pane_bridge::uninstalling();
		m_nativeScrollBar.release();
	}

	virtual rcref<dependency_property<scroll_bar_state> >	get_state_property() { return get_self_rcref(&m_stateProperty.get()).template static_cast_to<dependency_property<scroll_bar_state>>(); }
	virtual rcref<dependency_property<double> >			get_position_property() { return get_self_rcref(&m_positionProperty.get()).template static_cast_to<dependency_property<double>>(); }
};


}
}


#endif
