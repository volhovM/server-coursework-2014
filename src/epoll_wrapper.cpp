#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <netdb.h>
#include <errno.h>

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <cstdio>
#include "tcp.h"

vm::epoll_wrapper::epoll_wrapper()
    : epoll_wrapper("localhost", "7273", false)
{}

vm::epoll_wrapper::epoll_wrapper(std::string host, std::string port, bool listening):
    on_socket_connect([](tcp_socket&&){}),
    on_data_income([](int fd){}),
    on_connection_closed([](int fd){}),
    socket_input(host, port, listening)
{
    efd = epoll_create1(0);
    if (efd == 0) {
	perror("epoll_create");
	abort();
    }
    socket_input.add_flag(O_NONBLOCK);
    epoll_event event;
    event.data.fd = socket_input.get_fd();
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, socket_input.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
}

vm::epoll_wrapper::~epoll_wrapper()
{
    close(efd);
}

void vm::epoll_wrapper::set_on_socket_connect(std::function<void(tcp_socket&&)> foo)
{
    on_socket_connect = foo;
}

void vm::epoll_wrapper::set_on_data_income(std::function<void(int)> foo)
{
    on_data_income = foo;
}

void vm::epoll_wrapper::set_on_connection_closed(std::function<void(int)> foo)
{
    on_connection_closed = foo;
}

void vm::epoll_wrapper::process_data_as_server()
{
    epoll_event *events = (epoll_event *) calloc(MAXEVENTS, sizeof(epoll_event));
    int n = epoll_wait(efd, events, MAXEVENTS, -1);
    //    std::cout << "epoll_wait succeeded, errno " << errno << " n " << n << std::endl;
    for (int i = 0; i < n; i++) {
	if ((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP))
	{
	    //	    std::cout << "EPOLL: " << "connection closed" << std::endl;
	    this->on_connection_closed(events[i].data.fd);
	}
	else if ((events[i].events & EPOLLERR) ||
	    (!(events[i].events & EPOLLIN)))
	{
	    //	    std::cout << "EPOLL: " << "events error" << std::endl;
	    continue;
	}
	else if (this->socket_input.get_fd() ==
		 events[i].data.fd)
	{
	    //	    std::cout << "EPOLL: " << "connection" << std::endl;
	    for (;;)
	    {
		//		std::cout << "EPOLL: " << "trying" << std::endl;
		sockaddr in_addr;
		socklen_t in_len;
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		in_len = sizeof in_addr;
		//	std::cout <<"waiting for accept" << std::endl;
		tcp_socket temp(socket_input.get_fd(), in_addr, in_len);
		if (!temp.is_valid())
		{
		    if ((errno == EAGAIN) ||
			(errno == EWOULDBLOCK))
		    {
			break;
		    }
		    //dat's strange
		    else
		    {
			perror("accept");
			break;
		    }
		} else this->on_socket_connect(std::move(temp));
		//		std::cout << "EPOLL: " << "calling on_socket_connect" << std::endl;
	    }
	    continue;
	}
	else
	{
	    std::cout << "EPOLL: " << "reading" << std::endl;
	    /* Let's read all the data available on event.fd
	       We also have edge-triggered mode, so we must
	       read all data as we won't get another notification
	    */
	    this->on_data_income(events[i].data.fd);
	}
    }
    free(events);
}

void vm::epoll_wrapper::process_data_as_client()
{
    epoll_event *events = (epoll_event *) calloc(MAXEVENTS, sizeof(epoll_event));
    std::cout << "waiting for epoll"  << std::endl;
    int n = epoll_wait(efd, events, MAXEVENTS, -1);
    std::cout << "epoll_wait succeeded, errno " << errno << " n " << n << std::endl;
    for (int i = 0; i < n; i++) {
	if ((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP))
	{
	    std::cout << "EPOLL: " << "connection closed" << std::endl;
	    this->on_connection_closed(events[i].data.fd);
	}
	else if ((events[i].events & EPOLLERR) ||
	    (!(events[i].events & EPOLLIN)))
	{
	    std::cout << "EPOLL: " << "events error" << std::endl;
	    continue;
	}
	else if (this->socket_input.get_fd() ==
		 events[i].data.fd)
	{
	    //	    std::cout << "EPOLL: " << "connection" << std::endl;
	    std::cout << "EPOLL: " << "reading" << std::endl;
	    this->on_data_income(events[i].data.fd);
	}
    }
    free(events);
}

void vm::epoll_wrapper::add_socket(tcp_socket& socket)
{
    epoll_event event;
    event.data.fd = socket.get_fd();
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    //std::cout << "adding " << socket.get_fd() << std::endl;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, socket.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
}
