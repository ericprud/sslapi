/*
 * Sample application based loosely on existing async server sample to demonstrate ssl 
 *
 * Requires openssl lib to run (https://www.openssl.org/)
 * 
 * (C) Copyright Jelle Van den Driessche 2014.
 *
 * Distributed under the Boost Software License, Version 1.0. (See copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "server.h"


int main(void) try
{
	netlib_server s = netlib_server::get_instance();

	// setup clean shutdown
	boost::asio::signal_set signals(*s.p_io_service, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(shut_me_down, _1, _2, s.p_server_instance));

	s.run();


    // we are stopped - shutting down
	s.stop();

    std::cout << "Terminated normally" << std::endl;
    exit(EXIT_SUCCESS);
}
catch(const std::exception& e)
{
	std::cout <<"Abnormal termination - exception:"<< e.what() << std::endl;
    exit(EXIT_FAILURE);
}
