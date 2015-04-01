// -*- c++ -*-
#pragma once

#include "moses/ThreadPool.h"
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#ifndef WITH_THREADS
#pragma message("COMPILING WITHOUT THREADS!")
#endif
namespace MosesServer
{
  class 
  // MosesServer::
  Translator : public xmlrpc_c::method
  {
  public:
    Translator(size_t numThreads = 10);
    
    void execute(xmlrpc_c::paramList const& paramList,
		 xmlrpc_c::value *   const  retvalP);
  private:
    Moses::ThreadPool m_threadPool;
  };
  
}
