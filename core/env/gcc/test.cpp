
#include <stdio.h>
#include <iostream>

#include "cogs/cogs.hpp"

using namespace cogs;
using namespace io;
using namespace net;
using namespace ip;


COGS_MAIN
{
	return cogs::main([](const auto& uiSubsystem)
	{
		rcptr<http::server> m_httpServer;
		rcptr<smtp::server> m_smtpServer;

		rcptr<tcp::listener> m_httpListener;
		rcptr<tcp::listener> m_smtpListener;

		m_httpServer = rcnew(http::server);
		m_smtpServer = rcnew(smtp::server);

		m_httpListener = ip::tcp::server_listen(m_httpServer.dereference(), 8080);// 80);
		m_smtpListener = ip::tcp::server_listen(m_smtpServer.dereference(), 8081);// 25);

		int i;
		std::cin >> i;

		m_httpServer.release();
		m_smtpServer.release();

		m_httpListener.release();
		m_smtpListener.release();

		quit_dispatcher::get()->request();
		return EXIT_SUCCESS;
	});
}
