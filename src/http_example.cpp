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
#include "http.h"
using namespace vm;

int main(int argc, char *argv[]) {
    tcp_server* server;
    if (argc < 2) {
	fprintf(stderr, "Usage: %s [port]\n", argv[0]);
	return 0;
    } else if (argc == 2)
	server = new tcp_server("localhost", argv[1]);
    else
	server = new tcp_server(argv[1], argv[2]);
    server->set_on_data_income([] (std::map<int, vm::tcp_client>& clients,
				    vm::tcp_client& client)
			       {
				   std::cout << "got text:" << std::endl;
				   std::string msg = client.recieve_data();
				   //  std::string part1 = msg.substr(0, 90);
				   //  std::string part2 = msg.substr(90, 90);
				   //  std::string part3 = msg.substr(180, 140);
				   //  std::string part4 = msg.substr(320);
				   //
				   //  std::cout << "$$$" << msg << "$$$" << std::endl;
				   //  std::cout << "------------------------------------" << std::endl;
				   //  std::cout << part1 << std::endl << std::endl;
				   //  std::cout << part2 << std::endl << std::endl;;
				   //  std::cout << part3 << std::endl << std::endl;
				   //  std::cout << part4 << std::endl << std::endl;
				   //  std::cout << "------------------------------------" << std::endl;
				   //				   std::cout << std::endl;
				   //
				   http_request request;
				   request.append_data(msg);
				   //				   request.append_data(part1);
				   //				   request.append_data(part2);
				   //				   request.append_data(part3);
				   //				   request.append_data(part4);
				   std::cout << "$$$" << request.commit() << "$$$" << std::endl;
			       });
    server->start();
}
