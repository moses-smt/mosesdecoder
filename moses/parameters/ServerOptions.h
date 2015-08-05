// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include "moses/Parameter.h"
namespace Moses
{

  struct 
  ServerOptions 
  {
    int port;
    bool is_serial;
    std::string logfile;
    uint32_t num_threads;
    size_t session_timeout;
    size_t session_cache_size;
    bool init(Parameter const& param);
    ServerOptions(Parameter const& param);
  };

}
