#include <functional>
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
#include "tcp.h"
using namespace vm;

tcp_server::tcp_server()
    : on_data_income([](std::map<int, tcp_client>&, tcp_client&){})
    , on_connect([](std::map<int, tcp_client>&, tcp_client&){})
    , on_disconnect([](std::map<int, tcp_client>&, tcp_client&){})
{ init(); }

tcp_server::tcp_server(std::string host, std::string port)
    : on_data_income([](std::map<int, tcp_client>&, tcp_client&){})
    , on_connect([](std::map<int, tcp_client>&, tcp_client&){})
    , on_disconnect([](std::map<int, tcp_client>&, tcp_client&){})
    , epoll(host, port)
{ init(); }

void tcp_server::init()
{
    epoll.set_on_socket_connect([&](int fd)
				{
				    clients.insert(std::make_pair(fd, tcp_client(fd)));
				    /* Make the incoming socket non-blocking and add it to the
				       list of fds to monitor. */
				    clients[fd].id = id_counter++;
				    clients[fd].get_socket().add_flag(O_NONBLOCK);
				    // same as with first socket
				    epoll.add_socket(clients[fd].get_socket());
				    std::cout << "Added a client on socket with fd "
					      << fd << std::endl;
				    this->on_connect(clients, clients[fd]);
				});
    epoll.set_on_data_income([&](int fd)
			     {
				 // c++ cannot into linear search on vector, mda...
				 this->on_data_income(clients, clients[fd]);
				 //else std::cerr << "Got data from socket server is not tracking"
				 //		<< std::endl;
			     });
    epoll.set_on_connection_closed([&](int fd)
				   {
				       disconnect_client(clients[fd]);
				   });
}

void tcp_server::set_on_data_income(event_h handler)
{
    on_data_income = handler;
}

void tcp_server::set_on_connect(event_h handler)
{
    on_connect = handler;
}

void tcp_server::set_on_disconnect(event_h handler)
{
    on_disconnect = handler;
}

void tcp_server::start() {
    running = true;
    while (running) {
	epoll.process_data();
    }
}

void tcp_server::stop() {
    running = false;
}

bool tcp_server::is_running() {
    return running;
}

void tcp_server::disconnect_client(tcp_client& client)
{
    on_disconnect(clients, client);
    clients.erase(client.get_socket().get_fd());
    std::cout << "Connection closed on fd " << client.get_socket().get_fd() << std::endl;
}
