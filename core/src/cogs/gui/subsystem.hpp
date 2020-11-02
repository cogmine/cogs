//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_SUBSYSTEM
#define COGS_HEADER_GUI_SUBSYSTEM


#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/sync/dispatcher.hpp"


#define COGS_CONSOLE 1
#define COGS_GUI 2

#ifndef COGS_DEFAULT_UI_SUBSYSTEM
#define COGS_DEFAULT_UI_SUBSYSTEM COGS_GUI
#endif


namespace cogs {
namespace ui {


/// @ingroup GUI
/// @brief UI subsystem
class subsystem : public dispatcher
{
public:
	// Presents a message to the user.
	// For example:
	// In a text console: Outputs the message text
	// In the default Windowing GUI: Displays the message in a message box dialog with "OK" button for dismissal.
	// Or, an audio UI: Say the message to the user
//	virtual rcref<task<void> > message(const composite_string& msg) volatile = 0;

	// Present the message to the user, and request they respond with one of the provided options
	// For example:
	// In a text console: Output the message text, and display a menu containing the specified options for the user to select from.
	// In the default Windows GUI: Displays the message in a button-box dialog, with the specified options as buttons.
	// Or, an audio UI: Say the message to the user, and listen for one of the specified options in response.
//	virtual rcref<task<composite_string> > prompt(const composite_string& msg, const vector<composite_string>& options) volatile = 0;

	virtual bool is_ui_thread_current() const volatile = 0;

	static rcref<subsystem> get_default();
};


//class console : public ui::subsystem
//{
//public:
//	virtual rcref<task<void> > message(const composite_string& msg) volatile = 0;
//	virtual rcref<task<composite_string> > prompt(const composite_string& msg, const vector<composite_string>& options) volatile = 0;
//};


} // namespace ui


namespace gui {

class pane;
class bridgeable_pane;
class frame;

class button;
class check_box;
class text_editor;
class scroll_bar;
class window;

class button_interface;
class check_box_interface;
class text_editor_interface;
class button_box_interface;
class scroll_bar_interface;
//class canvas3D_pane_interface;
class window_interface;


/// @ingroup GUI
/// @brief GUI subsystem
class subsystem : public ui::subsystem, public object
{
protected:
	friend class button;
	virtual std::pair<rcref<bridgeable_pane>, rcref<button_interface> > create_button() volatile = 0;

	friend class check_box;
	virtual std::pair<rcref<bridgeable_pane>, rcref<check_box_interface> > create_check_box() volatile = 0;

	friend class text_editor;
	virtual std::pair<rcref<bridgeable_pane>, rcref<text_editor_interface> > create_text_editor() volatile = 0;

	friend class scroll_bar;
	virtual std::pair<rcref<bridgeable_pane>, rcref<scroll_bar_interface> > create_scroll_bar() volatile = 0;

	friend class native_container_pane;
	virtual rcref<bridgeable_pane> create_native_pane() volatile;

	// Provides subsystem derived classes with access to pane::install()
	friend class pane;
	static void install(pane& p, const rcptr<volatile subsystem>& subSystem);

public:
	//virtual rcptr<ui::console> get_default_console() volatile;
	//virtual rcptr<ui::console> create_console() volatile;

	//virtual rcref<task<void> > message(const composite_string& msg) volatile;

	virtual rcref<task<void> > open(
		const rcref<pane>& p,
		const composite_string& title = composite_string{}) volatile = 0;

	//virtual rcptr<canvas3D_pane_interface> create_canvas3D() volatile { return rcptr<canvas3D_pane_interface>(); } // 3D unsupported by default

	static rcptr<subsystem> get_default();
};


namespace windowing {


/// @ingroup GUI
/// @brief Windowing GUI subsystem
class subsystem : public gui::subsystem
{
protected:
	using gui::subsystem::create_button;
	using gui::subsystem::create_check_box;
	using gui::subsystem::create_text_editor;
	using gui::subsystem::create_scroll_bar;

	friend class gui::window;
	virtual std::pair<rcref<bridgeable_pane>, rcref<window_interface> > create_window() volatile = 0;

public:
	// Returns BOUNDS of all active display devices, in actual pixels.
	// Primary screen is first.  Might be empty list, if server without monitor.
	virtual vector<gfx::bounds> get_screens() volatile = 0;

	virtual rcref<task<void> > open(
		const rcref<pane>& p,
		const composite_string& title = composite_string{}) volatile;

	// If provided, position is in screen coordinates (get_screens()) and center is ignored.
	// If position is not provided, and center is true, the window will be centered.
	// If position is not provided, and center is false, a platform default position may be used. If no default, it will be centered.
	// Either contentSize or frameSize may be provided, not both.  If both are provided, contentSize is used and frameSize ignored.
	// If neither contentSize or frameSize are provided, the default size of the content will be used.
	// Otherwise, a size will be proposed based on contentSize or frameSize, and the adjusted size will be used.
	struct window_options
	{
		composite_string title;
		std::optional<color> backgroundColor;
		std::optional<gfx::point> position;
		std::optional<gfx::size> contentSize;
		std::optional<gfx::size> frameSize;
		bool center = false;
		rcref<pane> content;
	};

	virtual rcref<gui::window> open_window(window_options&& options) volatile;

	//virtual rcref<task<void> > open_full_screen(
	//	const gfx::point& screenAtPosition, // Main display is 0,0
	//	const composite_string& title,
	//	const rcref<pane>& p) volatile = 0;

	static rcptr<subsystem> get_default();
};


}
}
}


#endif
