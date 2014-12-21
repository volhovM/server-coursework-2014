#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <stdexcept>
#include "io.h"
using namespace vm;

tcp_socket::tcp_socket(std::string hostname, std::string port) : sfd(-1) {
    init(hostname, port);
}

tcp_socket::tcp_socket(std::string hostname, std::string port, bool srv) : sfd(-1), is_server(srv) {
    init(hostname, port);
}

tcp_socket::tcp_socket(int fd) {
    sfd = fd;
}

tcp_socket::~tcp_socket() {
    // TODO can return err
    close(sfd);
}

tcp_socket::tcp_socket(tcp_socket&& that) {
    sfd = that.sfd;
    that.sfd = -1;
}

int tcp_socket::get_fd() {
    return sfd;
}

void tcp_socket::set_listening() {
    // TODO add err handling
    listen(sfd, SOMAXCONN);
}

void tcp_socket::init(std::string hostname, std::string port) {
    addrinfo hints;
    addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    if (hostname == "") hostname = "localhost";
    int s = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	throw std::runtime_error("getaddrinfo fail for " + hostname + " with " + port);
    }

    //try all results until manage to bind to some address
    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (sfd == -1)
	    continue;
	int yes = 1;
	if (is_server) setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
	    //ok;
	    break;
	}
	close(sfd);
    }
    // no address succeeded
    if (rp == NULL) {
	throw std::runtime_error("could not bind to " + hostname + " :" + port);
    }
    // no longer needed
    freeaddrinfo(result);

    if (is_server) set_listening();
}

void tcp_socket::add_flag(int flag) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
	throw std::runtime_error("fcntl1");
    }
    flags |= flag;
    if (fcntl(sfd, F_SETFL, flags) == -1) {
	throw std::runtime_error("fcntl2");
    }
}

void tcp_socket::send(const std::string str) {
    //    std::cout << "writing char* '" << input << "' of size " << sizeof(input) << " and length "
    //      << strlen(input) << " into " << sfd << std::endl;
    write(sfd, str.c_str(), str.length());
}

std::string tcp_socket::get() {
    ssize_t count;
    int len = 512;
    char buf[len];
    std::string ret = "";
    while (true) {
	count = read(sfd, buf, len);
	if (count == -1) {
	    // if errno == EAGAIN, we have read all data
	    if (errno != EAGAIN) {
		std::cerr << "EAGAIN while reading socket fd #" << sfd << std::endl;
	    } else
	    {
		return ret;
	    }
	} else if (count == 0)
	{
	    //eof
	    return ret;
	}
	buf[count] = '\0';
	ret += std::string(buf);
    }
}
