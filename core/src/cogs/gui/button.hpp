//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_BUTTON
#define COGS_HEADER_GUI_BUTTON


#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI button.
class button_interface
{
public:
	virtual void set_text(const composite_string&) = 0;
	virtual void set_enabled(bool) = 0;
	virtual void set_default(bool) = 0;
	virtual void set_font(const gfx::font&) = 0;
};

/// @ingroup GUI
/// @brief A GUI button
class button : public pane_bridge
{
public:
	typedef function<void(const rcref<button>&)> action_delegate_t;

private:
	action_delegate_t m_action;
	composite_string m_text;
	gfx::font m_font;
	bool m_isEnabled;
	bool m_isDefault;
	rcptr<button_interface> m_nativeButton;

public:
	button(rc_obj_base& desc, const action_delegate_t& action, const composite_string& text, const gfx::font& fnt = gfx::font(), bool isEnabled = true, bool isDefault = false)
		: pane_bridge(desc),
		m_action(action),
		m_text(text),
		m_font(fnt),
		m_isEnabled(isEnabled),
		m_isDefault(isDefault)
	{ }

	virtual void installing()
	{
		auto nativeButton = get_subsystem()->create_button();
		m_nativeButton = std::move(nativeButton.second);
		pane_bridge::install_bridged(std::move(nativeButton.first));
	}

	virtual void uninstalling()
	{
		pane_bridge::uninstalling();
		m_nativeButton.release();
	}

	virtual void action()
	{
		if (!!m_action)
			m_action(this_rcref);
	}

	virtual void set_action(const action_delegate_t& newAction)
	{
		m_action = newAction;
	}

	const composite_string& get_text() const { return m_text; }
	void set_text(const composite_string& text)
	{
		m_text = text;
		if (!!m_nativeButton)
		{
			m_nativeButton->set_text(text);
			recompose();
		}
	}

	bool is_enabled() const { return m_isEnabled; }
	void set_enabled(bool b)
	{
		m_isEnabled = b;
		if (!!m_nativeButton)
		{
			m_nativeButton->set_enabled(b);
			recompose();
		}
	}

	bool is_default() const { return m_isDefault; }
	void set_default(bool b)
	{
		m_isDefault = b;
		if (!!m_nativeButton)
		{
			m_nativeButton->set_default(b);
			recompose();
		}
	}

	const gfx::font& get_font() const { return m_font; }
	void set_font(const gfx::font& fnt)
	{
		m_font = fnt;
		if (!!m_nativeButton)
		{
			m_nativeButton->set_font(fnt);
			recompose();
		}
	}

};


}
}


#endif
