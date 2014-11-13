/* ServerInteraction.hpp - test functions for programs which serve HTTP.
 *
 * g++ -g -o ./test ../client_server/test.cpp -lboost_unit_test_framework && ./test
 *
 * $Id: ServerInteraction.hpp,v 1 $
 */

#include <stdio.h>
#include <assert.h>
#include <sys/wait.h>   /* header for waitpid() and various macros */
#include <errno.h>

#ifdef _MSC_VER
  #define ntohl(X) _byteswap_ulong(X)
#else /* !_MSC_VER */
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif /* !_MSC_VER */

#include <stdexcept>
#include <boost/lexical_cast.hpp>

//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE SSLAPI
#include <boost/test/unit_test.hpp>

#define LOWPORT 0x9000
#define HIPORT  0x90ff

#define w3c_sw_STRINGIFY(x) #x
#define w3c_sw_TOSTRING(x) w3c_sw_STRINGIFY(x)
#define w3c_sw_LINE w3c_sw_LINEN << "LINE\n"
#define w3c_sw_LINEN std::cerr << __FILE__ "(" w3c_sw_TOSTRING(__LINE__) "): warning "
#define w3c_sw_LOCATION __FILE__ ":" w3c_sw_TOSTRING(__LINE__)
#define w3c_sw_NEED_IMPL(x) throw NotImplemented(__FILE__, w3c_sw_TOSTRING(__LINE__), x)
#define w3c_sw_IF_IMPL(t, x) if (t) {throw NotImplemented(__FILE__, w3c_sw_TOSTRING(__LINE__), x);}

namespace cliser {

    /** SigChildGuard - capture SIGCHLD signals and record the child return value.
     */
    struct SigChildGuard {
	static int ChildRet;		// A global to capture the result from a childHandler
	static bool ChildRetSet;	// and a marker for whether it's set.

	struct sigaction oldact;

	SigChildGuard () {
	    ChildRetSet = false;
	    if (sigaction(SIGCHLD, NULL, &oldact) < 0)
		throw std::runtime_error("unable to get current SIGCHLD action");

	    struct sigaction act;
	    sigemptyset(&act.sa_mask); // clear out act's sigset_t

	    act.sa_handler = childHandler;
	    act.sa_flags = SA_NOCLDSTOP; // only catch terminated children
	    if (sigaction(SIGCHLD, &act, NULL) < 0)
		throw std::runtime_error("unable to set SIGCHLD action");
	}

	~SigChildGuard () {
	    if (sigaction(SIGCHLD, &oldact, NULL) < 0)
		throw std::runtime_error("unable to restore old SIGCHLD action");
	}

	static void childHandler (int /* signo */) {
	    int status, child_val;

	    /* Wait for any child without blocking */
	    if (waitpid(-1, &status, WNOHANG) < 0) 
		; // sometimes returns error in linux
	    // throw std::runtime_error("signal caught but child failed to terminate");
	    if (WIFEXITED(status)) {            /* If child exited normally   */
		ChildRet = WEXITSTATUS(status); /*   get child's exit status. */
		ChildRetSet = true;
	    }
	}
    };
    int SigChildGuard::ChildRet = 0;		// A global to capture the result from a childHandler
    bool SigChildGuard::ChildRetSet = false;	// and a marker for whether it's set.


    /** substituteQueryVariables - perform the following substitutions on s:
     *   %p -> port.
     */
    std::string substituteQueryVariables (std::string s, int port) {
	std::string portStr = boost::lexical_cast<std::string>(port);
	for (size_t i = 0; i < s.length(); ) {
	    i = s.find("%p", i);
	    if (i == std::string::npos)
		break;
	    s.replace(i, 2, portStr);
	}
	return s;
    }

    struct ServerInteraction {
	std::string exe, path;
	std::string hostIP;
	int port;
	std::string serverS, serverURL;
	FILE *serverPipe;
	SigChildGuard g;

	ServerInteraction (std::string exe, std::string path,
			   std::string hostIP, std::string serverParams,
			   int lowPort, int highPort)
	    : exe(exe), path(path), hostIP(hostIP), port(firstOpenPort(hostIP, lowPort, highPort)),
	      serverURL("http://localhost:" + boost::lexical_cast<std::string>(port) + path)
	{
	    char line[80];

	    /* Start the server and wait for it to listen.
	     */
	    std::string serverCmd(// std::string("sleep 1 && ") + // slow start to reveal race conditions.
				  exe + " " + substituteQueryVariables(serverParams, port) + 
				  " --serve " + serverURL);
	    // serverCmd += " 2>&1 | tee -a server.mon";
	    // BOOST_LOG_SEV(w3c_sw::Logger::ProcessLog::get(), w3c_sw::Logger::info)
	    std::cerr << "serverCmd: " << serverCmd << std::endl;
	    serverPipe = popen(serverCmd.c_str(), "r");
	    if (serverPipe == NULL)
		throw std::string("popen") + strerror(errno);
	    if (fgets(line, sizeof line, serverPipe) == NULL ||
		strncmp("Working directory: ", line, 19))
		throw std::string(serverCmd + " didn't print a status line");
	    serverS += line;
	    waitConnect(hostIP, port);
	}

	~ServerInteraction () {
	    if (pclose(serverPipe) == -1 && errno != ECHILD)
		throw std::string() + "pclose(serverPipe)" + strerror(errno);
	}

	static int firstOpenPort (std::string ip, int start, int end) {
	    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    sockaddr_in remote;
	    remote.sin_family = AF_INET;
	    remote.sin_addr.s_addr = inet_addr(ip.c_str());

	    for (int port = start; port <= end; ++port) {
		remote.sin_port = htons(port);
		int ret = bind(sockfd, (struct sockaddr *) &remote, sizeof(remote));
		// w3c_sw_LINEN << " bind(" << port << ") = " << ret << " : " << strerror(errno) << "\n";
		if (ret != -1) {
		    close (sockfd);
		    return port;
		}
	    }
	    throw std::runtime_error("Unable to find an available port between "
				     + boost::lexical_cast<std::string>(start)
				     + " and "
				     + boost::lexical_cast<std::string>(end)
				     + ".");
	}

	/** waitConnect - busywait trying to connect to a port.
	 */
	static void waitConnect (std::string ip, int port) {
	    sockaddr_in remote;
	    remote.sin_family = AF_INET;
	    remote.sin_addr.s_addr = inet_addr(ip.c_str());
	    remote.sin_port = htons(port);

	    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    assert(sockfd != -1);

	    for (;;) {
		// assert(sockfd != -1);
		if (connect(sockfd, (struct sockaddr *) &remote, sizeof(remote)) < 0) {
		    // w3c_sw_LINEN << " still can't connect: " << strerror(errno) << "\n";
		} else {
		    // w3c_sw_LINEN << "Connected after " << (finish - start) << " seconds.\n";
		    close(sockfd);
		    break;
		}
	    }
	}

	/** waitLsof - busywait grepping for port in lsof.
	 * (An alternative to waitLsof.)
	 */
	static void waitLsof (int port) {
	    std::stringstream ss;
	    ss << "lsof -nP -iTCP -sTCP:LISTEN | grep " << port;
	    char buf[80];
	    for (;;) {
		struct sigaction child, old;
		child.sa_handler = SIG_IGN;
		sigaction(SIGCHLD, &child, &old);
		FILE* readyPipe = ::popen(ss.str().c_str(), "r");
		if (::fgets(buf, sizeof buf, readyPipe) != NULL)
		    break;
		else
		    w3c_sw_LINEN << "not yet\n";
		sigaction(SIGCHLD, &old, NULL);
		if (::pclose(readyPipe) == -1 && errno != ECHILD)
		    throw std::string("pclose(readyPipe)") + strerror(errno);
	    }
	    w3c_sw_LINEN << "running\n";
	}

	/** readToExhaustion - read and close an iostream.
	 */
	static void readToExhaustion (FILE* stream, std::string& str) {
	    char line[80];
	    while (fgets(line, sizeof line, stream) != NULL)
		str += line;
	}

    };


    /** SSLAPIServerInteraction - ivocations of the ./server binary.
     */
    struct SSLAPIServerInteraction : public ServerInteraction {
	SSLAPIServerInteraction (std::string exe, std::string serverParams, std::string serverPath, int lowPort, int highPort)
	    : ServerInteraction (exe, serverPath, "127.0.0.1", serverParams, lowPort, highPort)
	{  }
    };


    /** ClientServerInteraction - client interactions with ./server.
     */
    struct ClientServerInteraction : SSLAPIServerInteraction {
	std::string clientS;

	ClientServerInteraction (std::string exe, std::string serverParams, std::string serverPath, int lowPort, int highPort)
	    : SSLAPIServerInteraction(exe, serverParams, serverPath, lowPort, highPort)
	{  }

	void invoke (std::string clientCmd) {
	    // clientCmd += " | tee client.mon 2>&1";
	    // BOOST_LOG_SEV(Logger::ProcessLog::get(), Logger::info)
	    std::cerr << "clientCmd: " << clientCmd << std::endl;
	    char line[80];

	    /* Start the client and demand at least one line of output.
	     */
	    FILE *clientPipe = ::popen(clientCmd.c_str(), "r");
	    for (int tryNo = 0; tryNo < 2; ++tryNo)
		if (fgets(line, sizeof line, clientPipe) == NULL) {
		    if (errno != EINTR)
			throw std::string("no response from client process: ") + strerror(errno);
		} else {
		    clientS += line;
		    break;
		}

	    /* Read and close the client and server pipes.
	     */
	    readToExhaustion(clientPipe, clientS);
	    readToExhaustion(serverPipe, serverS);
	    if (pclose(clientPipe) == -1 && errno != ECHILD)
		throw std::string() + "pclose(clientPipe)" + strerror(errno);
	}

    };


    /** SPARQLClientServerInteraction - build a sparql client invocation string
     *  from the parameters used in the server invocation.
     */
    struct SPARQLClientServerInteraction : ClientServerInteraction {
	SPARQLClientServerInteraction (std::string serverParams, std::string serverPath,
				       std::string clientParams, int lowPort, int highPort)
	    : ClientServerInteraction("./server", serverParams, serverPath, lowPort, highPort)
	{
	    invoke("./client --service " + serverURL + " " + substituteQueryVariables(clientParams, port));
	}
    };

} // namespace cliser 

struct ServerTableQuery : cliser::SPARQLClientServerInteraction {
    ServerTableQuery (std::string serverParams,
		      std::string clientParams)
	: cliser::SPARQLClientServerInteraction (serverParams, "/SSLAPI", clientParams, LOWPORT, HIPORT)
    {  }
};

/* Simple SELECT.
 */
BOOST_AUTO_TEST_CASE( D_SELECT_SPO ) {
    ServerTableQuery i("--stop-after 1",
		       "client.crt client.key test ca.crt dh512.pem");
    std::string expected("Connected to /C=BE/ST=Some-State/O=Custodix/OU=maastro/CN=client\n");
    BOOST_CHECK_EQUAL(expected, i.clientS);
}



