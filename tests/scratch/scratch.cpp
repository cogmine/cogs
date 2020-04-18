//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#include <iostream>
#include "cogs/cogs.hpp"

using namespace cogs;
using namespace cogs::gui;
using namespace cogs::io;
using namespace cogs::io::net;
using namespace cogs::io::net::ip;

class box : public background
{
public:
	color m_baseColor;
	timeout_t::period_t m_timeoutPeriod;
	function<void()> m_expireDelegate;
	function<void()> m_expireInUiThreadDelegate;
	rcptr<resettable_timer> m_boxTimer;

	struct options
	{
		color backgroundColor = color::constant::white;
		frame_list frames;
		pane_list children;
	};

	box()
		: box(options())
	{ }

	explicit box(options&& o)
		: background({
			.backgroundColor = o.backgroundColor,
			.frames = std::move(o.frames),
			.children = std::move(o.children)
		}),
		m_baseColor(o.backgroundColor),
		m_timeoutPeriod(measure<int_type, milliseconds>(5)),
		m_expireDelegate([r{ this_weak_rcptr }]()
		{
			rcptr<box> r2 = r;
			if (!!r2)
				r2->box_timer_expired();
		}),
		m_expireInUiThreadDelegate([r{ this_weak_rcptr }]()
		{
			rcptr<box> r2 = r;
			if (!!r2)
				r2->box_timer_expired_in_ui_thread();
		})
	{ }

	virtual void showing()
	{
		m_boxTimer = rcnew(resettable_timer)(m_timeoutPeriod);
		m_boxTimer->dispatch(m_expireDelegate);
	}

	virtual void hiding()
	{
		m_boxTimer.release();
	}

	void box_timer_expired()
	{
		pane::dispatch(m_expireInUiThreadDelegate, const_max_int_v<int>);
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

COGS_MAIN
{
	return cogs::main([](const auto&)
	{
		{
			rcref<http::server> httpServer = rcnew(http::server);
			rcref<smtp::server> smtpServer = rcnew(smtp::server);
			rcref<tcp::listener> httpListener = ip::tcp::server_listen(httpServer, 8080);// 80);
			rcref<tcp::listener> smtpListener = ip::tcp::server_listen(smtpServer, 8081);// 25);
			cleanup_queue::get()->add(httpServer);
			cleanup_queue::get()->add(smtpServer);
			cleanup_queue::get()->add(httpListener);
			cleanup_queue::get()->add(smtpListener);
		}

		auto guiSubsystem = gui::subsystem::get_default();
		if (!guiSubsystem)
		{
			std::cout << "Console UI - TBD" << std::endl;
			int i;
			std::cin >> i;

			cogs::request_quit();
			return EXIT_SUCCESS;
		}

		rcref<count_down_event> quitCountDown = rcnew(count_down_event)(0, []() { cogs::request_quit(); });

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Single content test"),
		//		rcnew(background)({
		//			.backgroundColor = color(0xFF, 0x00, 0x00, 0xFF)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Fixed size window"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0x00, 0x00),
		//					.frames = frame_list::create(rcnew(fixed_size_frame)(gfx::canvas::size(200, 200)))
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Centering"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0x00, 0x00),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::center()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Alpha blend and animated + resizing test (black background)"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.children = { // TODO: Construct pane_list, when cl.exe no longer generates bad code
		//				rcnew(box)({
		//					.backgroundColor = color(0xFF, 0x00, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(box)({
		//					.backgroundColor = color(0x00, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(box)({
		//					.backgroundColor = color(0x00, 0x00, 0xFF, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(box)({
		//					.backgroundColor = color(0xFF, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				})
		//			}
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Alpha blend + resizing test (black background)"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.children = { // TODO: Construct pane_list, when cl.exe no longer generates bad code
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0x00, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0x00, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0x00, 0x00, 0xFF, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				})
		//			}
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Alpha blend + resizing test (white background)"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::white,
		//			.children = { // TODO: Construct pane_list, when cl.exe no longer generates bad code
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0x00, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0x00, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::top_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0x00, 0x00, 0xFF, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_left()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				}),
		//				rcnew(background)({
		//					.backgroundColor = color(0xFF, 0xFF, 0x00, 0x7F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_right()),
		//						rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				})
		//			}
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Alpha blend + resizing test (black background, native_container_pane's)"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.children = pane_list::create(
		//				rcnew(native_container_pane)({
		//					.children = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(background)({
		//							.backgroundColor = color(0xFF, 0x00, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::top_left()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0x00, 0xFF, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::top_right()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0x00, 0x00, 0xFF, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_left()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0xFF, 0xFF, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_right()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						})
		//					}
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Alpha blend + resizing test (white background, native_container_pane's)"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::white,
		//			.children = pane_list::create(
		//				rcnew(native_container_pane)({
		//					.children = { // TODO: Construct pane_list, when cl.exe no longer generates bad code
		//						rcnew(background)({
		//							.backgroundColor = color(0xFF, 0x00, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::top_left()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0x00, 0xFF, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::top_right()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0x00, 0x00, 0xFF, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_left()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						}),
		//						rcnew(background)({
		//							.backgroundColor = color(0xFF, 0xFF, 0x00, 0x7F),
		//							.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//								rcnew(unconstrained_frame)(geometry::planar::alignment::bottom_right()),
		//								rcnew(fixed_size_frame)(gfx::canvas::size(200, 200))
		//							}
		//						})
		//					}
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image aspect ratio test"),
		//		rcnew(bitmap_pane)({
		//			.imageLocation = L"guitar.bmp",
		//			.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200)),
		//				rcnew(propose_aspect_ratio_frame)
		//			}
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image aspect ratio test w/size"),
		//		rcnew(bitmap_pane)({
		//			.imageLocation = L"guitar.bmp",
		//			.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//				rcnew(propose_aspect_ratio_frame),
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			}
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image aspect ratio test2"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::beige,
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame),
		//						rcnew(override_default_size_frame)(gfx::canvas::size(200, 200)),
		//						rcnew(propose_aspect_ratio_frame)
		//					}
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image aspect ratio test2 w/size"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::beige,
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_frame),
		//						rcnew(propose_aspect_ratio_frame),
		//						rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//					}
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image stretch test"),
		//		rcnew(bitmap_pane)({
		//			.imageLocation = L"guitar.bmp",
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			)
		//		})
		//	);
		//}


		{
			*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test"),
				rcnew(scroll_pane)({
					.frames = frame_list::create(
						rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
					),
					.children = pane_list::create(
						rcnew(bitmap_pane)({
							.imageLocation = L"guitar.bmp",
							.frames = frame_list::create(
								rcnew(fixed_default_size_frame)
							)
						})
					)
				})
			);
		}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test"),
		//		rcnew(scroll_pane)({
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test"),
		//		rcnew(scroll_pane)({
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//////// same with scroll bar that does not auto-hide

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test"),
		//		rcnew(scroll_pane)({
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test"),
		//		rcnew(scroll_pane)({
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unshrinkable scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unshrinkable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test"),
		//		rcnew(scroll_pane)({
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, vert only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::vertical,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"unstretchable scroll_pane resizing test, horiz only"),
		//		rcnew(scroll_pane)({
		//			.scrollDimensions = scroll_pane::dimensions::horizontal,
		//			.hideInactiveScrollBar = false,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp",
		//					.frames = frame_list::create(
		//						rcnew(unstretchable_frame)
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"lines and nested checkbox"),
		//		rcnew(canvas_pane)({
		//			.drawDelegate = [](const rcref<canvas_pane>& f)
		//			{
		//				f->fill(f->get_size(), color::constant::blue);
		//				gfx::canvas::point dstPt(0, 0);
		//				dstPt += f->get_size();
		//				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y() / 2), gfx::canvas::point(dstPt.get_x(), dstPt.get_y() / 2), 1, color::constant::red);
		//				f->draw_line(gfx::canvas::point(dstPt.get_x() / 2, dstPt.get_y()), gfx::canvas::point(dstPt.get_x() / 2, 0), 1, color::constant::red);
		//				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//			},
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(color::constant::turquoise, 0x7f),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_max_frame),
		//						rcnew(fixed_default_size_frame)
		//					},
		//					.children = pane_list::create(
		//						rcnew(check_box)({
		//							.text = string::literal(L"check box"),
		//							.font = gfx::font(18),
		//							.frames = frame_list::create(
		//								rcnew(unshrinkable_frame)
		//							)
		//						})
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Lines and label"),
		//		rcnew(canvas_pane)({
		//			.drawDelegate = [](const rcref<canvas_pane>& f)
		//			{
		//				f->fill(f->get_size(), color::constant::blue);
		//				gfx::canvas::point dstPt(0, 0);
		//				dstPt += f->get_size();
		//				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//			},
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(color::constant::white, 0x3F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_max_frame),
		//						rcnew(fixed_default_size_frame)
		//					},
		//					.children = pane_list::create(
		//						rcnew(label)({
		//							.text = string::literal(L"LABEL"),
		//							.font = gfx::font(string::literal(L"what happens when a font is not found"), 38),
		//							.textColor = color::constant::black
		//						})
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Lines and label, no resize"),
		//		rcnew(canvas_pane)({
		//			.drawDelegate = [](const rcref<canvas_pane>& f)
		//			{
		//				f->fill(f->get_size(), color::constant::blue);
		//				gfx::canvas::point dstPt(0, 0);
		//				dstPt += f->get_size();
		//				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//			},
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(color::constant::turquoise, 0x7f),
		//					.frames = frame_list::create(
		//						rcnew(fixed_default_size_frame)
		//					),
		//					.children = pane_list::create(
		//						rcnew(label)({
		//							.text = string::literal(L"LABEL, no resize"),
		//							.font = gfx::font(38),
		//							.textColor = color::constant::black
		//						})
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Lines and button"),
		//		rcnew(canvas_pane)({
		//			.drawDelegate = [](const rcref<canvas_pane>& f)
		//			{
		//				f->fill(f->get_size(), color::constant::blue);
		//				gfx::canvas::point dstPt(0, 0);
		//				dstPt += f->get_size();
		//				f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//				f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//			},
		//			.children = pane_list::create(
		//				rcnew(background)({
		//					.backgroundColor = color(color::constant::white, 0x3F),
		//					.frames = { // TODO: Construct frame_list, when cl.exe no longer generates bad code
		//						rcnew(unconstrained_max_frame),
		//						rcnew(fixed_default_size_frame)
		//					},
		//					.children = pane_list::create(
		//						rcnew(button)({
		//							.text = string::literal(L"BUTTON"),
		//							.font = gfx::font(38)
		//						})
		//					)
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Mask render test"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(800, 650))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						color grn(0x40, 40, 40);
		//						grn.set_alpha(0x90);
		//						f->fill(f->get_size(), grn);
		//
		//						color a(color::constant::red);
		//						color b(color::constant::blue);
		//
		//						rcref<gfx::canvas::bitmask> msk = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk->fill(msk->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk->fill(gfx::canvas::bounds(0, 0, 50, 50), gfx::canvas::bitmask::fill_mode::set_mode);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 50, 100, 100), a, b, false, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 200, 100, 100), a, b, false, false);
		//
		//						a.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 50, 100, 100), a, color::constant::transparent, false, false);
		//
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 200, 100, 100), color::constant::transparent, b, false, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 50, 100, 100), a, b, false, false);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 200, 100, 100), a, b, false, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 50, 100, 100), a, color::constant::transparent, false, false);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 200, 100, 100), color::constant::transparent, b, false, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x20);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 50, 100, 100), a, b, false, false);
		//
		//						a.set_alpha(0x20);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 200, 100, 100), a, b, false, false);
		//
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 350, 100, 100), a, b, true, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 500, 100, 100), a, b, true, true);
		//
		//						a.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 350, 100, 100), a, color::constant::transparent, true, true);
		//
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 500, 100, 100), color::constant::transparent, b, true, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 350, 100, 100), a, b, true, true);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 500, 100, 100), a, b, true, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 350, 100, 100), a, color::constant::transparent, true, true);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 500, 100, 100), color::constant::transparent, b, true, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x20);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 350, 100, 100), a, b, true, true);
		//
		//						a.set_alpha(0x20);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 500, 100, 100), a, b, true, true);
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Mask render test2"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::black,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(800, 650))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						color grn(0x40, 40, 40);
		//						grn.set_alpha(0x90);
		//						f->fill(f->get_size(), grn);
		//
		//						color a(color::constant::red);
		//						color b(color::constant::blue);
		//
		//						rcref<gfx::canvas::bitmask> msk = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk->fill(msk->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk->fill(gfx::canvas::bounds(0, 0, 50, 50), gfx::canvas::bitmask::fill_mode::set_mode);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 50, 100, 100), a, b, true, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 200, 100, 100), a, b, true, false);
		//
		//						a.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 50, 100, 100), a, color::constant::transparent, true, false);
		//
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 200, 100, 100), color::constant::transparent, b, true, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 50, 100, 100), a, b, true, false);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 200, 100, 100), a, b, true, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 50, 100, 100), a, color::constant::transparent, true, false);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 200, 100, 100), color::constant::transparent, b, true, false);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x20);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 50, 100, 100), a, b, true, false);
		//
		//						a.set_alpha(0x20);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 200, 100, 100), a, b, true, false);
		//
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 350, 100, 100), a, b, false, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(50, 500, 100, 100), a, b, false, true);
		//
		//						a.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 350, 100, 100), a, color::constant::transparent, false, true);
		//
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(200, 500, 100, 100), color::constant::transparent, b, false, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0xFF);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 350, 100, 100), a, b, false, true);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(350, 500, 100, 100), a, b, false, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 350, 100, 100), a, color::constant::transparent, false, true);
		//
		//						a.set_alpha(0xFF);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(500, 500, 100, 100), color::constant::transparent, b, false, true);
		//
		//						a.set_alpha(0x7F);
		//						b.set_alpha(0x20);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 350, 100, 100), a, b, false, true);
		//
		//						a.set_alpha(0x20);
		//						b.set_alpha(0x7F);
		//						f->draw_bitmask(*msk, msk->get_size(), gfx::canvas::bounds(650, 500, 100, 100), a, b, false, true);
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image stretch test"),
		//		rcnew(bitmap_pane)({
		//			.imageLocation = L"guitar.bmp",
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"image stretch testx"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::purple,
		//			.compositingBehavior = compositing_behavior::buffer_self_and_children,
		//			.frames = frame_list::create(
		//				rcnew(override_default_size_frame)(gfx::canvas::size(200, 200))
		//			),
		//			.children = pane_list::create(
		//				rcnew(bitmap_pane)({
		//					.imageLocation = L"guitar.bmp"
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Mask render test3"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::green,
		//			.compositingBehavior = compositing_behavior::buffer_self_and_children,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(300, 400))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						color grn(color::constant::green);
		//						//color grn(0x40, 40, 40);
		//						//grn.set_alpha(0x90);
		//
		//						f->fill(f->get_size(), grn);
		//
		//						rcref<gfx::canvas::bitmask> msk = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk->fill(msk->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk->fill(gfx::canvas::bounds(20, 10, 30, 30));
		//
		//						rcref<gfx::canvas::bitmask> msk2 = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk2->fill(msk2->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk2->fill(gfx::canvas::bounds(5, 15, 30, 30));
		//
		//						rcref<gfx::canvas::bitmap> img = f->load_bitmap(L"guitar.bmp");
		//
		//						rcref<gfx::canvas::bitmap> tmpImg = f->create_bitmap(img->get_size());
		//
		//						tmpImg->draw_bitmap_with_bitmask(*img, img->get_size(), *msk2, msk2->get_size(), tmpImg->get_size(), false, false);
		//
		//						//f->draw_bitmap(*tmpImg, tmpImg->get_size(), f->get_size(), false);
		//
		//						rcref<gfx::canvas::bitmap> tmpImg2 = f->create_bitmap(f->get_size());
		//						tmpImg2->fill(tmpImg2->get_size(), color::constant::purple, false);
		//
		//						//tmpImg2->draw_bitmap(*tmpImg, tmpImg->get_size(), tmpImg2->get_size(), false);
		//						tmpImg2->draw_bitmap_with_bitmask(*tmpImg, tmpImg->get_size(), *msk, msk->get_size(), tmpImg2->get_size(), false, false);
		//
		//						f->draw_bitmap(*tmpImg2, tmpImg2->get_size(), f->get_size(), false);
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}
		//
		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"Mask render test4"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::green,
		//			.compositingBehavior = compositing_behavior::buffer_self_and_children,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(300, 400))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						color grn(color::constant::green);
		//						//color grn(0x40, 40, 40);
		//						//grn.set_alpha(0x90);
		//
		//						f->fill(f->get_size(), grn);
		//
		//						rcref<gfx::canvas::bitmask> msk = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk->fill(msk->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk->fill(gfx::canvas::bounds(20, 10, 30, 30));
		//
		//						rcref<gfx::canvas::bitmask> msk2 = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk2->fill(msk2->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk2->fill(gfx::canvas::bounds(5, 15, 30, 30));
		//
		//
		//						rcref<gfx::canvas::bitmap> img = f->load_bitmap(L"guitar.bmp");
		//
		//						rcref<gfx::canvas::bitmap> tmpImg = f->create_bitmap(img->get_size() + size(100, 100));
		//
		//						tmpImg->draw_bitmap_with_bitmask(*img, img->get_size(), *msk2, msk2->get_size(), tmpImg->get_size(), false, false);
		//
		//						//f->draw_bitmap(*tmpImg, tmpImg->get_size(), f->get_size(), false);
		//
		//						rcref<gfx::canvas::bitmap> tmpImg2 = f->create_bitmap(f->get_size());
		//						tmpImg2->fill(tmpImg2->get_size(), color::constant::purple, false);
		//
		//						//tmpImg2->draw_bitmap(*tmpImg, tmpImg->get_size(), tmpImg2->get_size(), false);
		//						tmpImg2->draw_bitmap_with_bitmask(*tmpImg, tmpImg->get_size(), *msk, msk->get_size(), tmpImg2->get_size(), false, false);
		//
		//						f->draw_bitmap(*tmpImg2, tmpImg2->get_size(), f->get_size(), false);
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"draw_image test with different buffer sizes"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::green,
		//			.compositingBehavior = compositing_behavior::buffer_self_and_children,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(300, 400))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						rcref<gfx::canvas::bitmap> img = f->load_bitmap(L"guitar.bmp");
		//
		//						rcref<gfx::canvas::bitmask> msk = f->create_bitmask(gfx::canvas::size(100, 100));
		//						msk->fill(msk->get_size(), gfx::canvas::bitmask::fill_mode::clear_mode);
		//						msk->fill(gfx::canvas::bounds(20, 10, 30, 30));
		//						img->mask_out(*msk, msk->get_size(), img->get_size());
		//
		//						//rcref<gfx::canvas::bitmap> tmpImg = f->create_bitmap(img->get_size() + size(100, 100));
		//						//tmpImg->draw_bitmap(*img, img->get_size(), tmpImg->get_size());
		//						//f->draw_bitmap(*tmpImg, tmpImg->get_size(), f->get_size());
		//						f->draw_bitmap(*img, img->get_size(), f->get_size());
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}

		//{
		//	*quitCountDown += guiSubsystem->open(string::literal(L"invert test"),
		//		rcnew(background)({
		//			.backgroundColor = color::constant::green,
		//			.compositingBehavior = compositing_behavior::buffer_self_and_children,
		//			.frames = frame_list::create(
		//				rcnew(fixed_size_frame)(gfx::canvas::size(300, 400))
		//			),
		//			.children = pane_list::create(
		//				rcnew(canvas_pane)({
		//					.drawDelegate = [](const rcref<canvas_pane>& f)
		//					{
		//						rcref<gfx::canvas::bitmap> img = f->load_bitmap(L"guitar.bmp");
		//						img->invert(img->get_size());
		//						f->draw_bitmap(*img, img->get_size(), f->get_size());
		//					},
		//					.compositingBehavior = compositing_behavior::buffer_self_and_children
		//				})
		//			)
		//		})
		//	);
		//}

		//// TBD:

		//////{
		//////	rcref<button> btn1 = rcnew(button)([](const rcref<button>& btn)
		//////	{
		//////		btn->detach();
		//////	}, string::literal(L"test one"));
		//////	rcref<button> btn2 = rcnew(button)(function<void()>(), string::literal(L"test two22222"));
		//////	rcref<button> btn3 = rcnew(button)(function<void()>(), string::literal(L"test three"), gfx::font(), true, true);
		//////	rcref<button> btn4 = rcnew(button)(function<void()>(), string::literal(L"test four with a very long name"));
		//////	rcref<check_box> cb1 = rcnew(check_box)(string::literal(L"check_box"), true, false, gfx::font(18));
		//////	rcref<label> lb1 = rcnew(label)(string::literal(L"label gjqyp,!l'"), gfx::font(24), color::constant::green);
		//////	rcref<text_editor> te1 = rcnew(text_editor)(string::literal(L"text editor transparent"), true);
		//////	rcref<text_editor> te2 = rcnew(text_editor)(string::literal(L"text editor2"), true);
		//////	rcref<background> te1Bkg = rcnew(background)(color(color::constant::turquoise, 0x7F));
		//////	te1Bkg->nest(te1);
		//////	gfx::canvas::range specialRange1;
		//////	specialRange1.set_min(100, 100);
		//////	specialRange1.set_max(120, 120);
		//////	gfx::canvas::range specialRange2;
		//////	specialRange2.set_min(125, 125);
		//////	specialRange2.set_max(150, 150);
		//////	typedef wrap_list<geometry::planar::script_flow::left_to_right_top_to_bottom> wrap_list_t;
		//////	rcref<wrap_list_t> wrapper = rcnew(wrap_list_t);
		//////	wrapper->nest(btn1);
		//////	wrapper->nest(btn2);
		//////	wrapper->nest(btn3);
		//////	wrapper->nest(lb1);
		//////	wrapper->nest(cb1);
		//////	wrapper->nest(te1Bkg);
		//////	wrapper->nest(te2);
		//////	wrapper->nest(btn4);
		//////	rcref<canvas_pane> box = rcnew(canvas_pane)([](const rcref<canvas_pane>& f)
		//////	{
		//////		gfx::canvas::point dstPt(0, 0);
		//////		dstPt += f->get_size();
		//////		f->draw_line(gfx::canvas::point(0, 0), dstPt, 1, color(0xff, 0, 0, 0x7f));
		//////		f->draw_line(gfx::canvas::point(0, dstPt.get_y()), gfx::canvas::point(dstPt.get_x(), 0), 1, color::constant::red);
		//////	});
		//////	rcref<background> wrapperBkg = rcnew(background)(color::constant::orange);
		//////	wrapperBkg->nest(box);
		//////	wrapperBkg->nest(wrapper);
		//////	*quitCountDown += guiSubsystem->open(string::literal(L"wrap_list + resizing test"), wrapperBkg);
		//////}


		//////{
		//////	typedef grid<> grid_t;
		//////	rcref<grid_t> g = rcnew(grid_t);
		//////
		//////	rcref<background> box00 = rcnew(background)(color(0xFF, 0x00, 0x00, 0xFF));
		//////	rcref<background> box01 = rcnew(background)(color(0x00, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box02 = rcnew(background)(color(0x00, 0x00, 0xFF, 0xFF));
		//////	rcref<background> box03 = rcnew(background)(color(0xFF, 0xFF, 0x00, 0xFF));
		//////
		//////	rcref<background> box10 = rcnew(background)(color(0x00, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box11 = rcnew(background)(color(0x00, 0x00, 0xFF, 0xFF));
		//////	rcref<background> box12 = rcnew(background)(color(0xFF, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box13 = rcnew(background)(color(0xFF, 0x00, 0x00, 0xFF));
		//////
		//////	rcref<background> box20 = rcnew(background)(color(0x00, 0x00, 0xFF, 0xFF));
		//////	rcref<background> box21 = rcnew(background)(color(0xFF, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box22 = rcnew(background)(color(0xFF, 0x00, 0x00, 0xFF));
		//////	rcref<background> box23 = rcnew(background)(color(0x00, 0xFF, 0x00, 0xFF));
		//////
		//////	rcref<background> box30 = rcnew(background)(color(0xFF, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box31 = rcnew(background)(color(0xFF, 0x00, 0x00, 0xFF));
		//////	rcref<background> box32 = rcnew(background)(color(0x00, 0xFF, 0x00, 0xFF));
		//////	rcref<background> box33 = rcnew(background)(color(0x00, 0x00, 0xFF, 0xFF));
		//////
		//////	rcref<label> lb00 = rcnew(label)(string::literal(L"0,0"), gfx::font(24));
		//////	rcref<label> lb01 = rcnew(label)(string::literal(L"0,1"), gfx::font(24));
		//////	rcref<label> lb02 = rcnew(label)(string::literal(L"0,2"), gfx::font(24));
		//////	rcref<label> lb03 = rcnew(label)(string::literal(L"0,3"), gfx::font(24));
		//////
		//////	rcref<label> lb10 = rcnew(label)(string::literal(L"1,0"), gfx::font(24));
		//////	rcref<label> lb11 = rcnew(label)(string::literal(L"1,1"), gfx::font(24));
		//////	rcref<label> lb12 = rcnew(label)(string::literal(L"1,2"), gfx::font(24));
		//////	rcref<label> lb13 = rcnew(label)(string::literal(L"1,3"), gfx::font(24));
		//////
		//////	rcref<label> lb20 = rcnew(label)(string::literal(L"2,0"), gfx::font(24));
		//////	rcref<label> lb21 = rcnew(label)(string::literal(L"2,1"), gfx::font(24));
		//////	rcref<label> lb22 = rcnew(label)(string::literal(L"2,2"), gfx::font(24));
		//////	rcref<label> lb23 = rcnew(label)(string::literal(L"2,3"), gfx::font(24));
		//////
		//////	rcref<label> lb30 = rcnew(label)(string::literal(L"3,0"), gfx::font(24));
		//////	rcref<label> lb31 = rcnew(label)(string::literal(L"3,1"), gfx::font(24));
		//////	rcref<label> lb32 = rcnew(label)(string::literal(L"3,2"), gfx::font(24));
		//////	rcref<label> lb33 = rcnew(label)(string::literal(L"3,3"), gfx::font(24));
		//////
		//////	box00->nest(lb00);
		//////	box01->nest(lb01);
		//////	box02->nest(lb02);
		//////	box03->nest(lb03);
		//////
		//////	box10->nest(lb10);
		//////	box11->nest(lb11);
		//////	box12->nest(lb12);
		//////	box13->nest(lb13);
		//////
		//////	box20->nest(lb20);
		//////	box21->nest(lb21);
		//////	box22->nest(lb22);
		//////	box23->nest(lb23);
		//////
		//////	box30->nest(lb30);
		//////	box31->nest(lb31);
		//////	box32->nest(lb32);
		//////	box33->nest(lb33);
		//////
		//////	g->nest(box00, 0, 0);
		//////	g->nest(box01, 0, 1);
		//////	g->nest(box02, 0, 2);
		//////	g->nest(box03, 0, 3);
		//////
		//////	g->nest(box10, 1, 0);
		//////	g->nest(box11, 1, 1);
		//////	g->nest(box12, 1, 2);
		//////	g->nest(box13, 1, 3);
		//////
		//////	g->nest(box20, 2, 0);
		//////	g->nest(box21, 2, 1);
		//////	g->nest(box22, 2, 2);
		//////	g->nest(box23, 2, 3);
		//////
		//////	g->nest(box30, 3, 0);
		//////	g->nest(box31, 3, 1);
		//////	g->nest(box32, 3, 2);
		//////	//g->nest(box33, 3, 3);
		//////
		//////	*quitCountDown += guiSubsystem->open(string::literal(L"Grid test"), g);
		//////}

		return EXIT_SUCCESS;
	});
}
