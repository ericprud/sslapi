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
FIND_PACKAGE(Boost 1.54 COMPONENTS thread date_time program_options filesystem system chrono regex unit_test_framework REQUIRED)

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
url.hpp
server.cpp)

# Client sources
SET(CLIENT_SRC
url.hpp
client.cpp)

# Test sources
SET(TESTSERVER_SRC
testServer.cpp)

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
target_link_libraries(server ${Boost_THREAD_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY} ${OPENSSL_LIBRARIES} ${server_parsers} ${CMAKE_THREAD_LIBS_INIT})

# Client
add_executable(client ${CLIENT_SRC})
target_link_libraries(client ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY} ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_DATE_TIME_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} )


# Test adds -lboost_unit_test_framework
add_executable(testServer ${TESTSERVER_SRC})

if(WIN32)
  target_link_libraries(testServer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} wsock32 ws2_32)
else(WIN32)
	target_link_libraries(testServer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
endif()

enable_testing()
add_test(server testServer)
# how do i say that target test depends target server?
add_custom_target(test COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS server testServer)
# add_dependencies(test server testServer)

# Copy the keys to the directory where the server executable is located (some extra keys are included which can be used by the client)
foreach( key ${KEYS} )
  add_custom_command(TARGET server POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJECT_SOURCE_DIR}/${key}"
		$<TARGET_FILE_DIR:server>)
endforeach( key )