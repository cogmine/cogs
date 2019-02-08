//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_OS_CHECK_BOX
#define COGS_OS_CHECK_BOX



#include "cogs/os/gui/nsview.hpp"
#include "cogs/gui/check_box.hpp"


namespace cogs {
namespace gui {
namespace os {


class check_box;


};
};
};



@interface objc_check_box : NSButton
{
@public
	cogs::weak_rcptr< cogs::gui::os::check_box> m_cppCheckBox;
	bool m_state;
}

@end



namespace cogs {
namespace gui {
namespace os {


class check_box : public nsview_pane<check_box_interface>
{
private:
	typedef nsview_pane<check_box_interface> base_t;

	rcptr<gfx::os::graphics_context::font>	m_cachedFont;
	size								m_defaultSize;

public:
	check_box(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: base_t(uiSubsystem)
	{ }

	~check_box()
	{
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		objcCheckBox->m_cppCheckBox.release();
	}

	virtual bool is_checked() const
	{
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		return [objcCheckBox state] == NSOnState;
	}

	virtual void set_text(const composite_string& text)
	{
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		NSString* text2 = string_to_NSString(text);
		[objcCheckBox setTitle:text2];
		[text2 release];
	}
	
	virtual void set_checked(bool b)
	{
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		if (b)
			[objcCheckBox setState:NSOnState];
		else
			[objcCheckBox setState:NSOffState];
	}

	virtual void set_enabled(bool isEnabled = true)
	{
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = load_font(fnt).static_cast_to<gfx::os::graphics_context::font>();
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		NSButtonCell* buttonCell = [objcCheckBox cell];
		NSFont* nsFont = m_cachedFont->m_nsFont;
		[buttonCell setFont:nsFont];
	}

	virtual void installing()
	{
		rcptr<gui::check_box> cb = get_bridge().static_cast_to<gui::check_box>();
		cb->set_completely_invalidate_on_reshape(true);

		objc_check_box* objcCheckBox = [[objc_check_box alloc] init];
		[objcCheckBox setButtonType:NSSwitchButton];
		objcCheckBox->m_cppCheckBox = this_rcptr;

		base_t::installing(objcCheckBox);
            
		set_text(cb->get_text());
		set_checked(cb->is_checked());
		set_font(cb->get_font());
		set_enabled(cb->is_enabled());
	}

	virtual void calculate_range()
	{
		objc_check_box* objcCheckBox = (objc_check_box*)m_nsView;
		NSButtonCell* buttonCell = [objcCheckBox cell];
		NSSize idealSize = [buttonCell cellSize];
		double w = (double)idealSize.width;
		double h = (double)idealSize.height;
		m_defaultSize.set(w, h);
	}

	virtual range	get_range() const					{ return range(m_defaultSize); }
	virtual size		get_default_size() const			{ return m_defaultSize; }

	virtual bool is_focusable() const	{ return true; }
};


}
}
}


#endif
