#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <stdexcept>
#include <utility>
#include "tcp.h"
using namespace vm;

tcp_socket::tcp_socket(std::string hostname, std::string port)
    : sfd(-1)
{
    std::cout << "initing socket 1" << hostname << " " << port << std::endl;
    init(hostname, port);
}

tcp_socket::tcp_socket(std::string hostname, std::string port, bool srv)
    : sfd(-1)
    , is_server(srv)
{
    std::cout << "initing socket 2" << hostname << " " << port << std::endl;
    init(hostname, port);
}

// accepting
tcp_socket::tcp_socket(int infd, sockaddr in_addr, socklen_t in_len)
    : is_server(false)
{
    sfd = accept(infd, &in_addr, &in_len);
}


tcp_socket::~tcp_socket() {
    // TODO can return err
    close_fd();
}

tcp_socket::tcp_socket(tcp_socket&& that) {
    sfd = that.sfd;
    that.invalidate();
}

void tcp_socket::invalidate()
{
    sfd = -1;
}

bool tcp_socket::is_valid()
{
    return sfd != -1;
}

void tcp_socket::close_fd()
{
    //    std::cout << "closing fd: " << sfd << std::endl;
    close(sfd);
    invalidate();
}

int tcp_socket::get_fd() const {
    return sfd;
}

void tcp_socket::set_listening() {
    // TODO add err handling
    if (is_server) listen(sfd, SOMAXCONN);
    else throw std::runtime_error("You don't want to listen to client socket");
}

void tcp_socket::init(std::string hostname, std::string port) {
    addrinfo hints;
    addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (hostname == "") hostname = "localhost";
    int s = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	throw std::runtime_error("getaddrinfo fail for host " + hostname + " with port " + port);
    }

    //try all results until manage to bind to some address
    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (sfd == -1)
	    continue;
	if (is_server)
	{
	    int yes = 1;
	    setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	    std::cout << "binding" << std::endl;
	    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
	    {
		//ok;
		break;
	    }
	} else
	{
	    std::cout << "connecting" << std::endl;
	    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
	    {
		break;
	    }
	}
	std::cout << "Could not bind/connect, errno: " << strerror(errno) << std::endl;
	close(sfd);
    }
    // no address succeeded
    if (rp == NULL) {
	throw std::runtime_error("could not bind/connect to " + hostname + ":" + port);
    }
    std::cout << "Binded/connected to " << sfd << std::endl;
    // no longer needed
    freeaddrinfo(result);

    if (is_server) set_listening();
}

void tcp_socket::add_flag(int flag) {
    //    std::cout << "addinng flag " << flag << " to socket " << sfd << std::endl;
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
	throw std::runtime_error("fcntl1, errno " + std::to_string(errno));
    }
    flags |= flag;
    if (fcntl(sfd, F_SETFL, flags) == -1) {
	throw std::runtime_error("fcntl2" + std::to_string(errno));
    }
}

void tcp_socket::send(const std::string str) {
    //    std::cout << "writing char* '" << input << "' of size " << sizeof(input) << " and length "
    //      << strlen(input) << " into " << sfd << std::endl;
    write(sfd, str.c_str(), str.length());
}

std::string tcp_socket::recieve() {
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

std::pair<std::string, std::string> vm::tcp_socket::get_address()
{
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;

    len = sizeof addr;
    getpeername(sfd, (struct sockaddr*)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
	struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
	struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
	port = ntohs(s->sin6_port);
	inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    return std::make_pair(std::string(ipstr), std::to_string(port));
}

bool vm::tcp_socket::is_server_type()
{
    return is_server;
}
