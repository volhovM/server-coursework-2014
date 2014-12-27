#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <memory>
#include "http.h"

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


std::vector<vm::http_response> vm::http_connection::send_waiting(std::vector<vm::http_request> req)
{
    std::cout << "sending requests" << std::endl;
    auto addr = get_socket().get_address();
    epoll_wrapper epoll(addr.first, addr.second, false);
    std::vector<vm::http_response> responses;
    responses.push_back(http_response());
    bool done;
    epoll.set_on_data_income([&](int fd)
				{
				    std::string data = this->recieve_data();
				    std::cout << "got data: " << data << std::endl;
				    responses.back().append_data(data);
				    if (responses.back().is_complete())
				    {
					std::cout << "responce ok " << responses.size()
						  << " of " << req.size();
					if (responses.size() == req.size())
					    blocking = false;
					else responses.push_back(http_response());
				    } else
				    {
					std::cout << "request not over" << std::endl;
				    }
				});
    epoll.set_on_connection_closed([&](int fd)
				   {
				       std::cout << "connection closed" << std::endl;
				       done = true;
				   });
    for (http_request r: req) send_request(r);
    blocking = true;
    while (blocking)
    {
	epoll.process_data_as_client();
    }
    return responses;
}

void vm::http_connection::stop_waiting()
{
    blocking = false;
}

void vm::http_connection::send_request(http_request request)
{
    this->send_data(request.commit());
}

void vm::http_connection::send_response(http_response response)
{
    this->send_data(response.commit());
}
