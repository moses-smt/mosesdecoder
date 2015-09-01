// -*- c++ -*-
// Adapted by Ulrich Germann from: 
// async_client.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace Moses
{
using boost::asio::ip::tcp;

std::string uri_encode(std::string const& in);

class http_client
{
  std::ostringstream m_content;
  std::vector<std::string> m_header;
  std::string m_http_version;
  unsigned int m_status_code;
  std::string m_status_message;
  std::ostringstream m_error;

public:
  http_client(boost::asio::io_service& io_service, std::string url, std::ostream* log);
  http_client(boost::asio::io_service& io_service,
	      std::string const& server, 
	      std::string const& port, 
	      std::string const& path);
private:

  void init(std::string const& server, 
	    std::string const& port, 
	    std::string const& path);

  void handle_resolve(const boost::system::error_code& err,
		      tcp::resolver::iterator endpoint_iterator);
  void handle_connect(const boost::system::error_code& err,
		      tcp::resolver::iterator endpoint_iterator);
  void handle_write_request(const boost::system::error_code& err);
  void handle_read_status_line(const boost::system::error_code& err);
  void handle_read_headers(const boost::system::error_code& err);
  void handle_read_content(const boost::system::error_code& err);
  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
public:
  std::string content() const;
  std::string error_msg() const { return m_error.str(); }
};

}
