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
  m_mgr->Decode();

  string out;
  out = m_mgr->OutputBest();
  m_retData["text"] = xmlrpc_c::value_string(out);

  {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    m_done = true;
  }
  m_cond.notify_one();

  delete m_mgr;
}

void TranslationRequest::pack_hypothesis(const Manager& manager, Hypothesis const* h,
    std::string const& key,
    std::map<std::string, xmlrpc_c::value> & dest) const
{

}

}
