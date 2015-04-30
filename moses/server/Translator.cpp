#include "Translator.h"
#include "TranslationRequest.h"

namespace MosesServer
{

  using namespace std;
  using namespace Moses;

  Translator::
  Translator(size_t numThreads)
    : m_threadPool(numThreads)
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
    boost::shared_ptr<TranslationRequest> task
      = TranslationRequest::create(paramList,cond,mut);
    m_threadPool.Submit(task);
    boost::unique_lock<boost::mutex> lock(mut);
    while (!task->IsDone())
      cond.wait(lock);
    *retvalP = xmlrpc_c::value_struct(task->GetRetData());
  }

}
