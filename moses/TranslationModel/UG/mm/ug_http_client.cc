#include "ug_http_client.h"
#include "moses/Util.h"
#include <iostream>

namespace Moses
{
using boost::asio::ip::tcp;

std::string http_client::content() const { return m_content.str(); }

http_client::
http_client(boost::asio::io_service& io_service,
	    std::string const& server, 
	    std::string const& port,
	    std::string const& path)
  : resolver_(io_service), socket_(io_service)
{
  init(server, port, path);
}
  
http_client::
http_client(boost::asio::io_service& io_service, std::string url, std::ostream* log)
  : resolver_(io_service), socket_(io_service)
{
  std::string server;
  std::string path = "/";
  std::string port = "http";
  size_t p = url.find("://"), q;
  if (p < url.size()) 
    {
      port = url.substr(0,p);
      url.erase(0, p+3);
    }
  p = std::min(url.find_first_of(":/"), url.size());
  q = std::min(url.find("/"), url.size());
  if (p < url.size() && url[p] == ':') 
    port = url.substr(p+1,q-p-1);
  server = url.substr(0,p);
  if (q < url.size()) 
    path = url.substr(q);
#if 1
  if (log)
    {
      *log << HERE << std::endl;
      // *log << "URL    " << url << std::endl;
      *log << "SERVER " << server << std::endl;
      *log << "PORT   " << port << "" << std::endl;
      *log << "PATH   " << path << std::endl; 
    }
#endif
  init(server, port, path);
}

void 
http_client::
init(std::string const& server, std::string const& port, std::string const& path)
{
  // Form the request. We specify the "Connection: close" header so
  // that the server will close the socket after transmitting the
  // response. This will allow us to treat all data up until the EOF
  // as the content.

  std::ostream request_stream(&request_);
  request_stream << "GET " << path << " HTTP/1.0\r\n";
  request_stream << "Host: " << server << "\r\n";
  request_stream << "Accept: */*\r\n";
  request_stream << "Connection: close\r\n\r\n";
  
  // Start an asynchronous resolve to translate the server and service names
  // into a list of endpoints.
  tcp::resolver::query query(server, port.c_str());
  resolver_.async_resolve(query,
			  boost::bind(&http_client::handle_resolve, this,
				      boost::asio::placeholders::error,
				      boost::asio::placeholders::iterator));
  
}

void 
http_client::
handle_resolve(const boost::system::error_code& err,
	       tcp::resolver::iterator endpoint_iterator)
{
  if (!err)
    {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
			    boost::bind(&http_client::handle_connect, this,
					boost::asio::placeholders::error, ++endpoint_iterator));
    }
  else
    {
      m_error << "Error: " << err.message() << "\n";
    }
}

void 
http_client::
handle_connect(const boost::system::error_code& err,
		      tcp::resolver::iterator endpoint_iterator)
{
  if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
			       boost::bind(&http_client::handle_write_request, this,
					   boost::asio::placeholders::error));
    }
  else if (endpoint_iterator != tcp::resolver::iterator())
    {
      // The connection failed. Try the next endpoint in the list.
      socket_.close();
      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
			    boost::bind(&http_client::handle_connect, this,
					boost::asio::placeholders::error, ++endpoint_iterator));
    }
  else m_error << "Error: " << err.message() << "\n";
}

void 
http_client::
handle_write_request(const boost::system::error_code& err)
{
  using namespace boost::asio;
  if (err) { m_error << "Error: " << err.message() << "\n"; return; }
  
  // Read the response status line. The response_ streambuf will
  // automatically grow to accommodate the entire line. The growth may be
  // limited by passing a maximum size to the streambuf constructor.
  async_read_until(socket_, response_, "\r\n",
		   boost::bind(&http_client::handle_read_status_line,
			       this, placeholders::error));
}

void 
http_client::
handle_read_status_line(const boost::system::error_code& err)
{
  if (err) { m_error << "Error: " << err << "\n"; return; }

  using namespace boost::asio;
  // Check that response is OK.
  std::istream response_stream(&response_);
  response_stream >> m_http_version >> m_status_code;
  std::getline(response_stream, m_status_message);
  if (!response_stream || m_http_version.substr(0, 5) != "HTTP/")
    m_error << "Invalid response\n"; 
  else if (m_status_code != 200)
    m_error << "Response returned with status code " << m_status_code << "\n";
  else // Read the response headers, which are terminated by a blank line.
    async_read_until(socket_, response_, "\r\n\r\n",
		     boost::bind(&http_client::handle_read_headers, this,
				 placeholders::error));
}


void 
http_client::
handle_read_headers(const boost::system::error_code& err)
{
  if (err) { m_error << "Error: " << err << "\n"; return; }
  
  // Process the response headers.
  std::istream response_stream(&response_);
  std::string line;
  while (std::getline(response_stream, line) && line != "\r")
    m_header.push_back(line);
  
  // Write whatever content we already have to output.
  if (response_.size() > 0)
    m_content << &response_;
  
  using namespace boost::asio;
  // Start reading remaining data until EOF.
  async_read(socket_, response_, transfer_at_least(1),
	     boost::bind(&http_client::handle_read_content, this,
			 placeholders::error));
}

void 
http_client::
handle_read_content(const boost::system::error_code& err)
{
  using namespace boost::asio;
  if(!err)
    {
      // Write all of the data that has been read so far.
      // Then continue reading remaining data until EOF.
      m_content << &response_; 
      async_read(socket_, response_, transfer_at_least(1),
		 boost::bind(&http_client::handle_read_content, this,
			     placeholders::error));
    }
  else if (err != boost::asio::error::eof) 
    { 
      m_error << "Error: " << err << "\n"; 
    }
}

std::string
uri_encode(std::string const& in)
{
  char buf[3 * in.size() + 1];
  size_t i = 0;
  for (unsigned char const* c = (unsigned char const*)in.c_str(); *c; ++c)
    {
      // cout << *c << " " << int(*c) << endl;
      if (*c == ' ') buf[i++] = '+';
      else if (*c == '.' || *c == '~' || *c == '_' || *c == '-') buf[i++] = *c;
      else if (*c <  '0') i += sprintf(buf+i, "%%%02x", int(*c));
      else if (*c <= '9') buf[i++] = *c;
      else if (*c <  'A') i += sprintf(buf+i, "%%%02x", int(*c));
      else if (*c <= 'Z') buf[i++] = *c;
      else if (*c <  'a') i += sprintf(buf+i, "%%%02x", int(*c));
      else if (*c <= 'z') buf[i++] = *c; 
      else i += sprintf(buf+i, "%%%x", int(*c));
    }
  buf[i] = 0;
  return std::string(buf);
}

}
