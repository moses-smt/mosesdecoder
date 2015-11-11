#include "Optimizer.h"
#include <iostream>

namespace MosesServer
{
using namespace std;

Optimizer::
Optimizer()
{
  // signature and help strings are documentation -- the client
  // can query this information with a system.methodSignature and
  // system.methodHelp RPC.
  this->_signature = "S:S";
  this->_help = "Optimizes multi-model translation model";
}

void
Optimizer::
execute(xmlrpc_c::paramList const& paramList,
        xmlrpc_c::value *   const  retvalP)
{
#ifdef WITH_DLIB
  const params_t params = paramList.getStruct(0);
  params_t::const_iterator si;
  if ((si  = params.find("model_name")) == params.end()) {
    string msg = "Missing name of model to be optimized";
    msg += " (e.g. PhraseDictionaryMultiModelCounts0)";
    throw xmlrpc_c::fault(msg, xmlrpc_c::fault::CODE_PARSE);
  }
  const string model_name = xmlrpc_c::value_string(si->second);

  if ((si = params.find("phrase_pairs")) == params.end()) {
    throw xmlrpc_c::fault("Missing list of phrase pairs",
                          xmlrpc_c::fault::CODE_PARSE);
  }


  vector<pair<string, string> > phrase_pairs;

  xmlrpc_c::value_array pp_array = xmlrpc_c::value_array(si->second);
  vector<xmlrpc_c::value> ppValVec(pp_array.vectorValueValue());
  for (size_t i = 0; i < ppValVec.size(); ++i) {
    xmlrpc_c::value_array pp_array
    = xmlrpc_c::value_array(ppValVec[i]);
    vector<xmlrpc_c::value> pp(pp_array.vectorValueValue());
    string L1 = xmlrpc_c::value_string(pp[0]);
    string L2 = xmlrpc_c::value_string(pp[1]);
    phrase_pairs.push_back(make_pair(L1,L2));
  }

  // PhraseDictionaryMultiModel* pdmm
  // = (PhraseDictionaryMultiModel*) FindPhraseDictionary(model_name);
  PhraseDictionaryMultiModel* pdmm = FindPhraseDictionary(model_name);
  vector<float> weight_vector = pdmm->MinimizePerplexity(phrase_pairs);

  vector<xmlrpc_c::value> weight_vector_ret;
  for (size_t i=0; i < weight_vector.size(); i++)
    weight_vector_ret.push_back(xmlrpc_c::value_double(weight_vector[i]));

  *retvalP = xmlrpc_c::value_array(weight_vector_ret);
#else
  string errmsg = "Error: Perplexity minimization requires dlib ";
  errmsg += "(compilation option --with-dlib)";
  std::cerr << errmsg << std::endl;
  *retvalP = xmlrpc_c::value_string(errmsg);
#endif
}
}
