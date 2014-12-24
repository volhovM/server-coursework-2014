#include <map>
#include <string>
#include <utility>
#include "tcp.h"

namespace vm
{
    const std::string HTTP_VERSION = "1.1";
    struct http_response
    {
	struct http_status_code
	{
	    int code;
	    std::string description;
	};

	static const http_status_code STATUS_CONTINUE;
	static const http_status_code STATUS_OK;
	static const http_status_code STATUS_CREATED;
	static const http_status_code STATUS_ACCEPTED;
	static const http_status_code STATUS_NON_AUTHORITATIVE_INFORMATION;
	static const http_status_code STATUS_NO_CONTENT;
	static const http_status_code STATUS_RESET_CONTENT;
	static const http_status_code STATUS_PARTIAL_CONTENT;
	static const http_status_code STATUS_MULTIPLE_CHOICES;
	static const http_status_code STATUS_MOVED_PERMANENTLY;
	static const http_status_code STATUS_FOUND;
	static const http_status_code STATUS_SEE_OTHER;
	static const http_status_code STATUS_NOT_MODIFIED;
	static const http_status_code STATUS_USE_PROXY;
	static const http_status_code STATUS_TEMPORARY_REDIRECT;
	static const http_status_code STATUS_BAD_REQUEST;
	static const http_status_code STATUS_UNAUTHORIZED;
	static const http_status_code STATUS_PAYMENT_REQUIRED;
	static const http_status_code STATUS_FORBIDDEN;
	static const http_status_code STATUS_NOT_FOUND;
	static const http_status_code STATUS_METHOD_NOT_ALLOWED;
	static const http_status_code STATUS_NOT_ACCEPTABLE;
	static const http_status_code STATUS_PROXY_AUTHENTICATION_REQUIRED;
	static const http_status_code STATUS_REQUEST_TIMEOUT;
	static const http_status_code STATUS_CONFLICT;
	static const http_status_code STATUS_GONE;
	static const http_status_code STATUS_LENGTH_REQUIRED;
	static const http_status_code STATUS_PRECONDITION_FAILED;
	static const http_status_code STATUS_REQUEST_ENTITY_TOO_LARGE;
	static const http_status_code STATUS_REQUEST_URI_TOO_LONG;
	static const http_status_code STATUS_REQUEST_UNSUPPORTED_MEDIA_TYPE;
	static const http_status_code STATUS_REQUESTED_RANGE_NOT_SATISFIABLE;
	static const http_status_code STATUS_EXPECTATION_FAILED;
	static const http_status_code STATUS_INTERNAL_SERVER_ERROR;
	static const http_status_code STATUS_NOT_IMPLEMENTED;
	static const http_status_code STATUS_BAD_GATEWAY;
	static const http_status_code STATUS_SERVICE_UNAVAILABLE;
	static const http_status_code STATUS_GATEWAY_TIMEOUT;
	static const http_status_code STATUS_HTTP_VERSION_NOT_SUPPORTED;

	void append_data(std::string);
	bool is_complete();

	void set_status_code(http_status_code);
	void add_header(std::string name, std::string value);
	void set_body(std::string);

	http_status_code get_status_code();
	std::map<std::string, std::string> get_headers();
	std::string get_body();

	std::string commit();

    private:
	http_status_code status_code;
	std::string unparsed;
	std::map<std::string, std::string> headers;
	std::string body;
    };

    struct http_request
    {
	struct http_request_method
	{
	    std::string description;
	};

	static const http_request_method GET;
	static const http_request_method POST;
	static const http_request_method HEAD;
	static const http_request_method PUT;
	static const http_request_method DELETE;
	static const http_request_method TRACE;
	static const http_request_method CONNECT;

	void append_data(std::string);
	bool is_complete();

	void set_request_method(http_request_method);
	void set_url(std::string);
	void add_header(std::string name, std::string value);

	std::map<std::string, std::string> get_headers();
	std::string get_url();
	http_request_method get_request_method();

	std::string commit();

    private:
	http_request_method request_method;
	std::string http_version;
	std::string url;
	std::string body;
	std::map<std::string, std::string> headers;

	void parse();
	std::string unparsed;
	bool title_parsed = false;
	bool headers_parsed = false;
	bool body_parsed = false;
    };

    struct http_server
    {
	http_server();
	http_server(std::string host, std::string port);

	void start();
	void stop();
	bool is_running();

	void set_on_request(std::function<http_response(http_request)>);

    private:
	tcp_server server;
	std::function<http_response(http_request)> on_request;
    };

    struct http_client
    {
    };
}
