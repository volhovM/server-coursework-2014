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
#include <string>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <stdexcept>
#include <utility>
#include "tcp.h"
#include "logger.h"
using namespace vm;

tcp_socket::tcp_socket(std::string hostname, std::string port, bool srv)
    : sfd(-1)
    , is_server(srv)
{
    vm::log_d("socket: creating socket " + hostname + " " + port + " of type server: " +
              std::to_string(is_server));
    addrinfo hints;
    addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (hostname == "") hostname = "localhost";
    int s = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    vm::log_d("socket: got addr_info");
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
            vm::log_d("socket: binding");
            if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            {
                //ok;
                break;
            }
        } else
        {
            vm::log_d("socket: connecting");
            if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            {
                break;
            }
        }
        vm::log_e("socket: could not bind/connect, errno: " + std::to_string(errno));
        close(sfd);
    }
    // no address succeeded
    if (rp == NULL) {
        throw std::runtime_error("could not bind/connect to " + hostname + ":" + port);
    }
    vm::log_d("Binded/connected to " + std::to_string(sfd));
    // no longer needed
    freeaddrinfo(result);
    if (is_server) set_listening();
}

tcp_socket::tcp_socket(std::string hostname, std::string port)
    : tcp_socket(hostname, port, false)
{}

// accepting
tcp_socket::tcp_socket(int master_fd, sockaddr in_addr, socklen_t in_len)
    : is_server(false)
{
    sfd = accept(master_fd, &in_addr, &in_len);
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
    vm::log_d("socket: closing fd: " + std::to_string(sfd));
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

void tcp_socket::revert_flag(int flag) {
    //    std::cout << "addinng flag " << flag << " to socket " << sfd << std::endl;
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl1, errno " + std::to_string(errno));
    }
    flags ^= flag;
    if (fcntl(sfd, F_SETFL, flags) == -1) {
        throw std::runtime_error("fcntl2" + std::to_string(errno));
    }
}

void tcp_socket::send(const std::string str) {
    //    std::cout << "writing char* '" << input << "' of size " << sizeof(input) << " and length "
    //      << strlen(input) << " into " << sfd << std::endl;
    vm::log_d("socket #" + std::to_string(sfd) + " writing "
              + std::to_string(str.length()) + " chars");
    if (sfd == 0) vm::log_e("socket #0, wouldn't write to it");
    else
    {
        revert_flag(O_NONBLOCK);
        int written = write(sfd, str.c_str(), str.length());
        if (written == -1)
        {
            vm::log_e("socket: error when writing, errno #" +
                      std::to_string(errno));
        } else
        {
            vm::log_d("socket: writed bytes/of: " +
                      std::to_string(written) + "/" +
                      std::to_string(str.length()));
        }
        revert_flag(O_NONBLOCK);
    }
}

//std::string tcp_socket::recieve() {
//    std::cout << "reading from socket " << sfd << std::endl;
//    ssize_t count;
//    int len = 512;
//    char buf[len];
//    std::string ret = "";
//    while (true) {
//	std::cout << "before read from " << sfd << std::endl;
//	count = read(sfd, buf, len);
//	std::cout << "after read from " << sfd << " got " << count << std::endl;
//	if (count == -1) {
//          // if errno == EAGAIN, we have read all data
//          if (errno != EAGAIN) {
//		std::cerr << "EAGAIN while reading socket fd #" << sfd << std::endl;
//          } else
//          {
//		return ret;
//          }
//	} else if (count == 0)
//	{
//          //eof
//          return ret;
//	}
//	buf[count] = '\0';
//	ret += std::string(buf);
//    }
//}


//std::string tcp_socket::recieve()
//{
//    std::string ret;
//    ssize_t count;
//    int len = 512;
//    char buf[len];
//    while(true)
//    {
//	vm::log_d("socket: before recv");
//	count = recv(sfd, buf, len, 0);
//	vm::log_d("socket: after recv, got " + std::to_string(count));
//	if(count == -1) {
//          if(errno != EAGAIN && errno != EWOULDBLOCK) {
//		throw std::runtime_error("Error while reading the socket, errno "
//                                       + std::to_string(errno));
//          }
//          break;
//	} else if (count == 0) {
//          break;
//	}
//	ret.append(buf, (size_t) count);
//    }
//    return ret;
//}

std::string tcp_socket::recieve()
{
    std::string ret;
    int len = 0;
    ioctl(sfd, FIONREAD, &len);
    if (len > 0) {
        char buf[len];
        len = read(sfd, buf, len);
        ret.append(buf, (size_t) len);
        return ret;
    } else return "";
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
