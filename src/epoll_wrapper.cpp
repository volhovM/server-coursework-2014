#include <functional>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <stdexcept>
#include "io.h"

epoll_wrapper::epoll_wrapper()
    : serv("localhost", "7373", true)
    , connection_handler([](std::vector<tcp_socket>&){}){
    init();
}

epoll_wrapper::epoll_wrapper(std::string host, std::string port)
    : serv(host, port, true)
    , connection_handler([](std::vector<tcp_socket>&){}) {
    init();
}

epoll_wrapper::~epoll_wrapper() {
    close(efd);

}

void epoll_wrapper::init() {
//    event_handler = [&] (epoll_event& event) {
//	if (process_data(event.data.fd) != 0) {
//	    printf("Closed connection on descriptor %d\n",
//			       event.data.fd);
//	    /* Closing the descriptor will make epoll remove it
//	       from the set of descriptors which are monitored. */
//	    close(event.data.fd);
//	}
//    };
    efd = epoll_create1(0);
    if (efd == 0) {
	perror("epoll_create");
	abort();
    }
    serv.add_flag(O_NONBLOCK);
    serv.set_listening();
    epoll_event event;
    event.data.fd = serv.get_fd();
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, serv.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
    // TODO make local
}

void epoll_wrapper::on_event_income(event_h handler) {
    event_handler = handler;
}

void epoll_wrapper::on_connected(simple_h handler) {
    connection_handler = handler;
}

void epoll_wrapper::add_socket(tcp_socket& socket) {
    // epollin -- read, epollet -- edge triggered behaviour (wait returns
    // each time there`s data in fd left
    // ??? WHY DO WE NEED TO SET event.data.fd, WHEN WE'RE ALREADY PASSING IN sfd ???
    epoll_event event;
    event.data.fd = socket.get_fd();
    event.events = EPOLLIN | EPOLLET;
    std::cout << "adding " << socket.get_fd() << std::endl;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, socket.get_fd(), &event) == -1) {
	perror("epoll_ctl");
	abort();
    }
}

void epoll_wrapper::start() {
    running = true;
    epoll_event event;
    epoll_event *events = (epoll_event *) calloc(MAXEVENTS, sizeof event);
    while (running) {
	//	    std::cout << "before epoll_wait" << std::endl;
	int n = epoll_wait(efd, events, MAXEVENTS, -1);
	//	    std::cout << "epoll_wait succeeded" << std::endl;
	for (int i = 0; i < n; i++) {
	    // error, hang up or not read
	    if ((events[i].events & EPOLLERR) ||
		(events[i].events & EPOLLHUP) ||
		(!(events[i].events & EPOLLIN))) {
		fprintf(stderr, "epoll error\n");
		// closing file descriptor

		continue;
	    }
	    // this event seems ok, it's about our socket, let's connect it
	    else if (this->serv.get_fd() == events[i].data.fd) {
		for (; ;) {
		    struct sockaddr in_addr;
		    socklen_t in_len;
		    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
		    in_len = sizeof in_addr;
		    /* We're getting new socket s from sfd, in &in_addr we'll
		       get info about connected side, in_len before
		       call contains sizeof, after -- length of
		       adress in bytes. If socket non-blockable,
		       accept will not hang but return EAGAIN
		    */
		    //	std::cout <<"waiting for accept" << std::endl;
		    sockets.emplace_back(accept(serv.get_fd(), &in_addr, &in_len));
		    if (sockets.back().get_fd() == -1) {
			sockets.pop_back();
			std::cout << "vector size after accept: "
				  << sockets.size() << std::endl;
			// no connection? ok then
			// we've proceeded all incoming connections
			if ((errno == EAGAIN) ||
			    (errno == EWOULDBLOCK)) {
			    break;
			}
			//dat's strange
			else {
			    perror("accept");
			    break;
			}
		    }
		    /* so we got valid addr and len, let's get some data
		       we'll get machine name into hbuf, port into sbuf
		       MAGIC CONSTANTS -- return hbuf and sbuf in numeric form
		    */
		    if (getnameinfo(&in_addr, in_len,
				    hbuf, sizeof hbuf,
				    sbuf, sizeof sbuf,
				    NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
			printf("Accepted connection on descriptor %d "
			       "(host=%s, port=%s)\n", sockets.back().get_fd(), hbuf, sbuf);
		    }
		    /* Make the incoming socket non-blocking and add it to the
		       list of fds to monitor. */
		    sockets.back().add_flag(O_NONBLOCK);
		    // same as with first socket
		    add_socket(sockets.back());
		    connection_handler(sockets);
		    std::cout << "ADDED" << std::endl;
		}
		continue;
	    }
	    else {
		/* Let's read all the data available on event.fd
		   We also have edge-triggered mode, so we must
		   read all data as we won't get another notification
		*/
		event_handler(sockets, events[i]);
	    }
	}
    }
    free(events);
}

bool epoll_wrapper::is_running() {
    return running;
}