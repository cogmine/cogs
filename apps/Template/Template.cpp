
#include <iostream>
#include "cogs/cogs.hpp"

using namespace cogs;
using namespace cogs::gui;

COGS_MAIN
{
	return cogs::main([](const auto& uiSubsystem)
	{
		auto guiSubsystem = gui::subsystem::get_default();
		if (!guiSubsystem)
		{
			std:: cout << "Hello World!" << std::endl;
			cogs::request_quit();
			return EXIT_SUCCESS;
		}

		rcref<count_down_event> quitCountDown = rcnew(count_down_event, 0, []() { cogs::request_quit(); });

		rcref<background> bg = rcnew(background, color::constant::white);
		rcref<label> lbl = rcnew(label, string::literal(L"Hello World!"), gfx::font(38));
		bg->nest(lbl);
		*quitCountDown += guiSubsystem->open(string::literal(L"Template App"), bg);

		return EXIT_SUCCESS;
	});
}
