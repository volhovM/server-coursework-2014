#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include "logger.h"
#include "http.h"

void vm::http_request::append_data(std::string data)
{
    unparsed += data;
    parse();
}

bool vm::http_request::is_complete()
{
    return title_parsed && headers_parsed && body_parsed;
}

void vm::http_request::parse()
{
    //	std::cout << "parsing: " << "$$$" << unparsed << "$$$ "
    //	      << title_parsed << " " << headers_parsed << std::endl;
    while (true)
    {
	if (!title_parsed)
	{
	    request_method = http_request_method { unparsed.substr(0, unparsed.find(" ")) };
	    unparsed = unparsed.substr(unparsed.find(" ") + 1);
	    url = unparsed.substr(0, unparsed.find(" "));
	    //	    std::cout << "PARSER URL: " <<  url << std::endl;
	    unparsed = unparsed.substr(unparsed.find(" ") + 1);
	    http_version = unparsed.substr(0, unparsed.find("\r\n"));
	    unparsed = unparsed.substr(unparsed.find("\r\n") + 2);
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
	    vm::log_d("request: parsing body " + unparsed);
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
    // std::cout << "LEFT TO PARSE: " << unparsed << std::endl;
}

void vm::http_request::set_request_method(http_request_method new_method)
{
    request_method = new_method;
}

void vm::http_request::set_url(std::string new_url)
{
    url = new_url;
}

void vm::http_request::add_header(std::string name, std::string value)
{
    headers[name] = value;
}


std::string vm::http_request::get_body()
{
    return body;
}

std::string vm::http_request::commit()
{
    std::string ret = commit_headers();
    ret += body;
    ret += "\r\n";
    return ret;
}

std::string vm::http_request::commit_headers()
{
    std::string ret = "";
    ret += request_method.description + " " + url + " " + http_version + "\r\n";
    for (auto header: headers)
    {
	ret += header.first + ": " + header.second + "\r\n";
    }
    ret += "\r\n";
    return ret;
}


std::map<std::string, std::string> vm::http_request::get_headers()
{
    return headers;
}

std::string vm::http_request::get_url()
{
    return url;
}

vm::http_request::http_request_method vm::http_request::get_request_method()
{
    return request_method;
}

const vm::http_request::http_request_method vm::http_request::GET = {"GET"};
const vm::http_request::http_request_method vm::http_request::POST = {"POST"};
const vm::http_request::http_request_method vm::http_request::HEAD = {"HEAD"};
const vm::http_request::http_request_method vm::http_request::PUT = {"PUT"};
const vm::http_request::http_request_method vm::http_request::DELETE = {"DELETE"};
const vm::http_request::http_request_method vm::http_request::TRACE = {"TRACE"};
const vm::http_request::http_request_method vm::http_request::CONNECT = {"CONNECT"};
