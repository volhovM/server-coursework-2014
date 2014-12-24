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

vm::tcp_client::tcp_client(): socket("localhost", "127.0.0.1") {}
vm::tcp_client::tcp_client(int fd): socket(fd) {}
vm::tcp_client::tcp_client(std::string host, std::string port): socket(host, port) {}
vm::tcp_client::tcp_client(tcp_client&& that): socket(that.get_socket().get_fd())
{
    that.get_socket().invalidate();
}
void vm::tcp_client::connect_to(std::string host, std::string port)
{
    new (&socket) tcp_socket(host, port);
}
std::string vm::tcp_client::recieve_data()
{
    return socket.recieve();
}

void vm::tcp_client::send_data(std::string data)
{
    socket.send(data);
}

void vm::tcp_client::disconnect()
{
    socket.close_fd();
}
vm::tcp_socket& vm::tcp_client::get_socket()
{
    return socket;
}
bool vm::tcp_client::operator==(const tcp_client& that)
{
    return socket.get_fd() == that.socket.get_fd();
}
bool vm::tcp_client::operator!=(const tcp_client& that)
{
    return socket.get_fd() != that.socket.get_fd();
}
