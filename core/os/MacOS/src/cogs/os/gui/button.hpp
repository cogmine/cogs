//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_BUTTON
#define COGS_HEADER_OS_GUI_BUTTON


#include "cogs/os/gui/nsview.hpp"
#include "cogs/gui/button.hpp"


namespace cogs {
namespace gui {
namespace os {


class button;


};
};
};


@interface objc_button : NSButton
{
@public
	cogs::weak_rcptr< cogs::gui::os::button> m_cppButton;
}

-(void)on_click:(id)sender;

@end



namespace cogs {
namespace gui {
namespace os {



class button : public nsview_pane<button_interface>
{
private:
	typedef nsview_pane<button_interface> base_t;

	rcptr<gfx::os::graphics_context::font>	m_cachedFont;
	size								m_defaultSize;

public:
	button(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: base_t(uiSubsystem)
	{ }

	~button()
	{
		objc_button* objcButton = (objc_button*)m_nsView;
		objcButton->m_cppButton.release();
	}

	void action()
	{
		rcptr<gui::button> btn = get_bridge().static_cast_to<gui::button>();
		if (!!btn)
			btn->action();
	}

	virtual void set_text(const composite_string& text)
	{
		objc_button* objcButton = (objc_button*)m_nsView;
		NSString* text2 = string_to_NSString(text);
		[objcButton setTitle:text2];
		[text2 release];
	}

	virtual void set_enabled(bool isEnabled = true)
	{
	}

	virtual void set_default(bool isDefault = true)
	{
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = load_font(fnt).static_cast_to<gfx::os::graphics_context::font>();
		objc_button* objcButton = (objc_button*)m_nsView;
		NSButtonCell* buttonCell = [objcButton cell];
		NSFont* nsFont = m_cachedFont->m_nsFont;
		[buttonCell setFont:nsFont];
	}

	virtual void installing()
	{
		rcptr<gui::button> btn = get_bridge().static_cast_to<gui::button>();
		btn->set_completely_invalidate_on_reshape(true);

		objc_button* objcButton = [[objc_button alloc] init];
		objcButton->m_cppButton = this_rcptr;
		[objcButton setButtonType:NSMomentaryLightButton];
		[objcButton setBordered:YES];
		[objcButton setBezelStyle:NSRoundedBezelStyle];
		[objcButton setTarget:objcButton];
		[objcButton setAction:@selector(on_click:)];

        base_t::installing(objcButton);
            
		set_text(btn->get_text());
		set_font(btn->get_font());
		set_enabled(btn->is_enabled());
		set_default(btn->is_default());
	}

	virtual void calculate_range()
	{
		objc_button* objcButton = (objc_button*)m_nsView;
		NSButtonCell* buttonCell = [objcButton cell];
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
