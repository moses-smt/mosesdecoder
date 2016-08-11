/*
 * Weights.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <cassert>
#include <string>
#include <vector>
#include "FF/FeatureFunction.h"
#include "FF/FeatureFunctions.h"
#include "Weights.h"
#include "System.h"
#include "legacy/Util2.h"

using namespace std;

namespace Moses2
{

Weights::Weights()
{
  // TODO Auto-generated constructor stub

}

Weights::~Weights()
{
  // TODO Auto-generated destructor stub
}

void Weights::Init(const FeatureFunctions &ffs)
{
  size_t totalNumScores = ffs.GetNumScores();
  //cerr << "totalNumScores=" << totalNumScores << endl;
  m_weights.resize(totalNumScores, 1);
}

std::ostream &Weights::Debug(std::ostream &out, const System &system) const
{
  const FeatureFunctions &ffs  = system.featureFunctions;
  size_t numScores = ffs.GetNumScores();
  for (size_t i = 0; i < numScores; ++i) {
    out << m_weights[i] << " ";
  }

}


void Weights::CreateFromString(const FeatureFunctions &ffs,
    const std::string &line)
{
  std::vector<std::string> toks = Tokenize(line);
  assert(toks.size());

  string ffName = toks[0];
  assert(ffName[ffName.size() - 1] == '=');

  ffName = ffName.substr(0, ffName.size() - 1);
  //cerr << "ffName=" << ffName << endl;

  const FeatureFunction *ff = ffs.FindFeatureFunction(ffName);
  assert(ff);
  size_t startInd = ff->GetStartInd();
  size_t numScores = ff->GetNumScores();
  assert(numScores == toks.size() - 1);

  for (size_t i = 0; i < numScores; ++i) {
    SCORE score = Scan<SCORE>(toks[i + 1]);
    m_weights[i + startInd] = score;
  }
}

}

