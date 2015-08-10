// -*- mode: c++; indent-tabs-mode: nil; tab-width: -*-
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
  CloseSession : public xmlrpc_c::method
  {
    Server& m_server;
  public:
    CloseSession(Server& server);

    void execute(xmlrpc_c::paramList const& paramList,
		 xmlrpc_c::value *   const  retvalP);
    
  };
  
}
