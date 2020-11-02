//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_TEXT_EDITOR
#define COGS_HEADER_OS_GUI_TEXT_EDITOR


#include "cogs/os/gui/nsview.hpp"
#include "cogs/gui/text_editor.hpp"


namespace cogs {
namespace os {


class text_editor;


};
};


@interface objc_text_editor : NSTextField <NSTextFieldDelegate>
{
@public
	cogs::weak_rcptr<cogs::os::text_editor> m_cppTextEditor;
	NSString* m_text;
}

-(BOOL)acceptsFirstResponder;
-(BOOL)becomeFirstResponder;

@end


namespace cogs {
namespace os {


class text_editor : public nsview_pane, public gui::text_editor_interface
{
private:
	rcptr<graphics_context::font> m_cachedFont;

public:
	explicit text_editor(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(uiSubsystem)
	{ }

	virtual void installing()
	{
		rcptr<gui::text_editor> te = get_bridge().template static_cast_to<gui::text_editor>();

		__strong objc_text_editor* objcTextEditor = [[objc_text_editor alloc] init];
		objcTextEditor->m_cppTextEditor = this_rcptr;
		[objcTextEditor setDelegate:objcTextEditor];
		[objcTextEditor setEditable:YES];
		[objcTextEditor setSelectable:YES];
		[objcTextEditor setBezeled:NO];

		install_NSView(objcTextEditor);

		set_text(te->get_text());
		set_font(te->get_font());
		set_max_length(te->get_max_length());
		set_text_color(te->get_text_color());

		nsview_pane::installing();
	}

	virtual void set_text_color(const std::optional<color>& c)
	{
		color c2 = c.has_value() ? *c : get_default_text_foreground_color();
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		__strong NSColor* c3 = make_NSColor(c2);
		[objcTextEditor setTextColor : c3] ;
	}

	virtual void set_text(const composite_string& text)
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		__strong NSString* text2 = string_to_NSString(text);
		[objcTextEditor setStringValue:text2];
	}

	virtual void set_max_length(size_t numChars)
	{
		// TBD
	}

	virtual composite_string get_text() const
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		return NSString_to_string([objcTextEditor stringValue]);
	}

	virtual void set_enabled(bool isEnabled = true)
	{
		// TBD
	}

	virtual void set_font(const gfx::font_parameters_list& fnt)
	{
		m_cachedFont = load_font(fnt).template static_cast_to<graphics_context::font>();
		// TBD
	}

	virtual bool is_focusable() const { return true; }

	void focus(int direction = 0)
	{
		rcptr<gui::text_editor> te = get_bridge().template static_cast_to<gui::text_editor>();
		if (!!te)
			pane_orchestrator::focus(*te, direction);
	}
};


inline std::pair<rcref<gui::bridgeable_pane>, rcref<gui::text_editor_interface> > nsview_subsystem::create_text_editor() volatile
{
	rcref<text_editor> te = rcnew(text_editor)(this_rcref);
	return std::make_pair(te, te);
}


}
}


#ifdef COGS_OBJECTIVE_C_CODE


@implementation objc_text_editor


-(BOOL)acceptsFirstResponder
{
	return YES;
}

-(BOOL)becomeFirstResponder
{
	cogs::rcptr<cogs::os::text_editor> cppTextEditor = m_cppTextEditor;
	if (!!cppTextEditor)
		cppTextEditor->focus();
	return [super becomeFirstResponder];
}

@end


#endif


#endif
