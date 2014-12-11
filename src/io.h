#include <sys/epoll.h>
#include <string>
#include <functional>
#include <vector>


struct tcp_socket {
    tcp_socket(std::string, std::string);
    tcp_socket(std::string, std::string, bool isServer);
    tcp_socket(int fd);
    tcp_socket(tcp_socket&& that);
    ~tcp_socket();

    int get_fd();
    void set_listening();
    void init(std::string hostname, std::string port);
    void add_flag(int flag);

    void send(const char*, int length);
    std::string get();

private:
    bool is_server;
    int sfd;
    tcp_socket(const tcp_socket&);
    tcp_socket& operator=(const tcp_socket&);
};

typedef std::function<void(std::vector<tcp_socket>&, epoll_event& event)> event_h;
typedef std::function<void(std::vector<tcp_socket>&)> simple_h;

struct epoll_wrapper {
    epoll_wrapper();
    epoll_wrapper(std::string host, std::string port);
    ~epoll_wrapper();


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

    void init();
    tcp_socket serv;
    bool running;
    int efd;
    int MAXEVENTS = 64;
};