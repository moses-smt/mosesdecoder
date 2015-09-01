// -*- c++ -*-
#pragma once

#include "moses/parameters/ServerOptions.h"
#include "Session.h"
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#ifndef WITH_THREADS
#pragma message("COMPILING WITHOUT THREADS!")
#else
#include "moses/ThreadPool.h"
#endif
namespace MosesServer
{

  class Server;

  class
  Translator : public xmlrpc_c::method
  {
    Server& m_server;
    // Moses::ServerOptions m_server_options;
  public:
    Translator(Server& server);
    
    void execute(xmlrpc_c::paramList const& paramList,
		 xmlrpc_c::value *   const  retvalP);
    
    Session const& get_session(uint64_t session_id);
  private:
    Moses::ThreadPool m_threadPool;
  };

}
