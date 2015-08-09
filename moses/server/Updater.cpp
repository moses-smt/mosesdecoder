// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "Updater.h"

namespace MosesServer
{
using namespace Moses;
using namespace std;

Updater::
Updater()
{
  // signature and help strings are documentation -- the client
  // can query this information with a system.methodSignature and
  // system.methodHelp RPC.
  this->_signature = "S:S";
  this->_help = "Updates stuff";
}

void
Updater::
execute(xmlrpc_c::paramList const& paramList,
        xmlrpc_c::value *   const  retvalP)
{
#if PT_UG
  const params_t params = paramList.getStruct(0);
  breakOutParams(params);
  Mmsapt* pdsa = reinterpret_cast<Mmsapt*>(PhraseDictionary::GetColl()[0]);
  pdsa->add(m_src, m_trg, m_aln);
  XVERBOSE(1,"Done inserting\n");
  *retvalP = xmlrpc_c::value_string("Phrase table updated");
#endif
};

void
Updater::
breakOutParams(const params_t& params)
{
  params_t::const_iterator si = params.find("source");
  if(si == params.end())
    throw xmlrpc_c::fault("Missing source sentence",
                          xmlrpc_c::fault::CODE_PARSE);
  m_src = xmlrpc_c::value_string(si->second);
  XVERBOSE(1,"source = " << m_src << endl);
  si = params.find("target");
  if(si == params.end())
    throw xmlrpc_c::fault("Missing target sentence",
                          xmlrpc_c::fault::CODE_PARSE);
  m_trg = xmlrpc_c::value_string(si->second);
  XVERBOSE(1,"target = " << m_trg << endl);
  if((si = params.find("alignment")) == params.end())
    throw xmlrpc_c::fault("Missing alignment", xmlrpc_c::fault::CODE_PARSE);
  m_aln = xmlrpc_c::value_string(si->second);
  XVERBOSE(1,"alignment = " << m_aln << endl);
  m_bounded  = ((si = params.find("bounded")) != params.end());
  m_add2ORLM = ((si = params.find("updateORLM")) != params.end());
};

}
