cmake_minimum_required(VERSION 2.8)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (CourseWork)

set(SOURCE_FILES src/tcp.h
		 src/tcp_socket.cpp
		 src/epoll_wrapper.cpp
		 src/tcp_connection.cpp
		 src/server.cpp
		 src/http.h
		 src/http_request.cpp
		 src/http_response.cpp
		 src/http_connection.cpp
		 src/http_server.cpp
		 src/proxy.cpp
)

add_executable(CourseWork ${SOURCE_FILES})

list( APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs -O2")
