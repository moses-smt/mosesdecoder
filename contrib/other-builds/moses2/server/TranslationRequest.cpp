#include <boost/foreach.hpp>
#include "TranslationRequest.h"

using namespace std;

namespace Moses2
{

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

bool
check(std::map<std::string, xmlrpc_c::value> const& param, 
      std::string const key)
{
}

void
TranslationRequest::
parse_request(std::map<std::string, xmlrpc_c::value> const& params)
{
} // end of Translationtask::parse_request()


void
TranslationRequest::
run_chart_decoder()
{
}


void
TranslationRequest::
run_phrase_decoder()
{
}
}
