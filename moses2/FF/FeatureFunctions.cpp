/*
 * FeatureFunctions.cpp
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "FeatureRegistry.h"
#include "FeatureFunctions.h"
#include "StatefulFeatureFunction.h"
#include "../System.h"
#include "../Scores.h"
#include "../MemPool.h"

#include "../TranslationModel/PhraseTable.h"
#include "../TranslationModel/UnknownWordPenalty.h"
#include "../SCFG/TargetPhraseImpl.h"
#include "../SCFG/Word.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "util/exception.hh"

using namespace std;

namespace Moses2
{
FeatureFunctions::FeatureFunctions(System &system) :
  m_system(system), m_ffStartInd(0)
{
}

FeatureFunctions::~FeatureFunctions()
{
  RemoveAllInColl(m_featureFunctions);
}

void FeatureFunctions::Load()
{
  // load, everything but pts
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    FeatureFunction *nonConstFF = const_cast<FeatureFunction*>(ff);
    PhraseTable *pt = dynamic_cast<PhraseTable*>(nonConstFF);

    if (pt) {
      // do nothing. load pt last
    } else {
      cerr << "Loading " << nonConstFF->GetName() << endl;
      nonConstFF->Load(m_system);
      cerr << "Finished loading " << nonConstFF->GetName() << endl;
    }
  }

// load pt
  BOOST_FOREACH(const PhraseTable *pt, phraseTables) {
    PhraseTable *nonConstPT = const_cast<PhraseTable*>(pt);
    cerr << "Loading " << nonConstPT->GetName() << endl;
    nonConstPT->Load(m_system);
    cerr << "Finished loading " << nonConstPT->GetName() << endl;
  }
}

void FeatureFunctions::Create()
{
  const Parameter &params = m_system.params;

  const PARAM_VEC *ffParams = params.GetParam("feature");
  UTIL_THROW_IF2(ffParams == NULL, "Must have [feature] section");

  BOOST_FOREACH(const std::string &line, *ffParams) {
    //cerr << "line=" << line << endl;
    FeatureFunction *ff = Create(line);

    m_featureFunctions.push_back(ff);

    StatefulFeatureFunction *sfff = dynamic_cast<StatefulFeatureFunction*>(ff);
    if (sfff) {
      sfff->SetStatefulInd(m_statefulFeatureFunctions.size());
      m_statefulFeatureFunctions.push_back(sfff);
    }

    if (ff->HasPhraseTableInd()) {
      ff->SetPhraseTableInd(m_withPhraseTableInd.size());
      m_withPhraseTableInd.push_back(ff);
    }

    PhraseTable *pt = dynamic_cast<PhraseTable*>(ff);
    if (pt) {
      pt->SetPtInd(phraseTables.size());
      phraseTables.push_back(pt);
    }

    UnknownWordPenalty *unkWP = dynamic_cast<UnknownWordPenalty *>(pt);
    if (unkWP) {
      m_unkWP = unkWP;

      // legacy support
      if (m_system.options.unk.drop) {
        unkWP->SetParameter("drop", "true");
      }
      if (m_system.options.unk.mark) {
        unkWP->SetParameter("prefix", m_system.options.unk.prefix);
        unkWP->SetParameter("suffix", m_system.options.unk.suffix);
      }
    }
  }

  OverrideFeatures();
}

FeatureFunction *FeatureFunctions::Create(const std::string &line)
{
  vector<string> toks = Tokenize(line);

  FeatureFunction *ff = FeatureRegistry::Instance().Construct(m_ffStartInd, toks[0], line);
  UTIL_THROW_IF2(ff == NULL, "Feature function not created");

  // name
  if (ff->GetName() == "") {
    ff->SetName(GetDefaultName(toks[0]));
  }

  m_ffStartInd += ff->GetNumScores();

  return ff;
}

std::string FeatureFunctions::GetDefaultName(const std::string &stub)
{
  size_t ind;
  boost::unordered_map<std::string, size_t>::iterator iter =
    m_defaultNames.find(stub);
  if (iter == m_defaultNames.end()) {
    m_defaultNames[stub] = 0;
    ind = 0;
  } else {
    ind = ++(iter->second);
  }
  return stub + SPrint(ind);
}

const FeatureFunction *FeatureFunctions::FindFeatureFunction(
  const std::string &name) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    if (ff->GetName() == name) {
      return ff;
    }
  }
  return NULL;
}

FeatureFunction *FeatureFunctions::FindFeatureFunction(
  const std::string &name)
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    if (ff->GetName() == name) {
      return const_cast<FeatureFunction *>(ff);
    }
  }
  return NULL;
}

const PhraseTable *FeatureFunctions::GetPhraseTableExcludeUnknownWordPenalty(size_t ptInd)
{
  // assume only 1 unk wp
  std::vector<const PhraseTable*> tmpVec(phraseTables);
  std::vector<const PhraseTable*>::iterator iter;
  for (iter = tmpVec.begin(); iter != tmpVec.end(); ++iter) {
    const PhraseTable *pt = *iter;
    if (pt == m_unkWP) {
      tmpVec.erase(iter);
      break;
    }
  }

  const PhraseTable *pt = tmpVec[ptInd];
  return pt;
}

void FeatureFunctions::EvaluateInIsolation(MemPool &pool, const System &system,
    const Phrase<Moses2::Word> &source, TargetPhraseImpl &targetPhrase) const
{
  SCORE estimatedScore = 0;

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    Scores& scores = targetPhrase.GetScores();
    ff->EvaluateInIsolation(pool, system, source, targetPhrase, scores, estimatedScore);
  }

  targetPhrase.SetEstimatedScore(estimatedScore);
}

void FeatureFunctions::EvaluateInIsolation(
  MemPool &pool,
  const System &system,
  const Phrase<SCFG::Word> &source,
  SCFG::TargetPhraseImpl &targetPhrase) const
{
  SCORE estimatedScore = 0;

  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    Scores& scores = targetPhrase.GetScores();
    ff->EvaluateInIsolation(pool, system, source, targetPhrase, scores, estimatedScore);
  }

  targetPhrase.SetEstimatedScore(estimatedScore);
}

void FeatureFunctions::EvaluateAfterTablePruning(MemPool &pool,
    const TargetPhrases &tps, const Phrase<Moses2::Word> &sourcePhrase) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    ff->EvaluateAfterTablePruning(pool, tps, sourcePhrase);
  }
}

void FeatureFunctions::EvaluateAfterTablePruning(MemPool &pool, const SCFG::TargetPhrases &tps,
    const Phrase<SCFG::Word> &sourcePhrase) const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    ff->EvaluateAfterTablePruning(pool, tps, sourcePhrase);
  }
}

void FeatureFunctions::EvaluateWhenAppliedBatch(const Batch &batch) const
{
  BOOST_FOREACH(const StatefulFeatureFunction *ff, m_statefulFeatureFunctions) {
    ff->EvaluateWhenAppliedBatch(m_system, batch);
  }
}

void FeatureFunctions::CleanUpAfterSentenceProcessing() const
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    ff->CleanUpAfterSentenceProcessing();
  }
}

void FeatureFunctions::ShowWeights(const Weights &allWeights)
{
  BOOST_FOREACH(const FeatureFunction *ff, m_featureFunctions) {
    cout << ff->GetName();
    if (ff->IsTuneable()) {
      cout << "=";
      vector<SCORE> weights = allWeights.GetWeights(*ff);
      for (size_t i = 0; i < weights.size(); ++i) {
        cout << " " << weights[i];
      }
      cout << endl;
    } else {
      cout << " UNTUNEABLE" << endl;
    }
  }
}

void FeatureFunctions::OverrideFeatures()
{
  const Parameter &parameter = m_system.params;

  const PARAM_VEC *params = parameter.GetParam("feature-overwrite");
  for (size_t i = 0; params && i < params->size(); ++i) {
    const string &str = params->at(i);
    vector<string> toks = Tokenize(str);
    UTIL_THROW_IF2(toks.size() <= 1, "Incorrect format for feature override: " << str);

    FeatureFunction *ff = FindFeatureFunction(toks[0]);
    UTIL_THROW_IF2(ff == NULL, "Feature function not found: " << toks[0]);

    for (size_t j = 1; j < toks.size(); ++j) {
      const string &keyValStr = toks[j];
      vector<string> keyVal = Tokenize(keyValStr, "=");
      UTIL_THROW_IF2(keyVal.size() != 2, "Incorrect format for parameter override: " << keyValStr);

      cerr << "Override " << ff->GetName() << " "
           << keyVal[0] << "=" << keyVal[1] << endl;

      ff->SetParameter(keyVal[0], keyVal[1]);

    }
  }

}

}

