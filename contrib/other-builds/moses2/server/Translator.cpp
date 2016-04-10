/*
 * Translator.cpp
 *
 *  Created on: 1 Apr 2016
 *      Author: hieu
 */
#include <boost/shared_ptr.hpp>
#include "Translator.h"
#include "TranslationRequest.h"
#include "Server.h"
#include "../parameters/ServerOptions.h"

namespace Moses2
{

Translator::Translator(Server& server)
: m_server(server),
  m_threadPool(server.options().numThreads)
{
  // signature and help strings are documentation -- the client
  // can query this information with a system.methodSignature and
  // system.methodHelp RPC.
  this->_signature = "S:S";
  this->_help = "Does translation";
}

Translator::~Translator()
{
  // TODO Auto-generated destructor stub
}

void Translator::execute(xmlrpc_c::paramList const& paramList,
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

} /* namespace Moses2 */
