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
#include "tcp.h"
using namespace vm;

void init1(tcp_server<tcp_connection>* server)
{
    server->set_on_data_income([] (std::map<int, vm::tcp_connection>& clients,
                                    vm::tcp_connection& client)
                               {
                                   std::cout << "recieved data read request from fd: "
                                             << client.get_socket().get_fd() << std::endl;
                                   std::string msg = client.recieve_data();
                                   for (std::map<int, vm::tcp_connection>::iterator i = clients.begin();
                                        i != clients.end(); i++)
                                   {
                                       if (i->second != client && msg != "")
                                       {
                                           i->second.send_data("CLIENT #" +
                                                               std::to_string(client.id)
                                                               + ": " + msg);
                                       }
                                   }
                               });
    server->set_on_connect([] (std::map<int, vm::tcp_connection>& clients,
                                vm::tcp_connection& client)
                           {
                               for (std::map<int, vm::tcp_connection>::iterator i = clients.begin();
                                    i != clients.end(); i++)
                               {
                                   if (i->second != client)
                                   {
                                       i->second.send_data("CLIENT #" +
                                                           std::to_string(client.id)
                                                           + " CONNECTED\n");
                                   }
                               }
                           });
    server->set_on_disconnect([] (std::map<int, vm::tcp_connection>& clients,
                                   vm::tcp_connection& client)
                              {
                                  for (std::map<int, vm::tcp_connection>::iterator i = clients.begin();
                                       i != clients.end(); i++)
                                  {
                                      if (i->second != client)
                                      {
                                          i->second.send_data("CLIENT #" +
                                                              std::to_string(client.id)
                                                              + " DISCONNECTED\n");
                                      }
                                  }
                              });

}

void init2(tcp_server<tcp_connection>* server)
{
        server->set_on_connect([server] (std::map<int, vm::tcp_connection>& clients,
                                vm::tcp_connection& client)
                           {
                               std::string msg = "HTTP/1.1 200 OK\nContent-Type: text/plain; charset=utf-8\nContent-Length: 5\n\n12345";
                               client.send_data(msg);
                               server->disconnect_client(client);
                           });
}


int main2(int argc, char *argv[]) {
    tcp_server<tcp_connection>* server;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        return 0;
    } else if (argc == 2)
        server = new tcp_server<tcp_connection>("localhost", argv[1]);
    else
        server = new tcp_server<tcp_connection>(argv[1], argv[2]);

    init1(server);
    //init2(server);

    server->start();
}
