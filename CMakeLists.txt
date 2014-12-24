cmake_minimum_required(VERSION 2.8)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (CourseWork)

add_executable(main
	       src/tcp.h
	       src/tcp_socket.cpp
	       src/tcp_server.cpp
	       src/tcp_client.cpp
	       src/epoll_wrapper.cpp
	       src/server.cpp
	       src/http.h
	       src/http_response.cpp
	       src/http_request.cpp
	       src/http_server.cpp
	       src/http_client.cpp
	       src/http_example.cpp
)

list( APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs -O2")
