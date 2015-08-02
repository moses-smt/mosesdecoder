// -*- c++ -*-
#pragma once

#include "moses/ThreadPool.h"
#include "moses/parameters/ServerOptions.h"
#include "session.h"
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#ifndef WITH_THREADS
#pragma message("COMPILING WITHOUT THREADS!")
#endif
namespace MosesServer
{
class
Translator : public xmlrpc_c::method
{
  Moses::ServerOptions m_server_options;
public:
  Translator(Moses::ServerOptions const& sopts);
  
  void execute(xmlrpc_c::paramList const& paramList,
               xmlrpc_c::value *   const  retvalP);

  Session const& get_session(uint64_t session_id);
private:
  Moses::ThreadPool m_threadPool;
  SessionCache   m_session_cache;
};

}
