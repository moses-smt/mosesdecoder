/*
 * Transliteration.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "Transliteration.h"
#include "../System.h"
#include "../Scores.h"
#include "../InputType.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "../PhraseBased/InputPath.h"
#include "../PhraseBased/TargetPhrases.h"
#include "../PhraseBased/Sentence.h"
#include "../SCFG/InputPath.h"
#include "../SCFG/TargetPhraseImpl.h"
#include "../SCFG/Manager.h"
#include "../SCFG/Sentence.h"
#include "../SCFG/ActiveChart.h"
#include "util/tempfile.hh"
#include "../legacy/Util2.h"

using namespace std;

namespace Moses2
{

Transliteration::Transliteration(size_t startInd, const std::string &line) :
  PhraseTable(startInd, line)
{
  ReadParameters();
  UTIL_THROW_IF2(m_mosesDir.empty() ||
                 m_scriptDir.empty() ||
                 m_externalDir.empty() ||
                 m_inputLang.empty() ||
                 m_outputLang.empty(), "Must specify all arguments");
}

Transliteration::~Transliteration()
{
  // TODO Auto-generated destructor stub
}

void
Transliteration::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "moses-dir") {
    m_mosesDir = value;
  } else if (key == "script-dir") {
    m_scriptDir = value;
  } else if (key == "external-dir") {
    m_externalDir = value;
  } else if (key == "input-lang") {
    m_inputLang = value;
  } else if (key == "output-lang") {
    m_outputLang = value;
  } else {
    PhraseTable::SetParameter(key, value);
  }
}

void Transliteration::Lookup(const Manager &mgr,
                             InputPathsBase &inputPaths) const
{
  BOOST_FOREACH(InputPathBase *pathBase, inputPaths) {
    InputPath *path = static_cast<InputPath*>(pathBase);

    if (SatisfyBackoff(mgr, *path)) {
      const SubPhrase<Moses2::Word> &phrase = path->subPhrase;

      TargetPhrases *tps = Lookup(mgr, mgr.GetPool(), *path);
      path->AddTargetPhrases(*this, tps);
    }
  }

}

TargetPhrases *Transliteration::Lookup(const Manager &mgr, MemPool &pool,
                                       InputPath &inputPath) const
{
  const SubPhrase<Moses2::Word> &sourcePhrase = inputPath.subPhrase;
  size_t hash = sourcePhrase.hash();

  // TRANSLITERATE
  const util::temp_file inFile;
  const util::temp_dir outDir;

  ofstream inStream(inFile.path().c_str());
  inStream << sourcePhrase.Debug(mgr.system) << endl;
  inStream.close();

  string cmd = m_scriptDir + "/Transliteration/prepare-transliteration-phrase-table.pl" +
               " --transliteration-model-dir " + m_filePath +
               " --moses-src-dir " + m_mosesDir +
               " --external-bin-dir " + m_externalDir +
               " --input-extension " + m_inputLang +
               " --output-extension " + m_outputLang +
               " --oov-file " + inFile.path() +
               " --out-dir " + outDir.path();

  int ret = system(cmd.c_str());
  UTIL_THROW_IF2(ret != 0, "Transliteration script error");

  TargetPhrases *tps = NULL;
  tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, 1);

  vector<TargetPhraseImpl*> targetPhrases
  = CreateTargetPhrases(mgr, pool, sourcePhrase, outDir.path());

  vector<TargetPhraseImpl*>::const_iterator iter;
  for (iter = targetPhrases.begin(); iter != targetPhrases.end(); ++iter) {
    TargetPhraseImpl *tp = *iter;
    tps->AddTargetPhrase(*tp);
  }
  mgr.system.featureFunctions.EvaluateAfterTablePruning(pool, *tps, sourcePhrase);

  inputPath.AddTargetPhrases(*this, tps);
}

std::vector<TargetPhraseImpl*> Transliteration::CreateTargetPhrases(
  const Manager &mgr,
  MemPool &pool,
  const SubPhrase<Moses2::Word> &sourcePhrase,
  const std::string &outDir) const
{
  std::vector<TargetPhraseImpl*> ret;

  string outPath = outDir + "/out.txt";
  ifstream outStream(outPath.c_str());

  string line;
  while (getline(outStream, line)) {
    vector<string> toks = Moses2::Tokenize(line, "\t");
    UTIL_THROW_IF2(toks.size() != 2, "Error in transliteration output file. Expecting word\tscore");

    TargetPhraseImpl *tp =
      new (pool.Allocate<TargetPhraseImpl>()) TargetPhraseImpl(pool, *this, mgr.system, 1);
    Moses2::Word &word = (*tp)[0];
    word.CreateFromString(mgr.system.GetVocab(), mgr.system, toks[0]);

    float score = Scan<float>(toks[1]);
    tp->GetScores().PlusEquals(mgr.system, *this, score);

    // score of all other ff when this rule is being loaded
    mgr.system.featureFunctions.EvaluateInIsolation(pool, mgr.system, sourcePhrase, *tp);

    ret.push_back(tp);
  }

  outStream.close();

  return ret;

}


void Transliteration::EvaluateInIsolation(const System &system,
    const Phrase<Moses2::Word> &source, const TargetPhraseImpl &targetPhrase, Scores &scores,
    SCORE &estimatedScore) const
{
  UTIL_THROW2("Not implemented");
}

// SCFG ///////////////////////////////////////////////////////////////////////////////////////////
void Transliteration::InitActiveChart(
  MemPool &pool,
  const SCFG::Manager &mgr,
  SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void Transliteration::Lookup(MemPool &pool,
                             const SCFG::Manager &mgr,
                             size_t maxChartSpan,
                             const SCFG::Stacks &stacks,
                             SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void Transliteration::LookupUnary(MemPool &pool,
                                  const SCFG::Manager &mgr,
                                  const SCFG::Stacks &stacks,
                                  SCFG::InputPath &path) const
{
  UTIL_THROW2("Not implemented");
}

void Transliteration::LookupNT(
  MemPool &pool,
  const SCFG::Manager &mgr,
  const Moses2::Range &subPhraseRange,
  const SCFG::InputPath &prevPath,
  const SCFG::Stacks &stacks,
  SCFG::InputPath &outPath) const
{
  UTIL_THROW2("Not implemented");
}

void Transliteration::LookupGivenWord(
  MemPool &pool,
  const SCFG::Manager &mgr,
  const SCFG::InputPath &prevPath,
  const SCFG::Word &wordSought,
  const Moses2::Hypotheses *hypos,
  const Moses2::Range &subPhraseRange,
  SCFG::InputPath &outPath) const
{
  UTIL_THROW2("Not implemented");
}

void Transliteration::LookupGivenNode(
  MemPool &pool,
  const SCFG::Manager &mgr,
  const SCFG::ActiveChartEntry &prevEntry,
  const SCFG::Word &wordSought,
  const Moses2::Hypotheses *hypos,
  const Moses2::Range &subPhraseRange,
  SCFG::InputPath &outPath) const
{
  UTIL_THROW2("Not implemented");
}

}

