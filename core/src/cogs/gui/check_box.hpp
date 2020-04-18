//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_CHECK_BOX
#define COGS_HEADER_GUI_CHECK_BOX


#include "cogs/gfx/color.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/gui/subsystem.hpp"


namespace cogs {
namespace gui {


/// @ingroup GUI
/// @brief Interface for a bridgeable GUI check box.
class check_box_interface
{
public:
	virtual bool is_checked() const = 0;

	virtual void set_text(const composite_string&) = 0;
	virtual void set_checked(bool) = 0;
	virtual void set_enabled(bool) = 0;
	virtual void set_font(const gfx::font&) = 0;
};

/// @ingroup GUI
/// @brief A GUI check box
class check_box : public pane_bridge
{
public:
	typedef function<void(const rcref<check_box>&)> action_delegate_t;

private:
	composite_string m_text;
	bool m_isEnabled;
	bool m_isChecked;
	gfx::font m_font;
	action_delegate_t m_action;
	rcptr<check_box_interface> m_nativeCheckBox;

public:
	struct options
	{
		composite_string text;
		bool isEnabled = true;
		bool isChecked = false;
		gfx::font font;
		action_delegate_t action;
		frame_list frames;
	};

	check_box()
		: check_box(options())
	{ }

	explicit check_box(options&& o)
		: pane_bridge({
			.frames = std::move(o.frames)
		}),
		m_text(std::move(o.text)),
		m_isEnabled(o.isEnabled),
		m_isChecked(o.isChecked),
		m_font(std::move(o.font)),
		m_action(std::move(o.action))
	{ }

	virtual void installing()
	{
		auto nativeCheckBox = get_subsystem()->create_check_box();
		pane_bridge::install_bridged(std::move(nativeCheckBox.first));
		m_nativeCheckBox = std::move(nativeCheckBox.second);
	}

	virtual void uninstalling()
	{
		if (!!m_nativeCheckBox)
			m_isChecked = m_nativeCheckBox->is_checked();
		pane_bridge::uninstalling();
		m_nativeCheckBox.release();
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
		if (!m_nativeCheckBox)
			m_text = text;
		else
		{
			m_nativeCheckBox->set_text(text);
			recompose();
		}
	}

	bool is_enabled() const { return m_isEnabled; }
	void set_enabled(bool b)
	{
		if (!!m_nativeCheckBox)
			m_nativeCheckBox->set_enabled(b);
		else
			m_isEnabled = b;
	}

	void set_checked(bool b)
	{
		if (!!m_nativeCheckBox)
			m_nativeCheckBox->set_checked(b);
		else if (b != m_isChecked)
		{
			m_isChecked = b;
			action();
		}
	}

	bool is_checked() const
	{
		if (!!m_nativeCheckBox)
			return m_nativeCheckBox->is_checked();
		return m_isChecked;
	}

	const gfx::font& get_font() const { return m_font; }
	void set_font(const gfx::font& fnt)
	{
		m_font = fnt;
		if (!!m_nativeCheckBox)
		{
			m_nativeCheckBox->set_font(fnt);
			recompose();
		}
	}
};


}
}


#endif
