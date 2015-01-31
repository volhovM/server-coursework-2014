#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <memory>
#include "http.h"
#include "logger.h"

vm::http_connection::http_connection()
    : tcp_connection()
{}

vm::http_connection::http_connection(std::string host)
    : tcp_connection(host, "http")
{}

vm::http_connection::http_connection(tcp_socket&& that)
    : tcp_connection(std::move(that))
{}

vm::http_connection::http_connection(tcp_connection&& con)
    : tcp_connection(std::move(con))
{}

vm::http_connection::http_connection(http_connection&& that)
    : tcp_connection(std::move(that.socket))
{}

void vm::http_connection::query(vm::http_request req,
                                std::function<void(vm::http_response)> response_handler,
                                std::function<void(int)> disconnection_handler,
                                epoll_wrapper& epoll)
{
    if (!is_alive)
    {
        std::unique_ptr<vm::http_response>* response = new std::unique_ptr<vm::http_response>();
        response->reset(new vm::http_response);
        //        std::vector<vm::http_response>* responses = new std::vector<vm::http_response>();
        epoll.add_socket(get_socket(),
                         vm::epoll_wrapper::epoll_handler
                         {
                             // FIXME capturing & ?
                             [&epoll, response, req, response_handler, this](int fd)
                             {
                                 std::string data = recieve_data();
                                 vm::log_d("http_connection: got data from bridge connection: " + data);

                                 // may fail because of invalid data
                                 try {
                                     (*response)->append_data(data);

                                     if ((*response)->is_complete())
                                     {
                                         vm::log_ex("callback <---" + std::to_string(this->get_fd()));
                                         vm::log_d("response complete");
                                         //epoll.remove_socket(get_fd());
                                         response_handler(**response);
                                         // FIXME may leak here, not deleting old one
                                         response->reset(new vm::http_response());
                                     } else
                                     {
                                         vm::log_d("http_connection: request not over");
                                     }
                                 } catch (std::exception parse_exception)
                                 {
                                     vm::log_e("http_connection: parsing exception!!!: " +
                                               std::string(parse_exception.what()));
                                     response->reset(new vm::http_response());
                                 }
                             },
                             [disconnection_handler, response, this](int fd)
                             {
                                 is_alive = false;
                                 vm::log_w("http_connection: couldn't download from fd "
                                           + std::to_string(fd));
                                 disconnection_handler(fd);
                                 delete response;
                             }
                         },
                         "server->" + get_socket().get_address().first);
        is_alive = true;
    } else
    {
        vm::log_d("http_connection #" + std::to_string(get_fd()) + " still alive");
    }
    vm::log_d("sending_request");
    this->send_request(req);
}

void vm::http_connection::send_request(http_request request)
{
    this->send_data(request.commit());
}

void vm::http_connection::send_response(http_response response)
{
    vm::log_d("http_connection: writing response, body: " +
              std::to_string(response.get_body().length()));
    this->send_data(response.commit());
}
