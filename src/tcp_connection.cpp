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
#include <algorithm>
#include <stdexcept>
#include <new>
#include "tcp.h"

vm::tcp_connection::tcp_connection(): socket("localhost", "7273") {}
vm::tcp_connection::tcp_connection(tcp_socket&& that): socket(std::move(that)) {}
vm::tcp_connection::tcp_connection(std::string host, std::string port): socket(host, port) {}
vm::tcp_connection::tcp_connection(tcp_connection&& that): socket(std::move(that.get_socket()))
{
    that.get_socket().invalidate();
}

void vm::tcp_connection::connect_to(std::string host, std::string port)
{
    new (&socket) tcp_socket(host, port);
}

std::string vm::tcp_connection::recieve_data()
{
    return socket.recieve();
}

void vm::tcp_connection::send_data(std::string data)
{
    socket.send(data);
}

void vm::tcp_connection::disconnect()
{
    socket.close_fd();
}

vm::tcp_socket& vm::tcp_connection::get_socket()
{
    return socket;
}

int vm::tcp_connection::get_fd()
{
    return socket.get_fd();
}

bool vm::tcp_connection::operator!=(const tcp_connection& that)
{
    return socket.get_fd() != that.socket.get_fd();
}
