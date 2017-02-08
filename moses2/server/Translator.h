/*
 * Translator.h
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <boost/thread/shared_mutex.hpp>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include "../legacy/ThreadPool.h"

namespace Moses2
{
class Server;
class System;
class Manager;

class Translator : public xmlrpc_c::method
{
public:
  Translator(Server& server, System &system);
  virtual ~Translator();

  void execute(xmlrpc_c::paramList const& paramList,
               xmlrpc_c::value *   const  retvalP);

protected:
  Server& m_server;
  Moses2::ThreadPool m_threadPool;
  System &m_system;
  long m_translationId;
  boost::shared_mutex m_accessLock;

};

} /* namespace Moses2 */

