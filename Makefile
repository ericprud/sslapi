COMPILE=g++ -g -c
LINK=g++ -g

BOOST_INCLUDE=../../boost_1_55_0
BOOST_LIB=../../libboost-1.55.install/lib
NETLIB_INCLUDE=cpp-netlib
NETLIB_LIB=cpp-netlib-proj3/libs/network/src

INCLUDES= -I$(BOOST_INCLUDE) -I$(NETLIB_INCLUDE)
LIBS= -L $(BOOST_LIB) -lboost_system-mt -lboost_thread-mt -lpthread -L $(NETLIB_LIB) -lcppnetlib-client-connections -lcppnetlib-server-parsers -lcrypto -lssl

# g++ -g -c -o ssl_server.o ssl_server.cpp -I../../boost_1_55_0 -Icpp-netlib
ssl_server.o: ssl_server.cpp
	$(COMPILE) -o $@ $< $(INCLUDES)

# g++ -g -o ssl_server ssl_server.o -L ../../libboost-1.55.install/lib -L cpp-netlib-proj3/libs/network/src/ -lboost_system-mt -lboost_thread-mt -lpthread -L cpp-netlib-proj/libs/network/src -lcppnetlib-client-connections -lcppnetlib-server-parsers -lcrypto -lssl
ssl_server: ssl_server.o
	$(LINK) -o $@ $< $(LIBS)

# LD_LIBRARY_PATH=../../libboost-1.55.install/lib ./ssl_server
test: ssl_server
	LD_LIBRARY_PATH=$(BOOST_LIB):$(LD_LIBRARY_PATH) $<
	@echo now run curl -k --cert client.pem:test https://localhost:8442

