/*
 * Server.h
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */
#pragma once

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

namespace Moses2
{
class System;
class ServerOptions;
class Manager;

class Server
{
public:
  Server(ServerOptions &server_options, System &system);
  virtual ~Server();

  void run(System &system);

  ServerOptions const&
  options() const;

protected:
  ServerOptions &m_server_options;
  std::string m_pidfile;
  xmlrpc_c::registry m_registry;
  xmlrpc_c::methodPtr const m_translator;

};

} /* namespace Moses2 */

