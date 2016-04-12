#include <boost/foreach.hpp>
#include "TranslationRequest.h"
#include "../ManagerBase.h"
#include "../System.h"

using namespace std;

namespace Moses2
{
TranslationRequest::
TranslationRequest(xmlrpc_c::paramList const& paramList,
                   boost::condition_variable& cond,
                   boost::mutex& mut,
                   System &system,
                   const std::string &line,
                   long translationId)
:TranslationTask(system, line, translationId)
,m_cond(cond)
,m_mutex(mut)
,m_done(false)
{

}

boost::shared_ptr<TranslationRequest>
TranslationRequest::
create(Translator* translator,
    xmlrpc_c::paramList const& paramList,
    boost::condition_variable& cond,
    boost::mutex& mut,
    System &system,
    const std::string &line,
    long translationId)
{
  boost::shared_ptr<TranslationRequest> ret;
  TranslationRequest *request = new TranslationRequest(paramList, cond, mut, system, line, translationId);
  ret.reset(request);
  ret->m_translator = translator;
  return ret;
}
  
void
TranslationRequest::
Run()
{
  cerr << "Run A" << endl;
  run_phrase_decoder();
  cerr << "Run B" << endl;

  {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_done = true;
  }
  m_cond.notify_one();
  cerr << "Run C" << endl;

}

void
TranslationRequest::
run_phrase_decoder()
{
  m_mgr->Decode();

  string out;

  out = m_mgr->OutputBest();
  m_mgr->system.bestCollector->Write(m_mgr->m_translationId, out);

  /*
  Manager manager(this->self());
  manager.Decode();
  pack_hypothesis(manager, manager.GetBestHypothesis(), "text", m_retData);
  if (m_session_id)
    m_retData["session-id"] = xmlrpc_c::value_int(m_session_id);

  if (m_withGraphInfo) insertGraphInfo(manager,m_retData);
  if (m_withTopts) insertTranslationOptions(manager,m_retData);
  if (m_options->nbest.nbest_size) outputNBest(manager, m_retData);
  */
}

}
