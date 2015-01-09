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
#include "logger.h"

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
    vm::log_d("before epoll_wait");
    int n = epoll_wait(efd, events, MAXEVENTS, -1);
    vm::log_d("epoll_wait succeeded, errno " + std::to_string(errno) + " n " + std::to_string(n));
    for (int i = 0; i < n; i++) {
	int evfd = events[i].data.fd;
	if ((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP))
	{
	    vm::log_d("epoll: connection closed");
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
	    vm::log_e("epoll: events error");
	    continue;
	}
	else
	{
	    vm::log_d("epoll: data_income");
	    if (handlers.find(evfd) != handlers.end())
	    {
		vm::log_d("epoll: calling data_income_handler " + std::to_string(evfd));
		epoll_handler curr = handlers[evfd];
		curr.on_data_income(evfd);
	    }
	    vm::log_d("epoll: connection passed");
	}
    }
    free(events);
}

std::function<void(int)> vm::epoll_wrapper::on_socket_connect(std::function<void(vm::tcp_socket&&)> socket_handler)
{
    return [socket_handler](int fd)
    {
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
		vm::log_e("accept failed");
	    }
	    else
	    {
		perror("accept");
		vm::log_e("accept_failed");
	    }
	}
	socket_handler(std::move(temp));
    };
}

void vm::epoll_wrapper::add_socket(tcp_socket& socket, epoll_handler handler)
{
    vm::log_d("epoll: num " + std::to_string(efd));
    vm::log_d("epoll: adding socket with fd " + std::to_string(socket.get_fd()));
    epoll_event event;
    event.data.fd = socket.get_fd();
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, socket.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
    handlers.insert(std::make_pair(socket.get_fd(), handler));
    vm::log_d("epoll: added " + std::to_string(socket.get_fd()));
}

void vm::epoll_wrapper::remove_socket(int fd)
{
    vm::log_d("epoll: removing "  + std::to_string(fd));
    handlers.erase(fd);
    epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
}
