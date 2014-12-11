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

int process_data(std::vector<tcp_socket>& sockets, int fd) {
    std::cout << "processing input" << std::endl;
    ssize_t count;

    int len = 0;
    ioctl(fd, FIONREAD, &len);
    std::cout << "LEN IS:" << len << std::endl;
    char buf[len];
    count = read(fd, buf, 500);
    if (count == -1) {
	// if errno == EAGAIN, we have read all data
	if (errno != EAGAIN) {
	    perror("read");
	    return 1;
	}
    }
    else if (count == 0) {
	//eof
	return 1;
    }


    std::cout << "COUNT:" << count << std::endl;
    std::cout << "SIZEOF BUF:" << sizeof buf << std::endl;
    std::cout << "STRLEN BUF:" << strlen(buf) << std::endl;
    std::cout << "BUF IS:" << buf << std::endl;
    if (sizeof buf < 3) return 0;
    for (int j = 0; j < sockets.size(); j++) {
	if (sockets[j].get_fd() != fd){
	    std::string s = "client #" + std::to_string(fd) + ": ";
	    sockets[j].send(const_cast<char*>(s.c_str()), s.length() + 1);
	    sockets[j].send(buf, count);
	}
    }

    return 0;
}

int main(int argc, char *argv[]) {
    epoll_wrapper* server;
    if (argc < 2) {
	fprintf(stderr, "Usage: %s [port]\n", argv[0]);
	return 0;
    } else if (argc == 2)
	 server = new epoll_wrapper("localhost", argv[1]);
    else
	server = new epoll_wrapper(argv[1], argv[2]);

    server->on_event_income([&] (std::vector<tcp_socket>& vec, epoll_event& event) {
	    if (process_data(vec, event.data.fd) != 0) {
		printf("Closed connection on descriptor %d\n",
		       event.data.fd);
		/* Closing the descriptor will make epoll remove it
		   from the set of descriptors which are monitored. */
		close(event.data.fd);
	    }
	});
    server->start();
}
