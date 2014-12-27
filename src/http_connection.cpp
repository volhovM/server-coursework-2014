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


void vm::http_connection::query(std::vector<vm::http_request> req,
				std::function<void(std::vector<vm::http_response>)> response_handler,
				epoll_wrapper& epoll)
{
    std::vector<vm::http_response>* responses = new std::vector<vm::http_response>();
    responses->push_back(http_response());
    epoll.add_socket(get_socket(),
		     vm::epoll_wrapper::epoll_handler
		     {
			 // FIXME capturing & ?
			 [&epoll, responses, req, response_handler, this](int fd)
			 {
			     std::cout << "In http_connection query start handler" << std::endl;
			     std::string data = recieve_data();
			     std::cout << "got data: " << data << std::endl;
			     std::cout << (responses == NULL) << std::endl;
			     responses->back().append_data(data);
			     std::cout << "----MARK----" << std::endl;

			     if (responses->back().is_complete())
			     {
				 std::cout << "responce ok " << responses->size()
					   << " of " << req.size();
				 if (responses->size() == req.size())
				 {
				     epoll.remove_socket(get_fd());
				     response_handler(*responses);
				     delete responses;
				 }
				 else
				     responses->push_back(http_response());
			     } else
			     {
				 std::cout << "request not over" << std::endl;
			     }
			 },
			     [&](int fd)
			     {
				 std::cout << "Couldn't download from id " << id << std::endl;
			     }
		     });
    std::cout << "sending requests" << std::endl;
    for (http_request r: req) this->send_request(r);
}

void vm::http_connection::send_request(http_request request)
{
    this->send_data(request.commit());
}

void vm::http_connection::send_response(http_response response)
{
    this->send_data(response.commit());
}
