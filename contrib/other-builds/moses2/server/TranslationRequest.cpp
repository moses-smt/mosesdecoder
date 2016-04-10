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
create(Translator* translator, xmlrpc_c::paramList const& paramList,
       boost::condition_variable& cond, boost::mutex& mut)
{

}
  
void
TranslationRequest::
Run()
{
}


}
