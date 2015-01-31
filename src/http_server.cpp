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
    , on_disconnect([](vm::http_connection&){})
    , server(host, port)
{
    server.set_on_connect([this](std::map<int, vm::http_connection>& map,
                              http_connection& con)
                          {
                              request_map[con.get_fd()] = http_request();
                          });
    server.set_on_data_income([this](std::map<int, vm::http_connection>& clients, http_connection& con)
                              {
                                  vm::log_d("server: in on_data_income");
                                  std::string msg = con.recieve_data();
                                  vm::log_d("server: recieved data from "
                                            + std::to_string(con.get_fd()) + ": " +
                                            std::to_string(msg.length()) + " smbls");
                                  int fd = con.get_fd();
                                  http_request r = request_map[fd];
                                  r.append_data(msg);
                                  request_map[fd] = r;
                                  if (request_map[con.get_socket().get_fd()]
                                      .is_complete())
                                  {
                                      //vm::log_e("server: " +
                                      //          std::to_string(con.get_fd()) + "-->");
                                      vm::log_d("------------------INCOME------------------");
                                      vm::log_d(request_map[con.get_fd()].commit_headers());
                                      vm::log_d("+"
                                                + std::to_string(request_map[con.get_fd()]
                                                                 .get_body()
                                                                 .length()) + " body chars");
                                      vm::log_d("-----------------*INCOME*----------------");
                                      on_request(clients.at(con.get_fd()),
                                                 request_map[con.get_fd()]);
                                      request_map.erase(fd);
                                  }
                              });
    server.set_on_disconnect([this](std::map<int, vm::http_connection>&, http_connection& con)
                             {
                                 vm::log_d("server: calling auxiliary on_disconnect");
                                 on_disconnect(con);
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

void vm::http_server::disconnect_client(vm::http_connection& connection)
{
    server.disconnect_client(connection);
}

void vm::http_server::set_on_request(std::function<void(http_connection&, http_request)> foo)
{
    on_request = foo;
}

void vm::http_server::set_on_disconnect(std::function<void(http_connection&)> foo)
{
    on_disconnect = foo;
}

vm::epoll_wrapper& vm::http_server::get_epoll()
{
    return server.get_epoll();
}
