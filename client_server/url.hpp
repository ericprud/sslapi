#include <boost/regex.hpp>

struct URL {
    std::string serviceHost;
    std::string ports;
    int servicePort;
    std::string servicePath;

    URL (const char* s) {
	    boost::regex re;
	    boost::cmatch matches;

	    re = "(?:(ftp|http|https):)?\\/\\/((?:\\w+\\.)*\\w*)(?::([0-9]+))?(.*)";
	    if (!boost::regex_match(s, matches, re)) {
		std::cerr << "Address " << s << " is not a valid URL" << std::endl;
		exit(1);
	    }

#define PROT 1
#define HOST 2
#define PORT 3
#define PATH 4
	    serviceHost = std::string(matches[HOST].first, matches[HOST].second);
	    ports = std::string(matches[PORT].first, matches[PORT].second);
	    servicePort = ::atoi(ports.c_str());
	    servicePath = std::string(matches[PATH].first + 1, matches[PATH].second);
    }
};

