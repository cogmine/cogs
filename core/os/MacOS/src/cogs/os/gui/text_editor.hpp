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



class text_editor : public nsview_pane, public text_editor_interface
{
private:
	//color m_defaultTextColor;
	//color m_currentTextColor;

	rcptr<gfx::os::graphics_context::font>	m_cachedFont;

public:
	text_editor(const ptr<rc_obj_base>& desc, const rcref<volatile nsview_subsystem>& uiSubsystem)
		: nsview_pane(desc, uiSubsystem)
	{ }

	//~text_editor()
	//{
		//objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		//objcTextEditor->m_cppTextEditor.release();
	//}

	virtual void installing()
	{
		rcptr<gui::text_editor> te = get_bridge().template static_cast_to<gui::text_editor>();

		objc_text_editor* objcTextEditor = [[objc_text_editor alloc] init];
		objcTextEditor->m_cppTextEditor = this_rcptr;
		[objcTextEditor setDelegate:objcTextEditor];
		[objcTextEditor setEditable:YES];
		[objcTextEditor setSelectable:YES];
		[objcTextEditor setBezeled:NO];

		//__strong NSColor* nsColor = [objcTextEditor textColor];
		//nsColor = [nsColor colorUsingColorSpace:NSCalibratedRGBColorSpace];
		//m_defaultTextColor = gfx::os::graphics_context::from_NSColor(nsColor);

		//if (!te->is_multi_line())
		//	;	// ??

		install_NSView(objcTextEditor);

		//////set_font(te->get_font());
		//////set_text(te->get_text());
		//////set_max_length(te->get_max_length());

		//bool overrideColor;
		//color c = te->get_background_color(overrideColor);
		//if (!!overrideColor)
		//	set_background_color(c);
		//else
		//	set_background_color(color::white);

		color c = te->get_text_color();
		__strong NSColor* c2 = make_NSColor(c);
		[objcTextEditor setTextColor: c2];

		//if (!!overrideColor)
		//	m_currentTextColor = c;
		//else
		//	m_currentTextColor = m_defaultTextColor;	// NSSystemColorsDidChangeNotification ?

		nsview_pane::installing();
	}

	//virtual void set_background_color(const color& c)
	//{
	//	objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
	//	nsview_pane::set_background_color(c);
	//	if (c.is_fully_transparent())
	//		[objcTextEditor setDrawsBackground:NO];
	//	else
	//	{
	//		__strong NSColor* backColor = cogs::gfx::os::graphics_context::make_NSColor(c);
	//		[objcTextEditor setDrawsBackground:YES];
	//		[objcTextEditor setBackgroundColor:backColor];
	//	}
	//}

	virtual void set_text_color(const color& c)
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		//m_currentTextColor = c;
		__strong NSColor* c2 = cogs::gfx::os::graphics_context::make_NSColor(c);
		[objcTextEditor setTextColor:c2];
	}

	//virtual void clear_background_color()
	//{
	//	nsview_pane::set_background_color(color::transparent);
	//}

	//virtual void clear_text_color()
	//{
	//	m_currentTextColor = m_defaultTextColor;
	//}

	virtual void set_text(const composite_string& text)
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
		__strong NSString* text2 = string_to_NSString(text);
		[objcTextEditor setStringValue:text2];
		//[text2 release];
	}

	virtual void set_max_length(size_t numChars)
	{
	}

	virtual composite_string get_text() const
	{
		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
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
//		objc_text_editor* objcTextEditor = (objc_text_editor*)get_NSView();
//		if (!!objcTextEditor)
//			[[objcTextEditor window] makeFirstResponder: objcTextEditor];
//	}
};


inline std::pair<rcref<bridgeable_pane>, rcref<text_editor_interface> > nsview_subsystem::create_text_editor() volatile
{
	rcref<text_editor> te = rcnew(text_editor, this_rcref);
	return std::make_pair(te, te);
}


}
}
}


#endif
