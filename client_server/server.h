#pragma once

#include <boost/network/include/http/server.hpp>
#include <boost/shared_ptr.hpp>


struct handler;
typedef boost::network::http::async_server<handler> server;

struct handler {
	void operator()(server::request const& req, const server::connection_ptr& conn);
	bool verify_cert(bool preverified,  boost::asio::ssl::verify_context& ctx);
private:
	std::string name;
};

void shut_me_down(
        const boost::system::error_code& error
        , int signal, boost::shared_ptr< server > p_server_instance);

struct netlib_server {
public:
	static netlib_server& get_instance();

    handler request_handler;
	boost::shared_ptr< boost::asio::io_service > p_io_service;
	boost::shared_ptr< server > p_server_instance;


	void run();

	void stop();

private:
	netlib_server();
};
