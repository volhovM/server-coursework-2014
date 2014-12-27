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
using namespace vm;
using namespace std;

int main(int argc, char *argv[]) {
    http_server server;
    server.set_on_request([](http_connection& connection, http_request request)
			  {
			      vector<http_request> requests;
			      requests.push_back(request);
			      http_connection to(request.get_headers()["Host"]);
			      connection.send_response(to.send_waiting(requests).back());
			  });
    server.start();
}
