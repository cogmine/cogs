//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


#include "cogs/cogs.hpp"


namespace cogs
{
	int main(const rcref<gui::windowing::subsystem>&);
};

using namespace cogs;
using namespace cogs::gui;
using namespace cogs::io;
using namespace cogs::io::net;
using namespace cogs::io::net::ip;


class box : public background
{
public:
	color					m_baseColor;
	timeout_t::period_t		m_timeoutPeriod;
	rcptr<resettable_timer>	m_boxTimer;
	function<void()>		m_expireDelegate;
	function<void()>		m_expireInUiThreadDelegate;

	box(const color& c)
		:	background(c),
			m_timeoutPeriod(measure<int_type, milliseconds>(5)),
			m_baseColor(c),
			m_expireDelegate([r{ this_weak_rcptr }]()
			{
				rcptr<box> r2 = r;
				if (!!r2)
					r2->box_timer_expired();
			}),
			m_expireInUiThreadDelegate([r { this_weak_rcptr }]()
			{
				rcptr<box> r2 = r;
				if (!!r2)
					r2->box_timer_expired_in_ui_thread();
			})
	{ }

	virtual void installing()
	{
		pane::installing();
		m_boxTimer = resettable_timer::create(m_timeoutPeriod);
		m_boxTimer->dispatch(m_expireDelegate);
	}

	virtual void uninstalling()
	{
		m_boxTimer.release();
		pane::uninstalling();
	}

	void box_timer_expired()
	{
		pane::dispatch(m_expireInUiThreadDelegate, const_max_int<int>::value);
	}

	void box_timer_expired_in_ui_thread()
	{
		timeout_t::period_t p = timeout_t::now();
		measure<ulongest_type, milliseconds> n = p;
		unsigned short s = (unsigned short)(n.get().get_int() >> 3);
		unsigned char b = (unsigned char)(n.get().get_int() >> 3);
		if ((s >> 8) & 1)
			b = 255 - b;

		color c = m_baseColor;
		if (c.get_rgb().m_red != 0)
		{
			c.set_red(b);
		}
		if (c.get_rgb().m_green != 0)
		{
			c.set_green(b);
		}
		if (c.get_rgb().m_blue != 0)
		{
			c.set_blue(b);
		}

		set_color(c);

		rcptr<resettable_timer> t = m_boxTimer;
		if (!!t)
		{
			t->refire();
			t->dispatch(m_expireDelegate);
		}
	}
};

int cogs::main(const rcref<gui::windowing::subsystem>& guiSubsystem)
{
	//{
	//	rcptr<http::server> m_httpServer = rcnew(http::server);
	//	rcptr<smtp::server> m_smtpServer = rcnew(smtp::server);
	//	rcptr<tcp::listener> m_httpListener = ip::tcp::server_listen(m_httpServer.dereference(), 8080);
	//	rcptr<tcp::listener> m_smtpListener = ip::tcp::server_listen(m_smtpServer.dereference(), 8081);// 25);
	//	cleanup_queue::get_global()->add(m_httpServer);
	//	cleanup_queue::get_global()->add(m_smtpServer);
	//	cleanup_queue::get_global()->add(m_httpListener);
	//	cleanup_queue::get_global()->add(m_smtpListener);
	//}

	{
		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00, 0xFF));
		guiSubsystem->open(string::literal(L"Single content test"), box1)
			->dispatch([]() { cogs::request_quit(); })
			;
	}

	{
		rcref<background> blackBackgrounPane = rcnew(background, color::black);
		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00));

		rcref<fixed_size_frame> fixedSizeFrame = rcnew(fixed_size_frame, box1, gfx::canvas::size(200, 200));

		blackBackgrounPane->nest(box1, fixedSizeFrame);
		guiSubsystem->open(string::literal(L"Fixed size window"), blackBackgrounPane);
	}

	{
		rcref<background> blackBackgrounPane = rcnew(background, color::black);
		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00));

		rcref<fixed_size_frame> fixedSizeFrame = rcnew(fixed_size_frame, box1, gfx::canvas::size(200, 200));
		rcref<unconstrained_frame> unconstrainedFrame = rcnew(unconstrained_frame, fixedSizeFrame, geometry::planar::alignment::center());

		blackBackgrounPane->nest(box1, unconstrainedFrame);
		guiSubsystem->open(string::literal(L"Centering"), blackBackgrounPane);
	}

	{
		rcref<background> fourBoxPane = rcnew(background, color::black);

		rcref<pane> box1 = rcnew(box, color(0xFF, 0x00, 0x00, 0x7F));
		rcref<pane> box2 = rcnew(box, color(0x00, 0xFF, 0x00, 0x7F));
		rcref<pane> box3 = rcnew(box, color(0x00, 0x00, 0xFF, 0x7F));
		rcref<pane> box4 = rcnew(box, color(0xFF, 0xFF, 0x00, 0x7F));

		rcref<fixed_size_frame> box1Frame = rcnew(fixed_size_frame, box1, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box2Frame = rcnew(fixed_size_frame, box2, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box3Frame = rcnew(fixed_size_frame, box3, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box4Frame = rcnew(fixed_size_frame, box4, gfx::canvas::size(200, 200));

		fourBoxPane->nest(box1, rcnew(unconstrained_frame, box1Frame, geometry::planar::alignment::top_left()));
		fourBoxPane->nest(box2, rcnew(unconstrained_frame, box2Frame, geometry::planar::alignment::top_right()));
		fourBoxPane->nest(box3, rcnew(unconstrained_frame, box3Frame, geometry::planar::alignment::bottom_left()));
		fourBoxPane->nest(box4, rcnew(unconstrained_frame, box4Frame, geometry::planar::alignment::bottom_right()));

		guiSubsystem->open(string::literal(L"Alpha blended and animated + resizing test (black background)"), fourBoxPane);
	}

	{
		rcref<background> fourBoxPane = rcnew(background, color::black);

		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00, 0x7F));
		rcref<background> box2 = rcnew(background, color(0x00, 0xFF, 0x00, 0x7F));
		rcref<background> box3 = rcnew(background, color(0x00, 0x00, 0xFF, 0x7F));
		rcref<background> box4 = rcnew(background, color(0xFF, 0xFF, 0x00, 0x7F));

		rcref<fixed_size_frame> box1Frame = rcnew(fixed_size_frame, box1, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box2Frame = rcnew(fixed_size_frame, box2, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box3Frame = rcnew(fixed_size_frame, box3, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box4Frame = rcnew(fixed_size_frame, box4, gfx::canvas::size(200, 200));

		fourBoxPane->nest(box1, rcnew(unconstrained_frame, box1Frame, geometry::planar::alignment::top_left()));
		fourBoxPane->nest(box2, rcnew(unconstrained_frame, box2Frame, geometry::planar::alignment::top_right()));
		fourBoxPane->nest(box3, rcnew(unconstrained_frame, box3Frame, geometry::planar::alignment::bottom_left()));
		fourBoxPane->nest(box4, rcnew(unconstrained_frame, box4Frame, geometry::planar::alignment::bottom_right()));

		guiSubsystem->open(string::literal(L"Alpha blend + resizing test (black background)"), fourBoxPane);
	}

	{
		rcref<background> fourBoxPane = rcnew(background, color::black);

		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00, 0x7F));
		rcref<background> box2 = rcnew(background, color(0x00, 0xFF, 0x00, 0x7F));
		rcref<background> box3 = rcnew(background, color(0x00, 0x00, 0xFF, 0x7F));
		rcref<background> box4 = rcnew(background, color(0xFF, 0xFF, 0x00, 0x7F));

		rcref<native_container_pane> box1p = native_container_pane::create(); box1p->nest(box1);
		rcref<native_container_pane> box2p = native_container_pane::create(); box2p->nest(box2);
		rcref<native_container_pane> box3p = native_container_pane::create(); box3p->nest(box3);
		rcref<native_container_pane> box4p = native_container_pane::create(); box4p->nest(box4);

		rcref<fixed_size_frame> box1Frame = rcnew(fixed_size_frame, box1p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box2Frame = rcnew(fixed_size_frame, box2p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box3Frame = rcnew(fixed_size_frame, box3p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box4Frame = rcnew(fixed_size_frame, box4p, gfx::canvas::size(200, 200));

		fourBoxPane->nest(box1p, rcnew(unconstrained_frame, box1Frame, geometry::planar::alignment::top_left()));
		fourBoxPane->nest(box2p, rcnew(unconstrained_frame, box2Frame, geometry::planar::alignment::top_right()));
		fourBoxPane->nest(box3p, rcnew(unconstrained_frame, box3Frame, geometry::planar::alignment::bottom_left()));
		fourBoxPane->nest(box4p, rcnew(unconstrained_frame, box4Frame, geometry::planar::alignment::bottom_right()));

		guiSubsystem->open(string::literal(L"Alpha blend + resizing test (black background, native_container_pane's)"), fourBoxPane);
	}

	{
		rcref<background> fourBoxPane = rcnew(background, color::white);

		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00, 0x7F));
		rcref<background> box2 = rcnew(background, color(0x00, 0xFF, 0x00, 0x7F));
		rcref<background> box3 = rcnew(background, color(0x00, 0x00, 0xFF, 0x7F));
		rcref<background> box4 = rcnew(background, color(0xFF, 0xFF, 0x00, 0x7F));

		rcref<native_container_pane> box1p = native_container_pane::create(); box1p->nest(box1);
		rcref<native_container_pane> box2p = native_container_pane::create(); box2p->nest(box2);
		rcref<native_container_pane> box3p = native_container_pane::create(); box3p->nest(box3);
		rcref<native_container_pane> box4p = native_container_pane::create(); box4p->nest(box4);

		rcref<fixed_size_frame> box1Frame = rcnew(fixed_size_frame, box1p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box2Frame = rcnew(fixed_size_frame, box2p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box3Frame = rcnew(fixed_size_frame, box3p, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box4Frame = rcnew(fixed_size_frame, box4p, gfx::canvas::size(200, 200));

		fourBoxPane->nest(box1p, rcnew(unconstrained_frame, box1Frame, geometry::planar::alignment::top_left()));
		fourBoxPane->nest(box2p, rcnew(unconstrained_frame, box2Frame, geometry::planar::alignment::top_right()));
		fourBoxPane->nest(box3p, rcnew(unconstrained_frame, box3Frame, geometry::planar::alignment::bottom_left()));
		fourBoxPane->nest(box4p, rcnew(unconstrained_frame, box4Frame, geometry::planar::alignment::bottom_right()));

		guiSubsystem->open(string::literal(L"Alpha blend + resizing test (white background, native_container_pane's)"), fourBoxPane);
	}

	{
		rcref<background> fourBoxPane = rcnew(background, color::white);

		rcref<background> box1 = rcnew(background, color(0xFF, 0x00, 0x00, 0x7F));
		rcref<background> box2 = rcnew(background, color(0x00, 0xFF, 0x00, 0x7F));
		rcref<background> box3 = rcnew(background, color(0x00, 0x00, 0xFF, 0x7F));
		rcref<background> box4 = rcnew(background, color(0xFF, 0xFF, 0x00, 0x7F));

		rcref<fixed_size_frame> box1Frame = rcnew(fixed_size_frame, box1, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box2Frame = rcnew(fixed_size_frame, box2, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box3Frame = rcnew(fixed_size_frame, box3, gfx::canvas::size(200, 200));
		rcref<fixed_size_frame> box4Frame = rcnew(fixed_size_frame, box4, gfx::canvas::size(200, 200));

		fourBoxPane->nest(box1, rcnew(unconstrained_frame, box1Frame, geometry::planar::alignment::top_left()));
		fourBoxPane->nest(box2, rcnew(unconstrained_frame, box2Frame, geometry::planar::alignment::top_right()));
		fourBoxPane->nest(box3, rcnew(unconstrained_frame, box3Frame, geometry::planar::alignment::bottom_left()));
		fourBoxPane->nest(box4, rcnew(unconstrained_frame, box4Frame, geometry::planar::alignment::bottom_right()));

		guiSubsystem->open(string::literal(L"Alpha blend + resizing test (white background)"), fourBoxPane);
	}

	{
		rcref<pixel_image_pane> img4 = rcnew(pixel_image_pane, string::literal(L"guitar"));// , true, true);
		rcref<propose_aspect_ratio_frame> aspectRatioFrame = rcnew(propose_aspect_ratio_frame, img4);
		rcref<override_default_size_frame> defaultSizeFrame = rcnew(override_default_size_frame, aspectRatioFrame, gfx::canvas::size(200, 200));
		guiSubsystem->open(string::literal(L"image aspect ratio test"), img4, defaultSizeFrame);
	}

	{
		rcref<pixel_image_pane> img4 = rcnew(pixel_image_pane, string::literal(L"guitar"));// , true, true);
		rcref<override_default_size_frame> defaultSizeFrame = rcnew(override_default_size_frame, img4, gfx::canvas::size(200, 200));
		rcref<propose_aspect_ratio_frame> aspectRatioFrame = rcnew(propose_aspect_ratio_frame, defaultSizeFrame);
		guiSubsystem->open(string::literal(L"image aspect ratio test w/size"), img4, aspectRatioFrame);
	}

	{
		rcref<pixel_image_pane> img4 = rcnew(pixel_image_pane, string::literal(L"guitar"));// , true, true);
		rcref<propose_aspect_ratio_frame> aspectRatioFrame = rcnew(propose_aspect_ratio_frame, img4);
		rcref<override_default_size_frame> defaultSizeFrame = rcnew(override_default_size_frame, aspectRatioFrame, gfx::canvas::size(200, 200));
		rcref<unconstrained_frame> unconstrainedFrame = rcnew(unconstrained_frame, defaultSizeFrame);
		rcref<background> parentPane = rcnew(background, color::beige);
		parentPane->nest(img4, unconstrainedFrame);
		guiSubsystem->open(string::literal(L"image aspect ratio test2"), parentPane);
	}

	{
		rcref<pixel_image_pane> img4 = rcnew(pixel_image_pane, string::literal(L"guitar"));// , true, true);
		rcref<override_default_size_frame> defaultSizeFrame = rcnew(override_default_size_frame, img4, gfx::canvas::size(200, 200));
		rcref<propose_aspect_ratio_frame> aspectRatioFrame = rcnew(propose_aspect_ratio_frame, defaultSizeFrame);
		rcref<unconstrained_frame> unconstrainedFrame = rcnew(unconstrained_frame, aspectRatioFrame);
		rcref<background> parentPane = rcnew(background, color::beige);
		parentPane->nest(img4, unconstrainedFrame);
		guiSubsystem->open(string::literal(L"image aspect ratio test2 w/size"), parentPane);
	}

	{
		rcref<pixel_image_pane> img4 = rcnew(pixel_image_pane, string::literal(L"guitar"));// , true);
		rcref<override_default_size_frame> f2 = rcnew(override_default_size_frame, img4, gfx::canvas::size(200, 200));
		guiSubsystem->open(string::literal(L"image stretch test"), img4, f2);
	}


	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane);

		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));

		rcref<background> cornerPane = rcnew(background, color::purple);
		scrollPane->nest_corner(cornerPane);

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"), 168);
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane);

		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));

		rcref<background> cornerPane = rcnew(background, color::purple);
		scrollPane->nest_corner(cornerPane);

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test - 168 dpi"), root);
	}



	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically);

		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test, vert only"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally);

		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test, horiz only"), root);
	}




	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane);

		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically);

		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, vert only"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally);

		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, horiz only"), root);
	}




	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, vert only"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, horiz only"), root);
	}

	
	// same with scroll bar that do not auto-hide
	
	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally_and_vertically, false);
	
		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test"), root);
	}
	
	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically, false);
	
		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test, vert only"), root);
	}
	
	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally, false);
	
		scrollPane->nest(img1, rcnew(fixed_default_size_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"fixed scroll_pane resizing test, horiz only"), root);
	}




	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally_and_vertically, false);
	
		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test"), root);
	}
	
	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically, false);
	
		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, vert only"), root);
	}
	
	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally, false);
	
		scrollPane->nest(img1, rcnew(unshrinkable_frame, img1));
	
		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, horiz only"), root);
	}




	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally_and_vertically, false);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_vertically, false);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, vert only"), root);
	}

	{
		rcref<background> root = rcnew(background, color::beige);

		rcref<pixel_image_pane> img1 = rcnew(pixel_image_pane, string::literal(L"guitar"));
		rcref<scroll_pane> scrollPane = rcnew(scroll_pane, scroll_horizontally, false);

		scrollPane->nest(img1, rcnew(unstretchable_frame, img1));

		rcref<override_default_size_frame> f = rcnew(override_default_size_frame, scrollPane, gfx::canvas::size(200, 200));
		root->nest(scrollPane, f);
		guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, horiz only"), root);
	}





	{
		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane>& f)
			{
				f->fill(f->get_size(), color::blue);
				gfx::canvas::point dstpt(0, 0);
				dstpt += f->get_size();
				f->draw_line(gfx::canvas::point(0, 0), dstpt, 1, color(0xff, 0, 0, 0x7f));
				f->draw_line(gfx::canvas::point(0, dstpt.get_y()), gfx::canvas::point(dstpt.get_x(), 0), 1, color::red);
				f->draw_line(gfx::canvas::point(0, dstpt.get_y() / 2), gfx::canvas::point(dstpt.get_x(), dstpt.get_y() / 2), 1, color::red);
				f->draw_line(gfx::canvas::point(dstpt.get_x() / 2, dstpt.get_y()), gfx::canvas::point(dstpt.get_x() / 2, 0), 1, color::red);

				f->draw_line(gfx::canvas::point(0, 0), dstpt, 1, color(0xff, 0, 0, 0x7f));
				f->draw_line(gfx::canvas::point(0, dstpt.get_y()), gfx::canvas::point(dstpt.get_x(), 0), 1, color::red);
			});

		rcref<background> root = rcnew(background, color(color::turquoise, 0x7f));
		rcref<check_box> cb1 = rcnew(check_box, string::literal(L"check box"), true, false, gfx::font(18));

		root->nest(cb1, rcnew(unshrinkable_frame, cb1));
		box->nest(root, rcnew(unconstrained_max_frame, rcnew(fixed_default_size_frame, root)));

		guiSubsystem->open(string::literal(L"lines and nested checkbox"), box)
			->dispatch([]() { cogs::request_quit(); })
			;
	}



	{
		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane>& f)
		{
			f->fill(f->get_size(), color::blue);
			gfx::canvas::point dstPt(0, 0);
			dstPt += f->get_size();
			f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
			f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::red);
		});

		rcref<background> root = rcnew(background, color(color::turquoise, 0x7F));
		rcref<text_editor> te1 = rcnew(text_editor, string::literal(L"text editor"), true, gfx::font(string::literal(L"Arial"), 38));
		root->nest(te1);
		box->nest(root, rcnew(unconstrained_max_frame, rcnew(fixed_size_frame, root, size(300, 300))));

		guiSubsystem->open(string::literal(L"Lines and text_editor"), box);
	}


	{
		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane>& f)
		{
			f->fill(f->get_size(), color::blue);
			gfx::canvas::point dstPt(0, 0);
			dstPt += f->get_size();
			f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
			f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::red);
		});

		rcref<background> root = rcnew(background, color(color::white, 0x3F));
		rcref<label> lbl = rcnew(label, string::literal(L"LABEL"), gfx::font(38));
		root->nest(lbl);
		box->nest(root, rcnew(unconstrained_max_frame, rcnew(fixed_default_size_frame, root)));

		guiSubsystem->open(string::literal(L"Lines and label"), box);
	}

	{
		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane>& f)
		{
			f->fill(f->get_size(), color::blue);
			gfx::canvas::point dstPt(0, 0);
			dstPt += f->get_size();
			f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
			f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::red);
		});

		rcref<background> root = rcnew(background, color(color::white, 0x3F));
		rcref<label> lbl = rcnew(label, string::literal(L"LABEL CANT RESIZE"), gfx::font(38));
		root->nest(lbl);
		box->nest(root, rcnew(fixed_default_size_frame, root));

		guiSubsystem->open(string::literal(L"Lines and label"), box);
	}


	{
		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane> & f)
		{
			f->fill(f->get_size(), color::blue);
			gfx::canvas::point dstPt(0, 0);
			dstPt += f->get_size();
			f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
			f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::red);
		});

		rcref<background> root = rcnew(background, color(color::white, 0x3F));
		rcref<button> btn = rcnew(button, []() {}, string::literal(L"BUTTON"), gfx::font(38));
		root->nest(btn);
		box->nest(root, rcnew(unconstrained_max_frame, rcnew(fixed_default_size_frame, root)));

		guiSubsystem->open(string::literal(L"Lines and button"), box);
	}



	{
		rcref<button> btn1 = rcnew(button, [](const rcref<button>& btn)
		{
			btn->detach();
		}, string::literal(L"test one"));
		rcref<button> btn2 = rcnew(button, function<void()>(), string::literal(L"test two22222"));
		rcref<button> btn3 = rcnew(button, function<void()>(), string::literal(L"test three"), gfx::font(), true, true);
		rcref<button> btn4 = rcnew(button, function<void()>(), string::literal(L"test four with a very long name"));

		rcref<check_box> cb1 = rcnew(check_box, string::literal(L"check_box"), true, false, gfx::font(18));
		rcref<label> lb1 = rcnew(label, string::literal(L"label gjqyp,!l'"), gfx::font(24), color::green);
		rcref<text_editor> te1 = rcnew(text_editor, string::literal(L"text editor transparent"), true);
		rcref<text_editor> te2 = rcnew(text_editor, string::literal(L"text editor2"), true);

		rcref<background> te1Bkg = rcnew(background, color(color::turquoise, 0x7F));
		te1Bkg->nest(te1);

		gfx::canvas::range specialRange1;
		specialRange1.set_min(100, 100);
		specialRange1.set_max(120, 120);

		gfx::canvas::range specialRange2;
		specialRange2.set_min(125, 125);
		specialRange2.set_max(150, 150);

		typedef wrap_list<geometry::planar::script_flow::left_to_right_top_to_bottom> wrap_list_t;
		rcref<wrap_list_t> wrapper = rcnew(wrap_list_t);

		wrapper->nest(btn1);
		wrapper->nest(btn2);
		wrapper->nest(btn3);
		wrapper->nest(lb1);
		wrapper->nest(cb1);
		wrapper->nest(te1Bkg);
		wrapper->nest(te2);
		wrapper->nest(btn4);

		rcref<canvas_pane> box = canvas_pane::create([](const rcref<canvas_pane> & f)
			{
				gfx::canvas::point dstPt(0, 0);
				dstPt += f->get_size();
				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::red);
			});


		rcref<background> wrapperBkg = rcnew(background, color::orange);
		wrapperBkg->nest(box);
		wrapperBkg->nest(wrapper);

		guiSubsystem->open(string::literal(L"wrap_list + resizing test"), wrapperBkg)
			->dispatch([]() { cogs::request_quit(); });
	}


	{
		typedef grid<> grid_t;
		rcref<grid_t> g = rcnew(grid_t);


		rcref<background> box00 = rcnew(background, color(0xFF, 0x00, 0x00, 0xFF));
		rcref<background> box01 = rcnew(background, color(0x00, 0xFF, 0x00, 0xFF));
		rcref<background> box02 = rcnew(background, color(0x00, 0x00, 0xFF, 0xFF));
		rcref<background> box03 = rcnew(background, color(0xFF, 0xFF, 0x00, 0xFF));

		rcref<background> box10 = rcnew(background, color(0x00, 0xFF, 0x00, 0xFF));
		rcref<background> box11 = rcnew(background, color(0x00, 0x00, 0xFF, 0xFF));
		rcref<background> box12 = rcnew(background, color(0xFF, 0xFF, 0x00, 0xFF));
		rcref<background> box13 = rcnew(background, color(0xFF, 0x00, 0x00, 0xFF));

		rcref<background> box20 = rcnew(background, color(0x00, 0x00, 0xFF, 0xFF));
		rcref<background> box21 = rcnew(background, color(0xFF, 0xFF, 0x00, 0xFF));
		rcref<background> box22 = rcnew(background, color(0xFF, 0x00, 0x00, 0xFF));
		rcref<background> box23 = rcnew(background, color(0x00, 0xFF, 0x00, 0xFF));

		rcref<background> box30 = rcnew(background, color(0xFF, 0xFF, 0x00, 0xFF));
		rcref<background> box31 = rcnew(background, color(0xFF, 0x00, 0x00, 0xFF));
		rcref<background> box32 = rcnew(background, color(0x00, 0xFF, 0x00, 0xFF));
		rcref<background> box33 = rcnew(background, color(0x00, 0x00, 0xFF, 0xFF));

		rcref<label> lb00 = rcnew(label, string::literal(L"0,0"), gfx::font(24));
		rcref<label> lb01 = rcnew(label, string::literal(L"0,1"), gfx::font(24));
		rcref<label> lb02 = rcnew(label, string::literal(L"0,2"), gfx::font(24));
		rcref<label> lb03 = rcnew(label, string::literal(L"0,3"), gfx::font(24));

		rcref<label> lb10 = rcnew(label, string::literal(L"1,0"), gfx::font(24));
		rcref<label> lb11 = rcnew(label, string::literal(L"1,1"), gfx::font(24));
		rcref<label> lb12 = rcnew(label, string::literal(L"1,2"), gfx::font(24));
		rcref<label> lb13 = rcnew(label, string::literal(L"1,3"), gfx::font(24));

		rcref<label> lb20 = rcnew(label, string::literal(L"2,0"), gfx::font(24));
		rcref<label> lb21 = rcnew(label, string::literal(L"2,1"), gfx::font(24));
		rcref<label> lb22 = rcnew(label, string::literal(L"2,2"), gfx::font(24));
		rcref<label> lb23 = rcnew(label, string::literal(L"2,3"), gfx::font(24));

		rcref<label> lb30 = rcnew(label, string::literal(L"3,0"), gfx::font(24));
		rcref<label> lb31 = rcnew(label, string::literal(L"3,1"), gfx::font(24));
		rcref<label> lb32 = rcnew(label, string::literal(L"3,2"), gfx::font(24));
		rcref<label> lb33 = rcnew(label, string::literal(L"3,3"), gfx::font(24));

		box00->nest(lb00);
		box01->nest(lb01);
		box02->nest(lb02);
		box03->nest(lb03);

		box10->nest(lb10);
		box11->nest(lb11);
		box12->nest(lb12);
		box13->nest(lb13);

		box20->nest(lb20);
		box21->nest(lb21);
		box22->nest(lb22);
		box23->nest(lb23);

		box30->nest(lb30);
		box31->nest(lb31);
		box32->nest(lb32);
		box33->nest(lb33);

		g->nest(box00, 0, 0);
		g->nest(box01, 0, 1);
		g->nest(box02, 0, 2);
		g->nest(box03, 0, 3);

		g->nest(box10, 1, 0);
		g->nest(box11, 1, 1);
		g->nest(box12, 1, 2);
		g->nest(box13, 1, 3);

		g->nest(box20, 2, 0);
		g->nest(box21, 2, 1);
		g->nest(box22, 2, 2);
		g->nest(box23, 2, 3);

		g->nest(box30, 3, 0);
		g->nest(box31, 3, 1);
		g->nest(box32, 3, 2);
		//g->nest(box33, 3, 3);

		guiSubsystem->open(string::literal(L"Grid test"), g);
	}

	return 0;
}

