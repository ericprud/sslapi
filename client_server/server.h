#pragma once

#include <boost/network/include/http/server.hpp>
#include <boost/shared_ptr.hpp>


struct handler;
typedef boost::network::http::async_server<handler> server;

struct netlib_server;
struct handler {
    handler (netlib_server* p_server_instance)
    : p_server_instance(p_server_instance)
    {  }
	void operator()(server::request const& req, const server::connection_ptr& conn);
	bool verify_cert(bool preverified,  boost::asio::ssl::verify_context& ctx);
private:
	std::string name;
    netlib_server* p_server_instance;
};

void shut_me_down(
        const boost::system::error_code& error
        , int signal, boost::shared_ptr< server > p_server_instance);

struct netlib_server {
public:
	static netlib_server& get_instance(unsigned int port);

    handler request_handler;
	boost::shared_ptr< boost::asio::io_service > p_io_service;
	boost::shared_ptr< server > p_server_instance;
    unsigned int port;


	void run();

	void stop();
	void done();
    unsigned int stopAfter;

private:
	netlib_server(unsigned int port);
    const static unsigned int RunForever = ~0;
};
