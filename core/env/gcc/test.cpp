
#include <stdio.h>
#include <iostream>

#include "cogs/io/net/http.hpp"
#include "cogs/io/net/smtp.hpp"
#include "cogs/sync/quit_dispatcher.hpp"

using namespace cogs;
using namespace io;
using namespace net;
using namespace ip;



namespace cogs {

int main()
{
	{
		rcptr<http::server> m_httpServer;
		rcptr<smtp::server> m_smtpServer;

		rcptr<tcp::listener> m_httpListener;
		rcptr<tcp::listener> m_smtpListener;

		m_httpServer = rcnew(http::server);
		m_smtpServer = rcnew(smtp::server);

		m_httpListener = ip::tcp::server_listen(m_httpServer.dereference(), 8080);
		m_smtpListener = ip::tcp::server_listen(m_smtpServer.dereference(), 8081);// 25);


		int i;
		std::cin >> i;

		m_httpServer.release();
		m_smtpServer.release();

		m_httpListener.release();
		m_smtpListener.release();
	}
	cogs::quit_dispatcher::get()->request();
}

}
