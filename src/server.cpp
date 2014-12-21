#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "io.h"
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

    server->on_event_income([&] (std::vector<vm::tcp_client>& clients, vm::tcp_client& client) {
	    std::string msg = client.recieve_data();
	    for (int i = 0; i < clients.size(); i++)
	    {
		if (clients[i] != client)
		{
		    clients[i].send_data("CLIENT #" + std::to_string(i + 1) + ": " + msg);
		}
	    }
	});
    server->start();
}
