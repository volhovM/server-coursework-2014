#include <sys/epoll.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace vm {
    struct tcp_socket {
	tcp_socket(std::string, std::string);
	tcp_socket(std::string, std::string, bool is_server);
	tcp_socket(int fd);
	tcp_socket(tcp_socket&& that);
	~tcp_socket();

	int get_fd();
	void set_listening();
	void init(std::string hostname, std::string port);
	void add_flag(int flag);

	void send(const std::string);
	std::string get();

    private:
	bool is_server;
	int sfd;

	tcp_socket(const tcp_socket&);
	tcp_socket& operator=(const tcp_socket&);
    };

    class epoll_wrapper {
    public:
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

    typedef std::function<void(std::vector<tcp_socket>&, tcp_socket&)> event_h;
    typedef std::function<void(std::vector<tcp_socket>&)> simple_h;

    class tcp_server {
    public:
	tcp_server();
	tcp_server(std::string host, std::string port);
	~tcp_server();

	void on_event_income(event_h);
	void on_connected(simple_h);
	void add_socket(tcp_socket&);
	void stop();
	void start();
	bool is_running();

    private:
	event_h event_handler;
	simple_h connection_handler;
	std::vector<tcp_socket> sockets;
	epoll_wrapper epoll;
	void init();
	bool running;
    };
};
