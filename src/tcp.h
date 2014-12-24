#ifndef TCP_H
#define TCP_H

#include <sys/epoll.h>
#include <string>
#include <memory>
#include <functional>
#include <map>

namespace vm {
    struct tcp_socket {
	tcp_socket(std::string, std::string);
	tcp_socket(std::string, std::string, bool is_server);
	tcp_socket(int fd);
	tcp_socket(tcp_socket&& that);
	~tcp_socket();

	int get_fd() const;
	void invalidate();
	void close_fd();
	void set_listening();
	void init(std::string hostname, std::string port);
	void add_flag(int flag);

	void send(const std::string);
	std::string recieve();

    private:
	bool is_server;
	int sfd;

	tcp_socket(const tcp_socket&);
	tcp_socket& operator=(const tcp_socket&);
    };

    struct epoll_wrapper {
	epoll_wrapper();
	epoll_wrapper(std::string host, std::string port);
	~epoll_wrapper();

	void set_on_socket_connect(std::function<void(int fd)>);
	void set_on_data_income(std::function<void(int fd)>);
	void set_on_connection_closed(std::function<void(int fd)>);

	void process_data();
	void add_socket(tcp_socket&);
    private:
	void init();

	int efd;
	tcp_socket socket_input;
	int MAXEVENTS = 64;
	std::function<void(int fd)> on_socket_connect;
	std::function<void(int fd)> on_connection_closed;
	std::function<void(int fd)> on_data_income;
    };

    struct tcp_client {
	tcp_client();
	tcp_client(int);
	tcp_client(std::string host, std::string port);
	tcp_client(tcp_client&&);

	void connect_to(std::string host, std::string port);
	std::string recieve_data();
	void send_data(std::string);
	void disconnect();

	bool operator==(const tcp_client& that);
	bool operator!=(const tcp_client& that);

	tcp_socket& get_socket();

	int id;
    private:
	tcp_client(const tcp_client&);
	tcp_client& operator=(const tcp_client&);
	tcp_socket socket;
    };

    typedef std::function<void(std::map<int, tcp_client>&, tcp_client&)> event_h;

    struct tcp_server {
	tcp_server();
	tcp_server(std::string host, std::string port);

	void set_on_data_income(event_h);
	void set_on_connect(event_h);
	void set_on_disconnect(event_h);
	void add_socket(tcp_socket&);
	void stop();
	void start();
	bool is_running();
	void disconnect_client(tcp_client&);

    private:
	void init();

	event_h on_data_income;
	event_h on_connect;
	event_h on_disconnect;
	std::map<int, tcp_client> clients;
	epoll_wrapper epoll;
	bool running;
	int id_counter;
    };
};

#endif /* TCP_H */
