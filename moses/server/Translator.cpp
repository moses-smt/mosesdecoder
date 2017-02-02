// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-

#include "Translator.h"
#include "TranslationRequest.h"
#include "Server.h"

namespace MosesServer
{

using namespace std;
using namespace Moses;

Translator::
Translator(Server& server)
  : m_server(server),
    m_threadPool(server.options().numThreads)
{
  // signature and help strings are documentation -- the client
  // can query this information with a system.methodSignature and
  // system.methodHelp RPC.
  this->_signature = "S:S";
  this->_help = "Does translation";
}

void
Translator::
execute(xmlrpc_c::paramList const& paramList,
        xmlrpc_c::value *   const  retvalP)
{
  boost::condition_variable cond;
  boost::mutex mut;
  boost::shared_ptr<TranslationRequest> task;
  task = TranslationRequest::create(this, paramList,cond,mut);
  m_threadPool.Submit(task);
  boost::unique_lock<boost::mutex> lock(mut);
  while (!task->IsDone())
    cond.wait(lock);
  *retvalP = xmlrpc_c::value_struct(task->GetRetData());
}

Session const& 
Translator::
get_session(uint64_t const id)
{
  return m_server.get_session(id);
}

}
