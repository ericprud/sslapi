#include "server.h"


#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <iostream>
#include <signal.h>

/**
 * Password callback for the private key
 */
std::string password_callback(std::size_t max_length, boost::asio::ssl::context_base::password_purpose purpose) {
	return std::string("test");
}

void handler::operator()(server::request const& req, const server::connection_ptr& conn)
{
	// Set response status
	conn->set_status(server::connection::ok);
	server::response_header headers[] = {
		{"Content-type","text/html"},
		{"Connection","close"},
		{"X-Powered-By", "Hamsters"}
	};
	conn->set_headers(boost::make_iterator_range(headers, headers+3));

	std::stringstream out;
	out << "<HTML><HEAD/><BODY>Hello " << name << 
		", connecting from " << req.source.c_str() << ", you visited " << 
		req.destination << "</BODY></HTML>" << std::endl;

	// Write standard message
	conn->write(out.str());
}
	
/**
	* Client certificate validation, currently does not validate anything yet, 
	* extra checks could be added here :)
	*/
bool handler::verify_cert(bool preverified,  boost::asio::ssl::verify_context& ctx)
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



netlib_server::netlib_server() :
		p_io_service(boost::make_shared< boost::asio::io_service >()) {
		
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

			
			p_server_instance =
					boost::make_shared<server>(
							server::options(request_handler).
									address("0.0.0.0")
									.port("8442")
									.io_service(p_io_service)
									.reuse_address(true)
									.thread_pool(boost::make_shared<boost::network::utils::thread_pool>(2))
									.context(ctx)
									);


}

void netlib_server::run() {
	p_server_instance->run();
}

void netlib_server::stop() {
	p_io_service->stop();
}

netlib_server& netlib_server::get_instance() {
	static netlib_server instance;
	return instance;
}

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