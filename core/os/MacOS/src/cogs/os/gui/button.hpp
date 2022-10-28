//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_BUTTON
#define COGS_HEADER_OS_GUI_BUTTON


#include "cogs/os/gui/nsview.hpp"
#include "cogs/gui/button.hpp"


namespace cogs {
namespace os {


class button;


};
};


@interface objc_button : NSButton
{
@public
	cogs::weak_rcptr<cogs::os::button> m_cppButton;
}

-(void)on_click:(id)sender;

@end


namespace cogs {
namespace os {


class button : public nsview_pane, public gui::button_interface
{
private:
	rcptr<graphics_context::font> m_cachedFont;
	gfx::size m_defaultSize;

public:
	explicit button(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(uiSubsystem)
	{ }

	~button()
	{
		objc_button* objcButton = (objc_button*)get_NSView();
		if (!!objcButton)
			objcButton->m_cppButton.release();
	}

	void action()
	{
		rcptr<gui::button> btn = get_bridge().template static_cast_to<gui::button>();
		if (!!btn)
			btn->action();
	}

	virtual void set_text(const composite_string& text)
	{
		objc_button* objcButton = (objc_button*)get_NSView();
		__strong NSString* text2 = string_to_NSString(text);
		[objcButton setTitle:text2];
	}

	virtual void set_enabled(bool isEnabled = true)
	{
	}

	virtual void set_default(bool isDefault = true)
	{
	}

	virtual void set_font(const gfx::font_parameters_list& fnt)
	{
		m_cachedFont = load_font(fnt).template static_cast_to<graphics_context::font>();
		objc_button* objcButton = (objc_button*)get_NSView();
		NSButtonCell* buttonCell = [objcButton cell];
		NSFont* nsFont = m_cachedFont->get_NSFont();
		[buttonCell setFont:nsFont];
	}

	virtual void installing()
	{
		rcptr<gui::button> btn = get_bridge().template static_cast_to<gui::button>();

		__strong objc_button* objcButton = [[objc_button alloc] init];
		objcButton->m_cppButton = this_rcptr;
		[objcButton setButtonType: NSButtonTypeMomentaryLight];
		[objcButton setBordered:YES];
		[objcButton setBezelStyle: NSBezelStyleRegularSquare];
		[objcButton setTarget:objcButton];
		[objcButton setAction:@selector(on_click:)];

		install_NSView(objcButton);
		set_text(btn->get_text());
		set_font(btn->get_font());
		set_enabled(btn->is_enabled());
		set_default(btn->is_default());
		nsview_pane::installing();
	}

	virtual void calculating_range()
	{
		nsview_pane::calculating_range();
		objc_button* objcButton = (objc_button*)get_NSView();
		NSButtonCell* buttonCell = [objcButton cell];
		NSSize idealSize = [buttonCell cellSize];
		double w = (double)idealSize.width;
		double h = (double)idealSize.height;
		m_defaultSize.set(w, h);
	}

	virtual gfx::range get_range() const { return gfx::range(m_defaultSize); }
	virtual std::optional<gfx::size> get_default_size() const { return m_defaultSize; }

	virtual bool is_focusable() const { return true; }
};


inline std::pair<rcref<gui::bridgeable_pane>, rcref<gui::button_interface> > nsview_subsystem::create_button() volatile
{
	rcref<button> b = rcnew(button)(this_rcref);
	return std::make_pair(b, b);
}


}
}


#ifdef COGS_OBJECTIVE_C_CODE


@implementation objc_button

- (void)on_click:(id)sender
{
	cogs::rcptr<cogs::os::button> cppButton = m_cppButton;
	if (!!cppButton)
		cppButton->action();
}

@end


#endif


#endif
