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
#include <vector>

#include "logger.h"

namespace vm
{
    struct tcp_socket
    {
        // client type by default
        tcp_socket(std::string, std::string);
        tcp_socket(std::string, std::string, bool is_server);
        tcp_socket(int master_fd, sockaddr, socklen_t); //accepting here
        tcp_socket(tcp_socket&& that);
        ~tcp_socket();

        int get_fd() const;
        void invalidate();
        bool is_valid();
        void close_fd();
        void set_listening();
        void add_flag(int flag);
        void revert_flag(int flag);

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
        ~epoll_wrapper();

        static std::function<void(int)> on_socket_connect(std::function<void(tcp_socket&&)>);
        struct epoll_handler
        {
            std::function<void(int)> on_data_income;
            std::function<void(int)> on_disconnect;
        };

        void dump_status();
        void process_events();
        void add_socket(tcp_socket&, epoll_handler);
        void add_socket(tcp_socket&, epoll_handler, std::string tag);
        void remove_socket(int);

    private:
        std::map<int, epoll_handler> handlers;
        int efd;
        int MAXEVENTS = 64;
        std::vector<std::pair<int, std::string> > fd_status;
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
                , server_socket(host, port, true)
                , epoll()
            {
                epoll.add_socket(server_socket,
                                 epoll_wrapper::epoll_handler {
                                     [this](int server_fd) {
                                         sockaddr in_addr;
                                         socklen_t in_len;
                                         char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
                                         in_len = sizeof in_addr;
                                         tcp_socket temp(server_fd, in_addr, in_len);
                                         if (!temp.is_valid())
                                             {
                                                 if ((errno == EAGAIN) ||
                                                     (errno == EWOULDBLOCK))
                                                     {
                                                         vm::log_e("accept failed");
                                                     }
                                                 else
                                                     {
                                                         perror("accept");
                                                         vm::log_e("accept_failed");
                                                     }
                                             }
                                         int fd = -1;
                                         for (auto iter = clients.begin();
                                              iter != clients.end(); iter++)
                                             {
                                                 // TODO and check ports
                                                 if (iter->second.get_socket().get_address().first ==
                                                     temp.get_address().first)
                                                     {
                                                         fd = iter->first;
                                                         break;
                                                     }
                                             }
                                         // add socket if not present
                                         //if (fd == -1)
                                         if (true)
                                             {
                                                 fd = temp.get_fd();
                                                 clients.emplace(fd, std::move(temp));
                                                 vm::log_d("server: accepted client #" +
                                                           std::to_string(clients[fd].get_fd()));
                                                 clients[fd].id = id_counter++;
                                                 clients[fd].get_socket().add_flag(O_NONBLOCK);
                                                 epoll.add_socket(clients[fd].get_socket(),
                                                                  epoll_wrapper::epoll_handler
                                                                  {
                                                                      ([this](int fd)
                                                                       {
                                                                           this->on_data_income(clients, clients[fd]);
                                                                       }),
                                                                          [this](int fd)
                                                                              {
                                                                                  on_disconnect(clients, clients[fd]);
                                                                                  this->disconnect_client(clients[fd]);
                                                                              }
                                                                  },
                                                                  clients[fd].get_socket().get_address().first + "->server");
                                                 vm::log_m("Added a client on socket with fd "
                                                           + std::to_string(fd));
                                                 this->on_connect(clients, clients[fd]);
                                             } else
                                             {
                                                 vm::log_d("server: client with fd#" +
                                                           std::to_string(fd) +
                                                           " already exists, not adding");
                                             }
                                     },
                                         [](int fd){}
                                 },
                                 "server socket");
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
                    epoll.process_events();
                }
                clients.clear();
            }

            void stop() {
                running = false;
                epoll.remove_socket(server_socket.get_fd());
            }

            bool is_running() {
                return running;
            }

            void disconnect_client(T& client)
            {
                int fd = client.get_fd();
                epoll.remove_socket(fd);
                clients.erase(client.get_socket().get_fd());
                vm::log_m("serven: connection closed on fd " + std::to_string(fd));
            }

            epoll_wrapper& get_epoll()
            {
                return epoll;
            }

        private:
            std::map<int, T> clients;
            epoll_wrapper epoll;
            typename typehelper<T>::event_h on_data_income;
            typename typehelper<T>::event_h on_connect;
            typename typehelper<T>::event_h on_disconnect;
            tcp_socket server_socket;
            bool running;
            int id_counter;
        };
};

#endif /* TCP_H */
