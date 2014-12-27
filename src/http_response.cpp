#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include "http.h"

void vm::http_response::append_data(std::string data)
{
    unparsed += data;
    parse();
}

bool vm::http_response::is_complete()
{
    return title_parsed && headers_parsed && body_parsed;
}

void vm::http_response::parse()
{
    //	std::cout << "parsing: " << "$$$" << unparsed << "$$$ "
    //	      << title_parsed << " " << headers_parsed << std::endl;
    while (true)
    {
	if (!title_parsed)
	{
	    http_version = unparsed.substr(0, unparsed.find(" "));
	    unparsed = unparsed.substr(unparsed.find(" ") + 1);

	    int code = std::stoi(unparsed.substr(0, unparsed.find(" ")));
	    unparsed = unparsed.substr(unparsed.find(" ") + 1);
	    std::string reason = unparsed.substr(0, unparsed.find("\r\n"));
	    unparsed = unparsed.substr(unparsed.find("\r\n") + 2);
	    //	    std::cout << "PARSER URL: " <<  url << std::endl;
	    status_code = http_status_code { code, reason };

	    title_parsed = true;
	} else if (!headers_parsed)
	{
	    int next = unparsed.find("\r\n");
	    //	    std::cout << "PARSER: GOT " << next << std::endl;
	    if (next == 0)
	    {
		headers_parsed = true;
		unparsed = unparsed.substr(2);
	    } else if (next == -1) {
		// partial data
		break;
	    } else
	    {
		std::string curr = unparsed.substr(0, next);
		unparsed = unparsed.substr(next + 2);
		std::string name = curr.substr(0, curr.find(":"));
		curr = curr.substr(curr.find(":") + 2); // +space
		add_header(name, curr);
	    }
	} else if (!body_parsed)
	{
	    //std::cout << "PARSING BODY " << unparsed << std::endl;
	    if (headers.find("Content-Length") != headers.end())
	    {
		int cnt = std::stoi(headers["Content-Length"]);
		body += unparsed;
		unparsed = "";
		if (cnt == body.length()) body_parsed = true;
	    } else body_parsed = true;
	    break;
	}
    }
    std::cout << "LEFT TO PARSE: " << "$$$" << unparsed << "$$$" << std::endl;
}

void vm::http_response::set_status_code(http_status_code new_code)
{
    status_code = new_code;
}

void vm::http_response::add_header(std::string name, std::string value)
{
    headers.insert(std::make_pair(name, value));
}

void vm::http_response::set_body(std::string new_body)
{
    body = new_body;
}

std::string vm::http_response::commit()
{
    std::string ret = "";
    ret += "HTTP/" + HTTP_VERSION + " " + std::to_string(status_code.code)
	+ " " + status_code.description + "\r\n";
    for (auto header: headers)
    {
	ret += header.first + ": " + header.second + "\r\n";
    }
    ret += "\r\n";
    ret += body;
    return ret;
}

vm::http_response::http_status_code vm::http_response::get_status_code()
{
    return status_code;
}

std::map<std::string, std::string> vm::http_response::get_headers()
{
    return headers;
}

std::string vm::http_response::get_body()
{
    return body;
}

const vm::http_response::http_status_code vm::http_response::STATUS_CONTINUE = { 100, "CONTINUE" };
const vm::http_response::http_status_code vm::http_response::STATUS_OK = { 200, "OK" };
const vm::http_response::http_status_code vm::http_response::STATUS_CREATED = { 201, "CREATED" };
const vm::http_response::http_status_code vm::http_response::STATUS_ACCEPTED = { 202, "ACCEPTED" };
const vm::http_response::http_status_code vm::http_response::STATUS_NON_AUTHORITATIVE_INFORMATION = { 203, "NON_AUTHORITATIVE_INFORMATION" };
const vm::http_response::http_status_code vm::http_response::STATUS_NO_CONTENT = { 204, "NO_CONTENT" };
const vm::http_response::http_status_code vm::http_response::STATUS_RESET_CONTENT = { 205, "RESET_CONTENT" };
const vm::http_response::http_status_code vm::http_response::STATUS_PARTIAL_CONTENT = { 206, "PARTIAL_CONTENT" };
const vm::http_response::http_status_code vm::http_response::STATUS_MULTIPLE_CHOICES = { 300, "MULTIPLE_CHOICES" };
const vm::http_response::http_status_code vm::http_response::STATUS_MOVED_PERMANENTLY = { 301, "MOVED_PERMANENTLY" };
const vm::http_response::http_status_code vm::http_response::STATUS_FOUND = { 302, "FOUND" };
const vm::http_response::http_status_code vm::http_response::STATUS_SEE_OTHER = { 303, "SEE_OTHER" };
const vm::http_response::http_status_code vm::http_response::STATUS_NOT_MODIFIED = { 304, "NOT_MODIFIED" };
const vm::http_response::http_status_code vm::http_response::STATUS_USE_PROXY = { 305, "USE_PROXY" };
const vm::http_response::http_status_code vm::http_response::STATUS_TEMPORARY_REDIRECT = { 307, "TEMPORARY_REDIRECT" };
const vm::http_response::http_status_code vm::http_response::STATUS_BAD_REQUEST = { 400, "BAD_REQUEST" };
const vm::http_response::http_status_code vm::http_response::STATUS_UNAUTHORIZED = { 401, "UNAUTHORIZED" };
const vm::http_response::http_status_code vm::http_response::STATUS_PAYMENT_REQUIRED = { 402, "PAYMENT_REQUIRED" };
const vm::http_response::http_status_code vm::http_response::STATUS_FORBIDDEN = { 403, "FORBIDDEN" };
const vm::http_response::http_status_code vm::http_response::STATUS_NOT_FOUND = { 404, "NOT_FOUND" };
const vm::http_response::http_status_code vm::http_response::STATUS_METHOD_NOT_ALLOWED = { 405, "METHOD_NOT_ALLOWED" };
const vm::http_response::http_status_code vm::http_response::STATUS_NOT_ACCEPTABLE = { 406, "NOT_ACCEPTABLE" };
const vm::http_response::http_status_code vm::http_response::STATUS_PROXY_AUTHENTICATION_REQUIRED = { 407, "PROXY_AUTHENTICATION_REQUIRED" };
const vm::http_response::http_status_code vm::http_response::STATUS_REQUEST_TIMEOUT = { 408, "REQUEST_TIMEOUT" };
const vm::http_response::http_status_code vm::http_response::STATUS_CONFLICT = { 409, "CONFLICT" };
const vm::http_response::http_status_code vm::http_response::STATUS_GONE = { 410, "GONE" };
const vm::http_response::http_status_code vm::http_response::STATUS_LENGTH_REQUIRED = { 411, "LENGTH_REQUIRED" };
const vm::http_response::http_status_code vm::http_response::STATUS_PRECONDITION_FAILED = { 412, "PRECONDITION_FAILED" };
const vm::http_response::http_status_code vm::http_response::STATUS_REQUEST_ENTITY_TOO_LARGE = { 413, "REQUEST_ENTITY_TOO_LARGE" };
const vm::http_response::http_status_code vm::http_response::STATUS_REQUEST_URI_TOO_LONG = { 414, "REQUEST_URI_TOO_LONG" };
const vm::http_response::http_status_code vm::http_response::STATUS_REQUEST_UNSUPPORTED_MEDIA_TYPE = { 415, "REQUEST_UNSUPPORTED_MEDIA_TYPE" };
const vm::http_response::http_status_code vm::http_response::STATUS_REQUESTED_RANGE_NOT_SATISFIABLE = { 416, "REQUESTED_RANGE_NOT_SATISFIABLE" };
const vm::http_response::http_status_code vm::http_response::STATUS_EXPECTATION_FAILED = { 417, "EXPECTATION_FAILED" };
const vm::http_response::http_status_code vm::http_response::STATUS_INTERNAL_SERVER_ERROR = { 500, "INTERNAL_SERVER_ERROR" };
const vm::http_response::http_status_code vm::http_response::STATUS_NOT_IMPLEMENTED = { 501, "NOT_IMPLEMENTED" };
const vm::http_response::http_status_code vm::http_response::STATUS_BAD_GATEWAY = { 502, "BAD_GATEWAY" };
const vm::http_response::http_status_code vm::http_response::STATUS_SERVICE_UNAVAILABLE = { 503, "SERVICE_UNAVAILABLE" };
const vm::http_response::http_status_code vm::http_response::STATUS_GATEWAY_TIMEOUT = { 504, "GATEWAY_TIMEOUT" };
const vm::http_response::http_status_code vm::http_response::STATUS_HTTP_VERSION_NOT_SUPPORTED = { 505, "HTTP_VERSION_NOT_SUPPORTED" };
