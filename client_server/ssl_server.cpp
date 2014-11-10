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
#include "url.hpp"
#include <boost/program_options.hpp>
namespace po = boost::program_options;

//  --stop-after 1 --serve http://localhost:36864/SSLAPI
int main(int argc, const char* argv[]) try
{

        po::options_description generalOpts("General options");
        generalOpts.add_options()
            ("help,h", "brief help message")    
            ("serve", po::value<std::string>(), "serve URI")
            ("stop-after", po::value<unsigned int>(), "server handles n requests, then exits.")
	    ;
        po::options_description cmdline_options;
        cmdline_options.add(generalOpts);
        po::positional_options_description p;
        p.add("ordered", -1);
        po::variables_map vm; {
	    // ...populated by command line...
	    po::store(po::command_line_parser(argc, argv).
		      options(cmdline_options).positional(p).run(), vm);
	    // ...and a config file.
	    // std::ifstream ifs(".SSLAPI.cfg");
	    // po::store(parse_config_file(ifs, config_file_options), vm);
	    po::notify(vm);
	}

        if (vm.count("help")) {
            std::cout << 
		"Usage: sparql [opts] queryURI mapURI*\n"
		"       sparql [opts] -e query mapURI*\n"
		"       sparql [opts] --server URL mapURI*\n\n"
		"get started with: sparql --Help tutorial\n";
        }

        if (vm.count("serve")) {
	    std::string serverURI = vm["serve"].as<std::string>();

	    // if (vm.count("stop"))
	    // 	TheServer.stopCommand = vm["stop"].as<std::string>();
	    // if (vm.count("trap-sig-int"))
	    // 	TheServer.set_trap_sig_int(vm["trap-sig-int"].as<bool>());

	    URL u(serverURI.c_str());

	    unsigned int httpThreads = vm.count("http-threads") ? vm["http-threads"].as<unsigned int>() : 5;
	    // TheServer.runServer(handler, httpThreads, ServerURI, u.serviceHost, u.servicePort, u.servicePath, vm.count("server-no-description") == 0);

	    netlib_server& s = netlib_server::get_instance(u.servicePort);
	    //netlib_server s2(s);
	    if (vm.count("stop-after"))
		s.stopAfter = vm["stop-after"].as<unsigned int>();

	    // setup clean shutdown
	    boost::asio::signal_set signals(*s.p_io_service, SIGINT, SIGTERM);
	    signals.async_wait(boost::bind(shut_me_down, _1, _2, s.p_server_instance));

	    s.run();


	    // we are stopped - shutting down
	    s.stop();

	    std::cout << "Terminated normally" << std::endl;
	}

	exit(EXIT_SUCCESS);
 }
 catch(const std::exception& e)
     {
	 std::cout <<"Abnormal termination - exception:"<< e.what() << std::endl;
	 exit(EXIT_FAILURE);
     }
