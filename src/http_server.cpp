#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <memory>
#include "http.h"

vm::http_server::http_server(std::string host, std::string port)
    : on_request([](vm::http_connection& client, vm::http_request request) -> void
		 {
		     vm::http_response response;
		     response.set_status_code(vm::http_response::STATUS_NOT_FOUND);
		     client.send_response(response);
		 })
    , server(host, port)
{
    server.set_on_connect([&](std::map<int, vm::http_connection>& map, http_connection& con)
			  {
			      request_map.insert(std::make_pair(con.get_fd(), http_request()));
			  });
    server.set_on_data_income([&](std::map<int, vm::http_connection>& clients, http_connection& con)
			      {
				  std::string msg = con.recieve_data();
				  std::cout << msg << std::endl;
				  int fd = con.get_fd();
				  http_request r = request_map[fd];
				  r.append_data(msg);
				  std::cout << r.is_complete() << std::endl;
				  request_map[fd] = r;
				  if (request_map[con.get_socket().get_fd()]
				      .is_complete())
				  {
				      on_request(clients.at(con.get_fd()),
						 request_map[con.get_fd()]);
				      request_map.erase(fd);
				  }
			      });
    server.set_on_disconnect([&](std::map<int, vm::http_connection>&, http_connection& con)
			     {
				 request_map.erase(con.get_socket().get_fd());
			     });
}

vm::http_server::http_server()
    : http_server("localhost", "7273")
{}

void vm::http_server::start()
{
    server.start();
}

void vm::http_server::stop()
{
    server.stop();
}

bool vm::http_server::is_running()
{
    return server.is_running();
}

void vm::http_server::set_on_request(std::function<void(http_connection&, http_request)> foo)
{
    on_request = foo;
}
