// -*- mode: c++; indent-tabs-mode: nil; tab-width: -*-
#include "CloseSession.h"
#include "TranslationRequest.h"
#include "Server.h"

namespace MosesServer
{
  CloseSession::
  CloseSession(Server& server)
    : m_server(server) 
  { }
  
  void 
  CloseSession::
  execute(xmlrpc_c::paramList const& paramList,
	  xmlrpc_c::value *   const  retvalP)
  {
    typedef std::map<std::string, xmlrpc_c::value> params_t;
    paramList.verifyEnd(1); // ??? UG
    params_t const& params = paramList.getStruct(0);
    params_t::const_iterator si = params.find("session-id");
    if (si != params.end())
      {
	uint64_t session_id = xmlrpc_c::value_int(si->second);
	m_server.delete_session(session_id);
	*retvalP = xmlrpc_c::value_string("Session closed");
      }
  }
  
}
