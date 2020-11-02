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

		rcref<count_down_condition> quitCountDown = rcnew(count_down_condition)(0, []() { cogs::request_quit(); });

		*quitCountDown += guiSubsystem->open(
			rcnew(background)({
				.backgroundColor = color::constant::white,
				.children =  rcnew(label)({
					.text = string::literal(L"Hello World!"),
					.font = gfx::font_parameters{ .pointSize = 38 },
					.textColor = color::constant::black
				})
			}),
			string::literal(L"Template App")
		);

		return EXIT_SUCCESS;
	});
}
