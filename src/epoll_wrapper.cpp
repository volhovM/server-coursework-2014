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
{
    efd = epoll_create1(0);
    if (efd == 0) {
	perror("epoll_create");
	abort();
    }
}

vm::epoll_wrapper::~epoll_wrapper()
{
    close(efd);
}

void vm::epoll_wrapper::process_events()
{
    epoll_event *events = (epoll_event *) calloc(MAXEVENTS, sizeof(epoll_event));
    std::cout << "before epoll_wait" << std::endl;
    int n = epoll_wait(efd, events, MAXEVENTS, -1);
    std::cout << "epoll_wait succeeded, errno " << errno << " n " << n << std::endl;
    for (int i = 0; i < n; i++) {
	int evfd = events[i].data.fd;
	if ((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP))
	{
	    std::cout << "EPOLL: " << "connection closed" << std::endl;
	    if (handlers.find(evfd) != handlers.end())
	    {
		epoll_handler curr = handlers[evfd];
		curr.on_disconnect(evfd);
		remove_socket(evfd);
	    }
	}
	else if ((events[i].events & EPOLLERR) ||
	    (!(events[i].events & EPOLLIN)))
	{
	    std::cout << "EPOLL: " << "events error" << std::endl;
	    continue;
	}
	else
	{
	    std::cout << "EPOLL: " << "data_income" << std::endl;
	    if (handlers.find(evfd) != handlers.end())
	    {
		std::cout << "calling data_income_handler " << evfd << std::endl;
		epoll_handler curr = handlers[evfd];
		curr.on_data_income(evfd);
	    }
	    std::cout << "EPOLL: " << "connection passed" << std::endl;
	}
    }
    free(events);
}

std::function<void(int)> vm::epoll_wrapper::on_socket_connect(std::function<void(vm::tcp_socket&&)> socket_handler)
{
    return [socket_handler](int fd)
    {
	//std::cout << "EPOLL: " << "trying" << std::endl;
	sockaddr in_addr;
	socklen_t in_len;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	in_len = sizeof in_addr;
	tcp_socket temp(fd, in_addr, in_len);
	if (!temp.is_valid())
	{
	    if ((errno == EAGAIN) ||
		(errno == EWOULDBLOCK))
	    {
		std::cerr << "ACCEPT FAILED" << std::endl;
	    }
	    else
	    {
		perror("accept");
		std::cerr << "ACCEPT" << std::endl;
	    }
	}
	socket_handler(std::move(temp));
	//std::cout << "AFTER SOCKET_HANDLER IN EPOLL" << std::endl;
	//		std::cout << "EPOLL: " << "calling on_socket_connect" << std::endl;
	//std::cout << "EPOLL EXITING SOCKET ADD FOO" << std::endl;
    };
}

void vm::epoll_wrapper::add_socket(tcp_socket& socket, epoll_handler handler)
{
    std::cout << "EPOLL: adding socket with fd " << socket.get_fd() << std::endl;
    epoll_event event;
    event.data.fd = socket.get_fd();
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    //std::cout << "adding " << socket.get_fd() << std::endl;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, socket.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
    handlers.insert(std::make_pair(socket.get_fd(), handler));
    std::cout << "EPOLL: added " << socket.get_fd() << std::endl;
}

void vm::epoll_wrapper::remove_socket(int fd)
{
    std::cout << "EPOLL: removing " << fd << std::endl;
    handlers.erase(fd);
}
