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


void vm::http_connection::query(std::vector<vm::http_request> req,
				std::function<void(std::vector<vm::http_response>)> response_handler,
				std::function<void(int)> disconnection_handler,
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
			     std::string data = recieve_data();
			     vm::log_d((responses == NULL) ?
					"responces are null" : "responses are not null");
			     responses->back().append_data(data);

			     if (responses->back().is_complete())
			     {
				 vm::log_d("responce ok " + std::to_string(responses->size())
					   + " of " + std::to_string(req.size()));
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
				 vm::log_d("request not over");
			     }
			 },
			     [disconnection_handler](int fd)
			     {
				 vm::log_w("Couldn't download from fd " + std::to_string(fd));
				 disconnection_handler(fd);
			     }
		     });
    vm::log_d("sending_requests");
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
