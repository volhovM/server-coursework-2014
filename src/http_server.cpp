#include <vector>
#include <string>
#include <utility>
#include "http.h"

vm::http_server::http_server(std::string host, std::string port)
    : on_request([](vm::http_request) -> vm::http_response
		 {
		     vm::http_response response;
		     response.set_status_code(vm::http_response::STATUS_NOT_FOUND);
		     return response;
		 })
{
    //set callbacks
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

void vm::http_server::set_on_request(std::function<http_response(http_request)> foo)
{
    on_request = foo;
}
