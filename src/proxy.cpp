#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <cstdio>
#include <map>
#include "http.h"
#include "logger.h"

using namespace vm;
using namespace std;

int main(int argc, char *argv[]) {
    http_server server;
    http_request requestin;
    map<int, pair<string, http_connection> > con_map; //host, <parent_fd, new connection>
    map<http_request, http_response> proxy_cache;
    server.set_on_request([&server, &requestin, &con_map, &proxy_cache](http_connection& connection,
                                                                        http_request request)
                          {
                              vm::log_d("proxy: in data_income handler, got request from " +
                                        std::to_string(connection.get_fd()));
                              requestin = request;
                              // FIXME leaks here
                              string host = request.get_headers()["Host"];
                              vm::log_d(host);
                              for (auto i = con_map.begin(); i != con_map.end(); i++)
                              {
                                  if (i->first == connection.get_fd())
                                  {
                                      if (i->second.first != host)
                                      {
                                          log_d("proxy: emplacing connection");
                                          server.get_epoll()
                                              .remove_socket(con_map[connection.get_fd()].second.get_fd());
                                          con_map.erase(connection.get_fd());
                                          con_map.emplace(connection.get_fd(), make_pair(host, host));
                                          break;
                                      } else break;
                                  }
                              }
                              if (con_map.find(connection.get_fd()) == con_map.end())
                              {
                                  log_d("proxy: adding new bridge connection");
                                  con_map.emplace(connection.get_fd(), make_pair(host, host));
                              }
                              vm::log_ex("proxy: " + std::to_string(connection.get_fd()) + "--> "
                                        + std::to_string(con_map[connection.get_fd()]
                                                         .second.get_fd()) + "--->");
                              con_map[connection.get_fd()].second
                                  .query(request,
                                         [request, &server, &connection, &con_map, host]
                                         (vm::http_response res)
                                         {
                                             log_m("proxy: got response");
                                             log_d("-----------------OUTCOME-----------------");
                                             log_d(res.commit_headers());
                                             log_d("+" + std::to_string(res.get_body().length())
                                                   + " body chars");
                                             log_d("----------------*OUTCOME*----------------");
                                             log_m("proxy: now sending response to " +
                                                   std::to_string(connection.get_fd()));
                                             log_e("callback: <--" +
                                                   std::to_string(connection.get_fd()));
                                             connection.send_response(res);
                                         },
                                         [&server, &connection, &con_map](int fd)
                                         {
                                             server.disconnect_client(connection);
                                             vm::log_d("proxy: closing bridge socket");
                                             server
                                                 .get_epoll()
                                                 .remove_socket(
                                                                con_map[connection.get_fd()]
                                                                .second.get_fd());
                                             con_map.erase(connection.get_fd());
                                         },
                                         server.get_epoll());
                          });
    server.set_on_disconnect([&server, &con_map](http_connection& closing)
                             {
                                 if (con_map.find(closing.get_fd()) != con_map.end())
                                 {
                                     int child_fd = con_map[closing.get_fd()].second.get_fd();
                                     vm::log_d("proxy: parent " +
                                               std::to_string(closing.get_fd()) +
                                               " closing, so closing child " +
                                               std::to_string(child_fd));
                                     server.get_epoll().remove_socket(child_fd);
                                     con_map.erase(closing.get_fd());
                                 }
                             });
    server.start();
    std::cout << "proxy: exiting" << std::endl;
    //    vector<http_request> requests;
    //    requestin.add_header("Connection", "close");
    //    std::cout << requestin.get_headers()["Connection"] << std::endl;
    //    requests.push_back(requestin);
    //    http_connection to(requestin.get_headers()["Host"]);
    //    to.query(requests,
    //       [requests, &server](std::vector<vm::http_response> res)
    //       {
    //           for (http_response r: res)
    //               std::cout << res.back().commit() << std::endl;
    //           server.stop();
    //       },
    //       server.get_epoll()
    //       );
    //    server.start();
}
