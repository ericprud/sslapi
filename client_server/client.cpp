// Client.cpp : Defines the entry point for the console application.
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>

#include <boost/exception/all.hpp>

#include "url.hpp"

enum {
    max_length = 1024
};

class keypass 
{
public:
	keypass(std::string password) : pass(password) {}
	
	/**
	 * Password callback for the private key
	 */
	std::string password_callback(std::size_t max_length, boost::asio::ssl::context_base::password_purpose purpose) {
		return pass;
	}

private:
	std::string pass;
};

class client
{
public:
	
	/// Create a new client with the given settings
    static client* create_client(std::string host, std::string port, std::string path, std::string client_cert, std::string client_key, std::string client_keypass, std::string server_cert, std::string dh_file) {
		
		boost::asio::io_service io_service;

        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(host,port);
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

		// Create pass callback
		keypass pass(client_keypass);

		// Initialize ssl context
        boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::sslv23);

        ctx.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::single_dh_use);

		ctx.set_password_callback(boost::bind(&keypass::password_callback, &pass, _1, _2));
        ctx.set_verify_mode(boost::asio::ssl::context::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
		
		ctx.load_verify_file(server_cert);

		ctx.use_certificate_chain_file(client_cert);

		ctx.use_private_key_file(client_key, boost::asio::ssl::context::pem);

		
        ctx.use_tmp_dh_file(dh_file);

	client *c = new client(io_service, ctx, iterator, path);
		

		return c;

	}
	
	/// Handle the client connect, starts the handshake
    void handle_connect(const boost::system::error_code& error,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
        if (!error) {
            socket_.async_handshake(boost::asio::ssl::stream_base::client,
                    boost::bind(&client::handle_handshake, this,
                    boost::asio::placeholders::error));
        } else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
            socket_.lowest_layer().close();
            boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
            socket_.lowest_layer().async_connect(endpoint,
                    boost::bind(&client::handle_connect, this,
                    boost::asio::placeholders::error, ++endpoint_iterator));
        } else {
            std::cout << "Connect failed (" << error << "): " << error.message()  << "\n";
        }
    }

	/// Handle SSL handshake, send your message
    void handle_handshake(const boost::system::error_code& error) {
        if (!error) {
#if 0			
            std::cout << "Enter destination: ";
			char in[1024];

            std::cin.getline(in, max_length);
			// Send an actual http post
			snprintf(request_, sizeof(request_),"GET /%s HTTP/1.1\r\n\r\n", in);
			//sprintf_s(request_,"GET /%s HTTP/1.1\r\n\r\n", in);
#endif
#ifdef _WIN32
			sprintf(request_,"GET /%s HTTP/1.1\r\n\r\n", path.c_str());
#else
			sprintf(request_, sizeof(request_),"GET /%s HTTP/1.1\r\n\r\n", path.c_str());
#endif
            size_t request_length = strlen(request_);
			// Print the request
			// std::cout << "Sending request:" << std::endl << request_ << std::endl;

            boost::asio::async_write(socket_,
                    boost::asio::buffer(request_, request_length),
                    boost::bind(&client::handle_write, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            std::cout << "Handshake failed (" << error << "): " << error.message()  << "\n";
        }
    }

	/// Handle client write
    void handle_write(const boost::system::error_code& error,
            size_t bytes_transferred) {
        if (!error) {
			// Server message is terminated by a newline, wait untill we get it
			boost::asio::async_read_until(socket_,
					reply_,
					std::string("</HTML>"),
                    boost::bind(&client::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            std::cout << "Write failed (" << error << "): " << error.message()  << "\n";
        }
    }

	/// Handle the server answer
    void handle_read(const boost::system::error_code& error,
            size_t bytes_transferred) {
        if (!error) {
#if 0
            std::cout << "Reply: ";

            std::cout.write(boost::asio::buffer_cast<const char*>(reply_.data()), bytes_transferred);
            std::cout << "\n";
#endif
            std::string text(boost::asio::buffer_cast<const char*>(reply_.data()), bytes_transferred);
	    size_t back = text.find("client: ")+strlen("client: ");
	    size_t front = text.find("\n", back);
	    std::cout << "Connected to " << text.substr(back, front-back) << std::endl;
        } else {
			std::cout << "Read failed (" << error << "): " << error.message()  << "\n";
        }
    }

	~client(void) {}
	
private:
	
	/// Actual construction of the client
	client(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
	       boost::asio::ip::tcp::resolver::iterator endpoint_iterator, std::string path)
	    :  io_service_(io_service), socket_(io_service_, context), path(path) {
		
        boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
        socket_.lowest_layer().async_connect(endpoint,
                boost::bind(&client::handle_connect, this,
                boost::asio::placeholders::error, ++endpoint_iterator));
		io_service_.run();
    }

	boost::asio::io_service &io_service_;

    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
	char request_[max_length];
	boost::asio::streambuf reply_;
    std::string path;
};

/// Main method, allow configuration of the client
// --service http://localhost:36864/SSLAPI TestConnection
int main(int argc, const char* argv[])
{
	try {
	if(argc != 8) {
		printf("%d, %s",argc, argv[1]);
		std::cerr << "Usage: client --service <url> <client_cert> <client_key> <client_keypass> <server_cert> <dh_file>" << std::endl;
		exit(1);
	}

	URL u(argv[2]);
	// std::string host(argv[1]);
	// std::string port(argv[2]);
	std::string client_cert(argv[3]);
	std::string client_key(argv[4]);
	std::string client_keypass(argv[5]);
	std::string server_cert(argv[6]);
	std::string dh_file(argv[7]);
	client::create_client(u.serviceHost,u.ports,u.servicePath,client_cert,client_key,client_keypass,server_cert,dh_file);

	} catch(boost::exception const&  ex) {
		std::cerr << "Boost error: " << boost::diagnostic_information(ex) << std::endl;
		exit(1);
    } catch(std::exception const& e) {
		std::cerr << "Std exception: " << e.what() << std::endl;
		exit(1);
	}
	return 0;
}

