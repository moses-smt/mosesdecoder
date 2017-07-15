// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "PackScores.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include <boost/foreach.hpp>
namespace Moses {

void
PackScores(FeatureFunction const& ff, FVector const& S,
	   std::map<std::string, xmlrpc_c::value>& M)
{
  std::vector<xmlrpc_c::value> v;
  size_t N = ff.GetNumScoreComponents();

  std::vector<xmlrpc_c::value> dense; 
  dense.reserve(N);
  size_t o = ff.GetIndex();
  for (size_t i = 0; i < N; ++i) 
    if (ff.IsTuneableComponent(i)) 
      dense.push_back(xmlrpc_c::value_double(S[o+i]));
  v.push_back(xmlrpc_c::value_array(dense)); 

  std::map<std::string,xmlrpc_c::value> sparse;
  typedef FVector::FNVmap::const_iterator iter;
  for(iter m = S.cbegin(); m != S.cend(); ++m)
    sparse[m->first.name()] = xmlrpc_c::value_double(m->second);
  v.push_back(xmlrpc_c::value_struct(sparse));
  M[ff.GetScoreProducerDescription()] = xmlrpc_c::value_array(v);
}

xmlrpc_c::value
PackScores(ScoreComponentCollection const& S)
{
  std::map<std::string, xmlrpc_c::value> M;
  typedef StatefulFeatureFunction SFFF;
  typedef StatelessFeatureFunction SLFF;
  BOOST_FOREACH(SFFF const* ff, SFFF::GetStatefulFeatureFunctions())
    if (ff->IsTuneable()) 
      PackScores(*ff, S.GetScoresVector(), M);
  BOOST_FOREACH(SLFF const* ff, SLFF::GetStatelessFeatureFunctions())
    if (ff->IsTuneable()) 
      PackScores(*ff, S.GetScoresVector(), M);
  return xmlrpc_c::value_struct(M);
}
}
