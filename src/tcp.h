#ifndef TCP_H
#define TCP_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <type_traits>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <memory>
#include <functional>
#include <map>

namespace vm
{
    struct tcp_socket
    {
	// client type by default
	tcp_socket(std::string, std::string);
	tcp_socket(std::string, std::string, bool is_server);
	tcp_socket(int, sockaddr, socklen_t);
	tcp_socket(tcp_socket&& that);
	~tcp_socket();

	int get_fd() const;
	void invalidate();
	bool is_valid();
	void close_fd();
	void set_listening();
	void init(std::string hostname, std::string port);
	void add_flag(int flag);

	void send(const std::string);
	std::string recieve();

	std::pair<std::string, std::string> get_address();
	bool is_server_type();
    private:
	bool is_server;
	int sfd;

	tcp_socket(const tcp_socket&);
	tcp_socket& operator=(const tcp_socket&);
    };

    struct epoll_wrapper
    {
	epoll_wrapper();
	epoll_wrapper(std::string host, std::string port, bool is_server);
	~epoll_wrapper();

	void set_on_socket_connect(std::function<void(tcp_socket&&)>);
	void set_on_data_income(std::function<void(int fd)>);
	void set_on_connection_closed(std::function<void(int fd)>);

	void process_data_as_server();
	void process_data_as_client();
	void add_socket(tcp_socket&);
    private:
	int efd;
	tcp_socket socket_input;
	int MAXEVENTS = 64;
	std::function<void(tcp_socket&&)> on_socket_connect;
	std::function<void(int fd)> on_connection_closed;
	std::function<void(int fd)> on_data_income;
    };

    struct tcp_connection
    {
	tcp_connection();
	tcp_connection(tcp_socket&&);
	tcp_connection(std::string host, std::string port);
	tcp_connection(tcp_connection&&);

	void connect_to(std::string host, std::string port);
	std::string recieve_data();
	void send_data(std::string);
	void disconnect();

	bool operator!=(const tcp_connection& that);

	tcp_socket& get_socket();
	int get_fd();

	int id;

    protected:
	tcp_socket socket;

    private:
	tcp_connection(const tcp_connection&);
	tcp_connection& operator=(const tcp_connection&);
    };

    template<typename T>
     struct typehelper
     {
	 typedef std::function<void(std::map<int, T>&, T&)> event_h;
     };

    template<typename T>
    struct tcp_server
	{
	tcp_server(std::string host, std::string port)
	    : on_data_income([](std::map<int, T>&, T&){})
		, on_connect([](std::map<int, T>&, T&){})
		, on_disconnect([](std::map<int, T>&, T&){})
		, epoll(host, port, true)
		{
		    epoll.set_on_socket_connect([&](tcp_socket&& sock)
						{
						    int fd = sock.get_fd();
						    clients.insert(std::make_pair(fd,
										  T(std::move(sock))));
						    clients[fd].id = id_counter++;
						    clients[fd].get_socket().add_flag(O_NONBLOCK);
						    epoll.add_socket(clients[fd].get_socket());
						    std::cout << "Added a client on socket with fd "
							      << fd << std::endl;
						    this->on_connect(clients, clients[fd]);
						});
		    epoll.set_on_data_income([&](int fd)
					     {
						 this->on_data_income(clients, clients[fd]);
					     });
		    epoll.set_on_connection_closed([&](int fd)
						   {
						       disconnect_client(clients[fd]);
						   });
		}

	tcp_server()
	    : tcp_server("localhost", "7273")
		{}

	    void set_on_data_income(typename vm::typehelper<T>::event_h handler)
	    {
		on_data_income = handler;
	    }

	    void set_on_connect(typename vm::typehelper<T>::event_h handler)
	    {
		on_connect = handler;
	    }

	    void set_on_disconnect(typename vm::typehelper<T>::event_h handler)
	    {
		on_disconnect = handler;
	    }

	    void start() {
		running = true;
		while (running) {
		    epoll.process_data_as_server();
		}
	    }

	    void stop() {
		running = false;
	    }

	    bool is_running() {
		return running;
	    }

	    void disconnect_client(T& client)
	    {
		int fd = client.get_fd();
		on_disconnect(clients, client);
		clients.erase(client.get_socket().get_fd());
		std::cout << "Connection closed on fd " << fd << std::endl;
	    }

	protected:
	    std::map<int, T> clients;
	private:
	    typename typehelper<T>::event_h on_data_income;
	    typename typehelper<T>::event_h on_connect;
	    typename typehelper<T>::event_h on_disconnect;
	    epoll_wrapper epoll;
	    bool running;
	    int id_counter;
	};
};

#endif /* TCP_H */
