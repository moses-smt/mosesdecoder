#include <boost/foreach.hpp>
#include "TranslationRequest.h"

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
}


}
