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

	int get_fd() const;
	void invalidate();
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

    class tcp_client {
    public:
	tcp_client();
	tcp_client(int);
	tcp_client(std::string host, std::string port);
	tcp_client(tcp_client&&);

	void connect_to(std::string host, std::string port);
	std::string recieve_data();
	void send_data(std::string);

	bool operator==(const tcp_client& that);
	bool operator!=(const tcp_client& that);

	tcp_socket& get_socket();
    private:
	tcp_client(const tcp_client&);
	tcp_client& operator=(const tcp_client&);
	tcp_socket socket;
    };

    typedef std::function<void(std::vector<tcp_client>&, tcp_client&)> event_h;
    typedef std::function<void(std::vector<tcp_client>&)> simple_h;

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
	void init();

	event_h event_handler;
	simple_h connection_handler;
	std::vector<tcp_client> sockets;
	epoll_wrapper epoll;
	bool running;
    };
};
