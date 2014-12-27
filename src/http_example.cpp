#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <map>
#include "tcp.h"
#include "http.h"
using namespace vm;

int main3(int argc, char *argv[]) {
    http_connection connection("neerc.ifmo.ru");
    http_request request;
    request.set_request_method(http_request::GET);
    request.set_url("/~sta/index.html");
    request.add_header("Host", "neerc.ifmo.ru");
    request.add_header("Connection", "keep-alive");
    std::cout << "sending request1: " << std::endl << request.commit() << std::endl;
    http_request request2;
    request2.set_request_method(http_request::GET);
    request2.set_url("/~sta/2014-2015/2-algorithms/index.html");
    request2.add_header("Host", "neerc.ifmo.ru");
    request2.add_header("Connection", "close");
    std::cout << "sending request2: " << std::endl << request2.commit() << std::endl;
    std::vector<http_request> v;
    v.push_back(request);
    v.push_back(request2);
    bool run = true;
    epoll_wrapper epoll;
    connection.query(v,
		     [&](std::vector<http_response> rs)
		     {
			 std::cout << rs[0].commit() << std::endl;
			 std::cout << rs[1].commit() << std::endl;
			 run = false;
		     },
		     epoll);
    while(run)
    {
	epoll.process_events();
    }
    std::cout << "Now exiting" << std::endl;
    return 0;
}
