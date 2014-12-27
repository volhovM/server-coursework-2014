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
#include <type_traits>
#include "tcp.h"


vm::tcp_server<T>::tcp_server(std::string host, std::string port)
    : on_data_income([](std::map<int, T>&, T&){})
    , on_connect([](std::map<int, T>&, T&){})
    , on_disconnect([](std::map<int, T>&, T&){})
    , epoll(host, port, true)
{
    epoll.set_on_socket_connect([&](vm::tcp_socket&& sock)
				{
				    int fd = sock.get_fd();
				    clients.insert(std::make_pair(fd,
								  T(std::move(sock))));
				    clients[fd].id = id_counter++;
				    //clients[fd].get_socket().add_flag(O_NONBLOCK);
				    // same as with first socket
				    epoll.add_socket(clients[fd].get_socket());
				    std::cout << "Added a client on socket with fd "
					      << fd << std::endl;
				    this->on_connect(clients, clients[fd]);
				});
    epoll.set_on_data_income([&](int fd)
			     {
				 this->on_data_income(clients, clients[fd]);
				 //else std::cerr << "Got data from socket server is not tracking"
				 //		<< std::endl;
			     });
    epoll.set_on_connection_closed([&](int fd)
				   {
				       disconnect_client(clients[fd]);
				   });

}


vm::tcp_server<T>::tcp_server()
    : tcp_server("localhost", "7273")
{}


void vm::tcp_server<T>::set_on_data_income(typename vm::typehelper<T>::event_h handler)
{
    on_data_income = handler;
}


void vm::tcp_server<T>::set_on_connect(typename vm::typehelper<T>::event_h handler)
{
    on_connect = handler;
}


void vm::tcp_server<T>::set_on_disconnect(typename vm::typehelper<T>::event_h handler)
{
    on_disconnect = handler;
}


void vm::tcp_server<T>::start() {
    running = true;
    while (running) {
	epoll.process_data_as_server();
    }
}


void vm::tcp_server<T>::stop() {
    running = false;
}


bool vm::tcp_server<T>::is_running() {
    return running;
}


void vm::tcp_server<T>::disconnect_client(T& client)
{
    on_disconnect(clients, client);
    clients.erase(client.get_socket().get_fd());
    std::cout << "Connection closed on fd " << client.get_socket().get_fd() << std::endl;
}
