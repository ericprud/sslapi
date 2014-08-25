#########################################################
## CMake file used to build ssl client/server examples ##
#########################################################
cmake_minimum_required(VERSION 2.8)

project(SSLAPI)

# Make sure we use boost static libs + multithreading
set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_MULTITHREADED ON)

# Find openssl + boost
FIND_PACKAGE(OpenSSL)
FIND_PACKAGE(Boost 1.55 COMPONENTS thread date_time program_options filesystem system chrono regex REQUIRED)

# Find CPP-NETLIB
set(CPP_NETLIB_INCLUDE_DIR "" CACHE FILEPATH "Path to cpp-netlib source dir")
set(CPP_NETLIB_LIB_DIR "" CACHE FILEPATH "Path to cpp-netlib lib dir")

# Find boost threads
find_package(Threads REQUIRED)

# Find cpp-netlib libs
find_library(server_parsers cppnetlib-server-parsers ${CPP_NETLIB_LIB_DIR})

# These are not required
#find_library(client_conns cppnetlib-client-connections ${CPP_NETLIB_LIB_DIR})
#find_library(uri cppnetlib-uri ${CPP_NETLIB_LIB_DIR})

# If netlib is found, include it
if(CPP_NETLIB_INCLUDE_DIR)
INCLUDE_DIRECTORIES(${CPP_NETLIB_INCLUDE_DIR})
else(CPP_NETLIB_INCLUDE_DIR)
message(FATAL_ERROR "CPP-netlib not found")
endif(CPP_NETLIB_INCLUDE_DIR)

INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})

# If boost is found, include it
if(Boost_FOUND)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
else(Boost_FOUND)
message(FATAL_ERROR "Boost not found")
endif(Boost_FOUND)

# Server sources
SET(SERVER_SRC
ssl_server.cpp
server.h
server.cpp)

# Client sources
SET(CLIENT_SRC
client.cpp)

# Define all keys
SET(KEYS
ca.crt
ca.key
client.crt
client.key
client.pem
dh512.pem
server.crt
server.key
server.pem)

# Server
add_executable(server ${SERVER_SRC})
target_link_libraries(server ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${server_parsers} ${CMAKE_THREAD_LIBS_INIT})

# Client
add_executable(client ${CLIENT_SRC})
target_link_libraries(client ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Copy the keys to the directory where the server executable is located (some extra keys are included which can be used by the client)
foreach( key ${KEYS} )
  add_custom_command(TARGET server POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/${key}"
		$<TARGET_FILE_DIR:server>)
endforeach( key )