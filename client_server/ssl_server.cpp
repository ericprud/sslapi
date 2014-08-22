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

#include <boost/network/include/http/server.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <signal.h>

struct handler;
typedef boost::network::http::async_server<handler> server;

/**
 * Password callback for the private key
 */
std::string password_callback(std::size_t max_length, boost::asio::ssl::context_base::password_purpose purpose) {
	return std::string("test");
}


/**
 * Handler which is responsible for handling requests
 */
struct handler
{
    void operator()(server::request const& req, const server::connection_ptr& conn)
    {
		// Set response status
		conn->set_status(server::connection::ok);
		std::stringstream out;
		out << "HTTP/1.0 200 ok" << std::endl
			<< "Content-type: text/html" << std::endl << std::endl
			<< "<HTML><HEAD/><BODY>Hello " << name << ", connecting from " << req.source.c_str() << ", you visited " << req.destination << "</BODY></HTML>" << std::endl;

		// Write standard message
		conn->write(out.str());
    }
	
	/**
	 * Client certificate validation, currently does not validate anything yet, 
	 * extra checks could be added here :)
	 */
	bool verify_cert(bool preverified,  boost::asio::ssl::verify_context& ctx)
	{
		std::cout << "Verify " << std::endl;
		int8_t subject_name[256];
		X509_STORE_CTX *cts = ctx.native_handle();
		int32_t length = 0;
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		std::cout << "CTX ERROR : " << cts->error << std::endl;

		int32_t depth = X509_STORE_CTX_get_error_depth(cts);
		std::cout << "CTX DEPTH : " << depth << std::endl;

		switch (cts->error)
		{
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			std::cout << "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT" << std::endl;
			break;
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			 std::cout << "Certificate not yet valid!!"<< std::endl;
			break;
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			std::cout << "Certificate expired.." << std::endl;
			break;
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			std::cout << "Self signed certificate in chain!!!\n" << std::endl;
			break;
		default:
			break;
		}

		const int32_t name_length = 256;
		X509_NAME_oneline(X509_get_subject_name(cert), reinterpret_cast<char*>(subject_name), name_length);
		name = reinterpret_cast<char*>(subject_name);
		std::cout <<  "Verifying " <<  subject_name << std::endl;
		std::cout << "Verification status :" << preverified<< std::endl;

		return preverified;
	}
private:
	std::string name;
};

/**
 * Clean shutdown signal handler
 *
 * @param error
 * @param signal
 * @param p_server_instance
 */
void shut_me_down(
        const boost::system::error_code& error
        , int signal, boost::shared_ptr< server > p_server_instance)
{
    if (!error)
        p_server_instance->stop();
}


int main(void) try
{
	
    handler request_handler;

    // setup asio::io_service
    boost::shared_ptr< boost::asio::io_service > p_io_service(
            boost::make_shared< boost::asio::io_service >());

	// Initialize SSL context
	boost::shared_ptr<boost::asio::ssl::context> ctx = boost::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
	ctx->set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::single_dh_use);
	
	// Set keys
	ctx->set_password_callback(password_callback);
	ctx->use_certificate_chain_file("server.pem");
	ctx->use_private_key_file("server.pem", boost::asio::ssl::context::pem);
	ctx->use_tmp_dh_file("dh512.pem");
	
	/**
	 * verify client auth
	 */
	ctx->set_verify_mode(boost::asio::ssl::context::verify_fail_if_no_peer_cert | boost::asio::ssl::context::verify_peer);
	ctx->set_verify_callback(boost::bind(&handler::verify_cert, &request_handler, _1, _2));
	ctx->load_verify_file("ca.crt");

    // setup the async server, 
	// run on port 8442, use previously initialized SSL context
    boost::shared_ptr< server > p_server_instance(
            boost::make_shared<server>(
                    server::options(request_handler).
                            address("0.0.0.0")
                            .port("8442")
                            .io_service(p_io_service)
                            .reuse_address(true)
                            .thread_pool(boost::make_shared<boost::network::utils::thread_pool>(2))
							.context(ctx)
							));

    // setup clean shutdown
    boost::asio::signal_set signals(*p_io_service, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(shut_me_down, _1, _2, p_server_instance));

    // run the async server
    p_server_instance->run();

    // we are stopped - shutting down
    p_io_service->stop();


    std::cout << "Terminated normally" << std::endl;
    exit(EXIT_SUCCESS);
}
catch(const std::exception& e)
{
	std::cout <<"Abnormal termination - exception:"<< e.what() << std::endl;
    exit(EXIT_FAILURE);
}
