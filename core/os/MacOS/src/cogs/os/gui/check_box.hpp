//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_CHECK_BOX
#define COGS_HEADER_OS_GUI_CHECK_BOX



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


class check_box : public nsview_pane, public check_box_interface
{
private:
	rcptr<gfx::os::graphics_context::font> m_cachedFont;
	size m_defaultSize;

public:
	check_box(const ptr<rc_obj_base>& desc, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem)
	{ }

	virtual bool is_checked() const
	{
		objc_check_box* objcCheckBox = (objc_check_box*)get_NSView();
		return [objcCheckBox state] == NSControlStateValueOn;
	}

	virtual void set_text(const composite_string& text)
	{
		objc_check_box* objcCheckBox = (objc_check_box*)get_NSView();
		__strong NSString* text2 = string_to_NSString(text);
		[objcCheckBox setTitle:text2];
	}
	
	virtual void set_checked(bool b)
	{
		objc_check_box* objcCheckBox = (objc_check_box*)get_NSView();
		if (b)
			[objcCheckBox setState: NSControlStateValueOn];
		else
			[objcCheckBox setState: NSControlStateValueOff];
	}

	virtual void set_enabled(bool isEnabled = true)
	{
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = load_font(fnt).template static_cast_to<gfx::os::graphics_context::font>();
		objc_check_box* objcCheckBox = (objc_check_box*)get_NSView();
		NSButtonCell* buttonCell = [objcCheckBox cell];
		NSFont* nsFont = m_cachedFont->get_NSFont();
		[buttonCell setFont:nsFont];
	}

	virtual void installing()
	{
		rcptr<gui::check_box> cb = get_bridge().template static_cast_to<gui::check_box>();

		__strong objc_check_box* objcCheckBox = [[objc_check_box alloc] init];
		[objcCheckBox setButtonType: NSButtonTypeSwitch];
		objcCheckBox->m_cppCheckBox = this_rcptr;

		install_NSView(objcCheckBox);
		nsview_pane::installing();

		set_text(cb->get_text());
		set_checked(cb->is_checked());
		set_font(cb->get_font());
		set_enabled(cb->is_enabled());
	}

	virtual void calculate_range()
	{
		objc_check_box* objcCheckBox = (objc_check_box*)get_NSView();
		NSButtonCell* buttonCell = [objcCheckBox cell];
		NSSize idealSize = [buttonCell cellSize];
		double w = (double)idealSize.width;
		double h = (double)idealSize.height;
		m_defaultSize.set(w, h);
	}

	virtual range get_range() const { return range(m_defaultSize); }
	virtual size get_default_size() const { return m_defaultSize; }

	virtual bool is_focusable() const { return true; }
};


inline std::pair<rcref<bridgeable_pane>, rcref<check_box_interface> > nsview_subsystem::create_check_box() volatile
{
	rcref<check_box> cb = rcnew(check_box, this_rcref);
	return std::make_pair(cb, cb);
}


}
}
}


#ifdef COGS_OBJECTIVE_C_CODE


@implementation objc_check_box
@end


#endif


#endif
