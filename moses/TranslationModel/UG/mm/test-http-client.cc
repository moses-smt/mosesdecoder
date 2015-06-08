// -*- c++ -*-
#include "ug_http_client.h"

int main(int argc, char* argv[])
{
  try
    {
      if (argc != 2)
	{
	  std::cout << "Usage: async_client <url>\n";
	  std::cout << "Example:\n";
	  std::cout << "  async_client www.boost.org/LICENSE_1_0.txt\n";
	  return 1;
	}

      boost::asio::io_service io_service;
      Moses::http_client c(io_service, argv[1]);
      io_service.run();
      std::cout << c.content() << std::endl;
    }
  catch (std::exception& e)
    {
      std::cout << "Exception: " << e.what() << "\n";
    }

  return 0;
}
