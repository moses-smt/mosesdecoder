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

using namespace std;

namespace Moses2
{

Translator::Translator(Server& server, System &system)
  : m_server(server),
    m_threadPool(server.options().numThreads),
    m_system(system),
    m_translationId(0)
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
                         xmlrpc_c::value *const  retvalP)
{
  typedef std::map<std::string,xmlrpc_c::value> param_t;
  param_t const& params = paramList.getStruct(0);
  param_t::const_iterator si;
  si = params.find("text");
  if (si == params.end()) {
    throw xmlrpc_c::fault("Missing source text", xmlrpc_c::fault::CODE_PARSE);
  }

  string line = xmlrpc_c::value_string(si->second);
  long translationId;

  // get unique id. Thread safe
  {
    boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
    translationId = m_translationId++;
  }

  boost::condition_variable cond;
  boost::mutex mut;
  boost::shared_ptr<TranslationRequest> task;
  task = TranslationRequest::create(this, paramList,cond,mut, m_system, line, translationId);
  m_threadPool.Submit(task);
  boost::unique_lock<boost::mutex> lock(mut);
  while (!task->IsDone()) {
    cond.wait(lock);
  }
  *retvalP = xmlrpc_c::value_struct(task->GetRetData());
}

} /* namespace Moses2 */
