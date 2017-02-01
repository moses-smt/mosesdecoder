// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#include <string>
#include <map>
#include <stdint.h>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

namespace Moses2
{
class Parameter;

struct
    ServerOptions {
  bool is_serial;
  uint32_t numThreads; // might not be used any more, actually

  size_t sessionTimeout;   // this is related to Moses translation sessions
  size_t sessionCacheSize; // this is related to Moses translation sessions

  int port;              // this is for the abyss server
  std::string logfile;   // this is for the abyss server
  int maxConn;           // this is for the abyss server
  int maxConnBacklog;    // this is for the abyss server
  int keepaliveTimeout;  // this is for the abyss server
  int keepaliveMaxConn;  // this is for the abyss server
  int timeout;           // this is for the abyss server

  bool init(Parameter const& param);
  ServerOptions(Parameter const& param);
  ServerOptions();

  bool
  update(std::map<std::string,xmlrpc_c::value>const& params) {
    return true;
  }

};

}
