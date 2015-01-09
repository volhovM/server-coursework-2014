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
    server.set_on_request([&server, &requestin](http_connection& connection, http_request request)
			  {
			      requestin = request;
			      vector<http_request> requests;
			      request.add_header("Connection", "close");
			      requests.push_back(request);
			      http_connection* to = new http_connection(request.get_headers()["Host"]);
			      to->query(requests,
					[requests, &server, &connection](std::vector<vm::http_response> res)
					{
					    for (http_response r: res)
					    {
						log_m("got response, headers..");
						log_d(r.commit_headers());
						log_m("now sending response...");
						connection.send_response(r);
					    }
					    //server.stop();
					},
					[to](int fd)
					{
					    delete to;
					},
					server.get_epoll()
				       );
			  });
    server.start();
    std::cout << "exiting" << std::endl;
    //    vector<http_request> requests;
    //    requestin.add_header("Connection", "close");
    //    std::cout << requestin.get_headers()["Connection"] << std::endl;
    //    requests.push_back(requestin);
    //    http_connection to(requestin.get_headers()["Host"]);
    //    to.query(requests,
    //	     [requests, &server](std::vector<vm::http_response> res)
    //	     {
    //		 for (http_response r: res)
    //		     std::cout << res.back().commit() << std::endl;
    //		 server.stop();
    //	     },
    //	     server.get_epoll()
    //	     );
    //    server.start();
}