//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_OS_GUI_TEXT_EDITOR
#define COGS_HEADER_OS_GUI_TEXT_EDITOR



#include "cogs/os/gui/nsview.hpp"
#include "cogs/gui/text_editor.hpp"


namespace cogs {
namespace gui {
namespace os {


class text_editor;


};
};
};



@interface objc_text_editor : NSTextField <NSTextFieldDelegate>
{
@public
	cogs::weak_rcptr< cogs::gui::os::text_editor> m_cppTextEditor;
	NSString* m_text;
}

-(BOOL)acceptsFirstResponder;
-(BOOL)becomeFirstResponder;

@end
//-(BOOL)control: (NSControl *)control textView:(NSTextView *)textView doCommandBySelector: (SEL)commandSelector;



namespace cogs {
namespace gui {
namespace os {



class text_editor : public nsview_pane<text_editor_interface>
{
private:
	typedef nsview_pane<text_editor_interface> base_t;

	color m_defaultTextColor;
	color m_currentTextColor;

	rcptr<gfx::os::graphics_context::font>	m_cachedFont;

public:
	text_editor(const rcref<volatile nsview_subsystem>& uiSubsystem)
		: base_t(uiSubsystem)
	{ }

	~text_editor()
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
		objcTextEditor->m_cppTextEditor.release();
	}

	virtual void installing()
	{
		rcptr<gui::text_editor> te = get_bridge().template static_cast_to<gui::text_editor>();

		te->set_completely_invalidate_on_reshape(true);

		objc_text_editor* objcTextEditor = [[objc_text_editor alloc] init];
		objcTextEditor->m_cppTextEditor = this_rcptr;
		[objcTextEditor setDelegate:objcTextEditor];
		[objcTextEditor setEditable:YES];
		[objcTextEditor setSelectable:YES];
		[objcTextEditor setBezeled:NO];

		NSColor* nsColor = [objcTextEditor textColor];
		nsColor = [nsColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
		m_defaultTextColor = gfx::os::graphics_context::from_NSColor(nsColor);

		if (!te->is_multi_line())
			;	// ??

		base_t::installing(objcTextEditor);
            
		set_font(te->get_font());
		set_text(te->get_text());
		set_max_length(te->get_max_length());

		//bool overrideColor;
		//color c = te->get_background_color(overrideColor);
		//if (!!overrideColor)
		//	set_background_color(c);
		//else
		//	set_background_color(color::white);

		c = te->get_text_color(overrideColor);
		if (!!overrideColor)
			m_currentTextColor = c;
		else
			m_currentTextColor = m_defaultTextColor;	// NSSystemColorsDidChangeNotification ?
	}

	//virtual void set_background_color(const color& c)
	//{
	//	objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
	//	base_t::set_background_color(c);
	//	if (c.is_fully_transparent())
	//		[objcTextEditor setDrawsBackground:NO];
	//	else
	//	{
	//		NSColor* backColor = cogs::gfx::os::graphics_context::make_NSColor(c);
	//		[objcTextEditor setDrawsBackground:YES];
	//		[objcTextEditor setBackgroundColor:backColor];
	//	}
	//}

	virtual void set_text_color(const color& c)
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
		m_currentTextColor = c;
		NSColor* curTextColor = cogs::gfx::os::graphics_context::make_NSColor(c);
		[objcTextEditor setTextColor:curTextColor];
	}

	//virtual void clear_background_color()
	//{
	//	base_t::set_background_color(color::transparent);
	//}

	virtual void clear_text_color()
	{
		m_currentTextColor = m_defaultTextColor;
	}

	virtual void set_text(const composite_string& text)
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
		NSString* text2 = string_to_NSString(text);
		[objcTextEditor setStringValue:text2];
		[text2 release];
	}

	virtual void set_max_length(size_t numChars)
	{
	}

	virtual composite_string get_text() const
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
		NSString* str = [objcTextEditor stringValue];
		return NSString_to_string(str);
	}

	virtual void set_enabled(bool isEnabled = true)
	{
	}

	virtual void set_font(const gfx::font& fnt)
	{
		m_cachedFont = load_font(fnt).template static_cast_to<gfx::os::graphics_context::font>();
	}
	
	virtual size get_default_size() const	{ return size(100, 100); }

	virtual bool is_focusable() const	{ return true; }

//	virtual void focusing()
//	{
//		objc_text_editor* objcTextEditor = (objc_text_editor*)m_nsView;
//		if (!!objcTextEditor)
//			[[objcTextEditor window] makeFirstResponder: objcTextEditor];
//	}
};


}
}
}


#endif
