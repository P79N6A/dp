#ifndef _HTTP_BOOST_H_
#define _HTTP_BOOST_H_

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include "util/log.h"

using boost::asio::ip::tcp;


namespace poseidon
{
namespace ors_offline
{
int httpGet(std::string host, std::string port, std::string path, std::string& response_str)
{
	LOG_INFO("httpGet Beginning.");
	try
	{
	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(host.c_str(), port.c_str());//(argv[1], "http");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << path.c_str() << " HTTP/1.0\r\n"; //argv[2] << " HTTP/1.0\r\n";
	request_stream << "Host: " << host.c_str() << "\r\n"; 
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	// Send the request.
	boost::asio::write(socket, request);

	// Read the response status line. The response streambuf will automatically
	// grow to accommodate the entire line. The growth may be limited by passing
	// a maximum size to the streambuf constructor.
	boost::asio::streambuf response;
    response.prepare((std::size_t)1000000000);
	boost::asio::read_until(socket, response, "\r\n");

	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	unsigned int status_code;
	response_stream >> status_code;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/")
	{
		LOG_ERROR("Http get: Invalid response.");
		return -1;
	}
    
	if (status_code != 200)
	{
		LOG_ERROR("Response returned with status code %d", status_code);
		return -1;
	}

	// Read the response headers, which are terminated by a blank line.
	boost::asio::read_until(socket, response, "\r\n\r\n");

	// Process the response headers.
	std::string header;
	while (std::getline(response_stream, header) && header != "\r")
		LOG_INFO("Http response header:%s", header.c_str());

	// Write whatever content we already have to output.
	if (response.size() > 0)
	{
		std::string tmp((std::istreambuf_iterator<char>(&response)), std::istreambuf_iterator<char>());
		response_str = tmp;
	}

	// Read until EOF, writing data to output as we go.
	boost::system::error_code error;
	while (boost::asio::read(socket, response,
		boost::asio::transfer_at_least(1), error))

	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);
	}
	catch (std::exception& e)
	{
		LOG_ERROR("httpGet Exception: %s", e.what());
		return -1;
	}

	LOG_INFO("httpGet end.");
	return 0;
}

}// namespace ors_offline
}// namespace poseidon
#endif
