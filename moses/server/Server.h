// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
#pragma once
#ifdef HAVE_XMLRPC_C
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>
#include "Translator.h"
#include "Optimizer.h"
#include "Updater.h"
#include "CloseSession.h"
#include "Session.h"
#include "moses/parameters/ServerOptions.h"
#endif

namespace MosesServer
{
  class Server
  {
    Moses::ServerOptions m_server_options;
    SessionCache   m_session_cache;
    xmlrpc_c::registry m_registry;
    xmlrpc_c::methodPtr const m_updater;
    xmlrpc_c::methodPtr const m_optimizer;
    xmlrpc_c::methodPtr const m_translator;
    xmlrpc_c::methodPtr const m_close_session;
    
  public:
    Server(Moses::Parameter& params);

    int run();
    void delete_session(uint64_t const session_id);

    Moses::ServerOptions const& 
    options() const;
    
    Session const& 
    get_session(uint64_t session_id);

  };
}
