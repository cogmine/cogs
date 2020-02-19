//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_SCROLL_BAR
#define COGS_HEADER_GUI_SCROLL_BAR


#include "cogs/dependency_property.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace gui {


class scroll_bar_interface;


class scroll_bar_state
{
public:
	double m_max;
	double m_thumbSize;

	scroll_bar_state()
		: m_max(0),
		m_thumbSize(0)
	{ }

	scroll_bar_state(const scroll_bar_state& s)
		: m_max(s.m_max),
		m_thumbSize(s.m_thumbSize)
	{ }

	scroll_bar_state(double max, double thumbSize)
		: m_max(max),
		m_thumbSize(thumbSize)
	{ }

	scroll_bar_state& operator=(const scroll_bar_state& s) { m_max = s.m_max; m_thumbSize = s.m_thumbSize; return *this; }

	bool operator==(const scroll_bar_state& s) const { return m_max == s.m_max && m_thumbSize == s.m_thumbSize; }
	bool operator!=(const scroll_bar_state& s) const { return m_max != s.m_max || m_thumbSize != s.m_thumbSize; }
};


/// @ingroup GUI
/// @brief A GUI scroll bar
class scroll_bar : public pane_bridge
{
public:
	typedef function<void(const rcref<scroll_bar>&)> scrolled_delegate_t;

private:
	const dimension m_dimension;
	const bool m_isHiddenWhenInactive;

	typedef transactable<scroll_bar_state> transactable_t;
	volatile transactable_t m_state;
	volatile double m_pos;
	volatile boolean m_canAutoFade;
	volatile boolean m_shouldAutoFade;

	delegated_dependency_property<scroll_bar_state> m_stateProperty;
	delegated_dependency_property<double> m_positionProperty;
	delegated_dependency_property<bool> m_canAutoFadeProperty; // reflects scroll bars capability to auto-fade
	delegated_dependency_property<bool> m_shouldAutoFadeProperty; // reflects callers choice to auto-fade

public:
	explicit scroll_bar(rc_obj_base& desc,
		dimension d = dimension::vertical,
		bool isHiddenWhenInactive = false,
		const scroll_bar_state& s = scroll_bar_state(0, 0),
		double pos = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: pane_bridge(desc, frames),
		m_dimension(d),
		m_isHiddenWhenInactive(isHiddenWhenInactive),
		m_state(typename transactable_t::construct_embedded_t(), s),
		m_pos(pos),
		m_stateProperty(desc, *this, [this]()
		{
			return *(m_state.begin_read());
		}, [this](const scroll_bar_state& state)
		{
			scroll_bar_state newState = state;
			scroll_bar_state oldState = newState;
			m_state.swap_contents(oldState);
			if (newState != oldState)
				m_stateProperty.changed();
			m_stateProperty.set_complete();
		}),
		m_positionProperty(desc, *this, [this]()
		{
			return atomic::load(m_pos);
		}, [this](double d)
		{
			double newPos = d;
			double oldPos = atomic::exchange(m_pos, newPos);
			if (newPos != oldPos)
				m_positionProperty.changed();
			m_positionProperty.set_complete();
		}),
		m_canAutoFadeProperty(desc, *this, [this]()
		{
			return m_canAutoFade;
		}, [this](bool b)
		{
			bool oldValue = m_canAutoFade.exchange(b);
			if (b != oldValue)
				m_canAutoFadeProperty.changed();
			m_canAutoFadeProperty.set_complete();
		}),
		m_shouldAutoFadeProperty(desc, *this, [this]()
		{
			return m_shouldAutoFade;
		}, [this](bool b)
		{
			bool oldValue = m_shouldAutoFade.exchange(b);
			if (b != oldValue)
				m_shouldAutoFadeProperty.changed();
			m_shouldAutoFadeProperty.set_complete();
		})
	{ }

	scroll_bar(rc_obj_base& desc,
		bool isHiddenWhenInactive,
		const scroll_bar_state& s = scroll_bar_state(0, 0),
		double pos = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, dimension::vertical, isHiddenWhenInactive, s, pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		const scroll_bar_state& s,
		double pos = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, d, false, s, pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		bool isHiddenWhenInactive,
		double pos,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, d, isHiddenWhenInactive, scroll_bar_state(0, 0), pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		bool isHiddenWhenInactive,
		const scroll_bar_state& s,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, d, isHiddenWhenInactive, s, 0, frames)
	{ }


	scroll_bar(rc_obj_base& desc,
		const scroll_bar_state& s,
		double pos = 0,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, dimension::vertical, false, s, pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		bool isHiddenWhenInactive,
		double pos,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, dimension::vertical, isHiddenWhenInactive, scroll_bar_state(0, 0), pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		bool isHiddenWhenInactive,
		const scroll_bar_state& s,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, dimension::vertical, isHiddenWhenInactive, s, 0, frames)
	{ }


	scroll_bar(rc_obj_base& desc,
		dimension d,
		double pos,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, d, false, scroll_bar_state(0, 0), pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		const scroll_bar_state& s,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, d, false, s, 0, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		bool isHiddenWhenInactive,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, d, isHiddenWhenInactive, scroll_bar_state(0, 0), 0, frames)
	{ }


	scroll_bar(rc_obj_base& desc,
		double pos,
		const std::initializer_list<rcref<frame> >& frames = {})
		: scroll_bar(desc, dimension::vertical, false, scroll_bar_state(0, 0), pos, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		const scroll_bar_state& s,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, dimension::vertical, false, s, 0, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		bool isHiddenWhenInactive,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, dimension::vertical, isHiddenWhenInactive, scroll_bar_state(0, 0), 0, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		dimension d,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, d, false, scroll_bar_state(0, 0), 0, frames)
	{ }

	scroll_bar(rc_obj_base& desc,
		const std::initializer_list<rcref<frame> >& frames)
		: scroll_bar(desc, dimension::vertical, false, scroll_bar_state(0, 0), 0, frames)
	{ }


	dimension get_dimension() const { return m_dimension; }
	bool is_hidden_when_inactive() const { return m_isHiddenWhenInactive; }

	virtual void installing()
	{
		pane_bridge::install_bridged(std::move(get_subsystem()->create_scroll_bar().first));
	}

	rcref<dependency_property<scroll_bar_state> > get_state_property() { return get_self_rcref(&m_stateProperty).template static_cast_to<dependency_property<scroll_bar_state> >(); }
	rcref<dependency_property<double> > get_position_property() { return get_self_rcref(&m_positionProperty).template static_cast_to<dependency_property<double> >(); }
	rcref<dependency_property<bool> > get_should_auto_fade_property() { return get_self_rcref(&m_shouldAutoFadeProperty).template static_cast_to<dependency_property<bool> >(); }
	rcref<dependency_property<bool, io::permission::read> > get_can_auto_fade_property() { return get_self_rcref(&m_canAutoFadeProperty).template static_cast_to<dependency_property<bool, io::permission::read> >(); }

	bool will_auto_fade() const { return m_canAutoFade && m_shouldAutoFade; }

	void scroll(double d)
	{
		double pos = atomic::load(m_pos);
		pos += d;
		if (pos < 0.0)
			pos = 0;
		else
		{
			auto rt = m_state.begin_read();
			double max = rt->m_max - rt->m_thumbSize;
			if (pos > max)
				pos = max;
		}
		m_positionProperty.set(pos);
	}

	friend class scroll_bar_interface;
};


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI scroll bar.
class scroll_bar_interface
{
protected:
	// Provides write access to read-only properties
	rcref<dependency_property<bool> > get_can_auto_fade_property(const rcref<scroll_bar>& sb) { return sb.member_cast_to(sb->m_canAutoFadeProperty).template static_cast_to<dependency_property<bool> >(); }
};


}
}


#endif
