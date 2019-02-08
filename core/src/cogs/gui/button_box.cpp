//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Placeholder, WorkInProgress


#ifdef COGS_COMPILE_SOURCE


#include "cogs/function.hpp"
#include "cogs/geometry/sizing_groups.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/gui/grid.hpp"
#include "cogs/gui/wrap_list.hpp"
#include "cogs/gui/label.hpp"
#include "cogs/gui/button.hpp"
#include "cogs/gui/button_box.hpp"
#include "cogs/gui/window.hpp"


namespace cogs {
namespace gui {


// TODO: Move this to a more appropriate file:
rcref<bridgeable_pane> subsystem::create_native_pane() volatile
{
	return rcnew(bridgeable_pane);
}

// TODO: Move this to a more appropriate file:
rcref<task<void> > gui::windowing::subsystem::open(
	const composite_string& title,
	const rcref<pane>& p,
	const rcptr<frame>& rshpr,
	const function<bool()>& closeDelegate) volatile
{
	return open_window(title, p, rshpr, closeDelegate)->get_window_task();
}


//class default_button_box_button : public button_box_interface::button
//{
//public:
//	wrap_list<script_flow::right_to_left_top_to_bottom>::nest_token	m_nestToken;
//	rcref<gui::button>											m_uiButton;
//
//	default_button_box_button(const rcref<gui::button>& uiButton)
//		:	m_uiButton(uiButton)
//	{ }
//
//	virtual void set_action(const function<void()>& d)	{ m_uiButton->set_action(d); }
//	virtual void set_text(const composite_string& s)		{ m_uiButton->set_text(s); }
//	virtual void set_enabled(bool b)			{ m_uiButton->set_enabled(b); }
//};
//
//
//class default_button_box : public button_box_interface
//{
//private:
//	rcptr<default_button_box_button>						m_defaultButton;
//	rcref<grid<> >											m_grid;
//	rcref<wrap_list<script_flow::right_to_left_top_to_bottom> >	m_wrapList;
//
//public:
//	default_button_box()
//		:	m_grid(rcnew(grid<>)),
//			m_wrapList(rcnew(wrap_list<script_flow::right_to_left_top_to_bottom>, 0.5, 0.5))
//	{
//		m_grid->nest(m_wrapList, 0, 1);
//	}
//	
//	virtual void nest(const rcref<pane>& contentPane)
//	{
//		m_grid->nest(contentPane, 0, 0);
//	}
//
//	virtual rcref<button_box_interface::button> append_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button>	uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		m_wrapList->nest_last(uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> prepend_button(const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button>	uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		m_wrapList->nest_first(uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> insert_button_before(const rcref<button_box_interface::button>& before, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button>	uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		rcref<default_button_box_button> beforeBtn = before.static_cast_to<default_button_box_button>();
//		m_wrapList->nest_before(uiButton, beforeBtn->m_uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual rcref<button_box_interface::button> insert_button_after(const rcref<button_box_interface::button>& after, const function<void()>& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
//	{
//		rcref<gui::button>	uiButton = rcnew(gui::button, action, text, fnt, isEnabled, isDefault);
//		rcref<default_button_box_button> b = rcnew(default_button_box_button, uiButton);
//		rcref<default_button_box_button> afterBtn = after.static_cast_to<default_button_box_button>();
//		m_wrapList->nest_before(uiButton, afterBtn->m_uiButton);
//		if (isDefault)
//			m_defaultButton = b;
//		return b;
//	}
//
//	virtual void set_default_button(const rcptr<button_box_interface::button>& btn)
//	{
//		rcptr<default_button_box_button> b = btn.static_cast_to<default_button_box_button>();
//		if (m_defaultButton != b)
//		{
//			if (!!m_defaultButton)
//				m_defaultButton->m_uiButton->set_default(false);
//			m_defaultButton = b;
//			if (!!b)
//				b->m_uiButton->set_default(true);
//		}
//	}
//	
//	virtual void remove(const rcref<button_box_interface::button>& btn)
//	{
//		rcref<default_button_box_button> b = btn.static_cast_to<default_button_box_button>();
//		if (m_defaultButton == b)
//			m_defaultButton.release();
//		b->m_uiButton->detach();
//	}
//
//	virtual void clear_buttons()
//	{
//		m_defaultButton.release();
//		m_wrapList->clear();
//	}
//
//	virtual void installing()
//	{
//		rcptr<gui::button_box> bb = get_bridge().static_cast_to<gui::button_box>();
//		bb->pane::nest(m_wrapList);
//		//bb->panel::nest(m_grid);
//		button_box_interface::installing();
//	}
//
//	virtual void uninstalling()
//	{
//		m_wrapList->detach();
//		//m_grid->detach();
//		button_box_interface::uninstalling();
//	}
//};
//
//
//rcref<button_box_interface> subsystem::create_button_box() volatile
//{
//	return rcnew(default_button_box);
//}
//
//
//
}
}

#endif
