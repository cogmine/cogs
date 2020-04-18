#include <iostream>
#include "cogs/cogs.hpp"

using namespace cogs;
using namespace cogs::gui;

COGS_MAIN
{
	return cogs::main([](const auto&)
	{
		auto guiSubsystem = gui::subsystem::get_default();
		if (!guiSubsystem)
		{
			std:: cout << "Hello World!" << std::endl;
			cogs::request_quit();
			return EXIT_SUCCESS;
		}

		rcref<count_down_event> quitCountDown = rcnew(count_down_event)(0, []() { cogs::request_quit(); });

		*quitCountDown += guiSubsystem->open(string::literal(L"Template App"),
			rcnew(background)({
				.backgroundColor = color::constant::white,
				.children = pane_list::create(
					rcnew(label)({
						.text = string::literal(L"Hello World!"),
						.font = gfx::font(38),
						.textColor = color::constant::black
					})
				)
			})
		);

		return EXIT_SUCCESS;
	});
}
