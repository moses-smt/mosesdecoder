/*
 * Translator.h
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include "../legacy/ThreadPool.h"

namespace Moses2
{
class Server;

class Translator : public xmlrpc_c::method
{
public:
  Translator(Server& server);
  virtual ~Translator();

  void execute(xmlrpc_c::paramList const& paramList,
   xmlrpc_c::value *   const  retvalP);

protected:
  Server& m_server;
  Moses2::ThreadPool m_threadPool;
};

} /* namespace Moses2 */

