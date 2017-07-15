/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>
#include <vector>
#include <sstream>
#include "Manager.h"
#include "TargetPhraseImpl.h"
#include "InputPath.h"
#include "Sentence.h"

#include "Normal/Search.h"
#include "CubePruningMiniStack/Search.h"

/*
 #include "CubePruningPerMiniStack/Search.h"
 #include "CubePruningPerBitmap/Search.h"
 #include "CubePruningCardinalStack/Search.h"
 #include "CubePruningBitmapStack/Search.h"
 */
#include "../TrellisPaths.h"
#include "../System.h"
#include "../Phrase.h"
#include "../InputPathsBase.h"
#include "../TranslationModel/PhraseTable.h"
#include "../TranslationModel/UnknownWordPenalty.h"
#include "../legacy/Range.h"
#include "../PhraseBased/TargetPhrases.h"

using namespace std;

namespace Moses2
{
Manager::Manager(System &sys, const TranslationTask &task,
                 const std::string &inputStr, long translationId) :
  ManagerBase(sys, task, inputStr, translationId)
  ,m_search(NULL)
  ,m_bitmaps(NULL)
{
  //cerr << translationId << " inputStr=" << inputStr << endl;
}

Manager::~Manager()
{
  //cerr << "Start ~Manager " << this << endl;
  delete m_search;
  delete m_bitmaps;
  //cerr << "Finish ~Manager " << this << endl;
}

void Manager::Init()
{
  // init pools etc
  InitPools();

  FactorCollection &vocab = system.GetVocab();
  m_input = Moses2::Sentence::CreateFromString(GetPool(), vocab, system, m_inputStr);

  m_bitmaps = new Bitmaps(GetPool());

  const PhraseTable &firstPt = *system.featureFunctions.phraseTables[0];
  m_initPhrase = new (GetPool().Allocate<TargetPhraseImpl>()) TargetPhraseImpl(
    GetPool(), firstPt, system, 0);

  const Sentence &sentence = static_cast<const Sentence&>(GetInput());
  //cerr << "sentence=" << sentence.GetSize() << " " << sentence.Debug(system) << endl;

  m_inputPaths.Init(sentence, *this);

  // xml
  const UnknownWordPenalty *unkWP = system.featureFunctions.GetUnknownWordPenalty();
  UTIL_THROW_IF2(unkWP == NULL, "There must be a UnknownWordPenalty FF");
  unkWP->ProcessXML(*this, GetPool(), sentence, m_inputPaths);

  // lookup with every pt
  const std::vector<const PhraseTable*> &pts = system.mappings;
  for (size_t i = 0; i < pts.size(); ++i) {
    const PhraseTable &pt = *pts[i];
    //cerr << "Looking up from " << pt.GetName() << endl;
    pt.Lookup(*this, m_inputPaths);
  }
  //m_inputPaths.DeleteUnusedPaths();
  CalcFutureScore();

  m_bitmaps->Init(sentence.GetSize(), vector<bool>(0));

  switch (system.options.search.algo) {
  case Normal:
    m_search = new NSNormal::Search(*this);
    break;
  case NormalBatch:
    //m_search = new NSBatch::Search(*this);
    UTIL_THROW2("Not implemented");
    break;
  case CubePruning:
  case CubePruningMiniStack:
    m_search = new NSCubePruningMiniStack::Search(*this);
    break;
    /*
     case CubePruningPerMiniStack:
     m_search = new NSCubePruningPerMiniStack::Search(*this);
     break;
     case CubePruningPerBitmap:
     m_search = new NSCubePruningPerBitmap::Search(*this);
     break;
     case CubePruningCardinalStack:
     m_search = new NSCubePruningCardinalStack::Search(*this);
     break;
     case CubePruningBitmapStack:
     m_search = new NSCubePruningBitmapStack::Search(*this);
     break;
     */
  default:
    UTIL_THROW2("Unknown search algorithm");
  }
}

void Manager::Decode()
{
  //cerr << "Start Decode " << this << endl;

  Init();
  m_search->Decode();

  //cerr << "Finished Decode " << this << endl;
}

void Manager::CalcFutureScore()
{
  const Sentence &sentence = static_cast<const Sentence&>(GetInput());
  size_t size = sentence.GetSize();
  m_estimatedScores =
    new (GetPool().Allocate<EstimatedScores>()) EstimatedScores(GetPool(),
        size);
  m_estimatedScores->InitTriangle(-numeric_limits<SCORE>::infinity());

  // walk all the translation options and record the cheapest option for each span
  BOOST_FOREACH(const InputPathBase *path, m_inputPaths) {
    const Range &range = path->range;
    SCORE bestScore = -numeric_limits<SCORE>::infinity();

    size_t numPt = system.mappings.size();
    for (size_t i = 0; i < numPt; ++i) {
      const TargetPhrases *tps = static_cast<const InputPath*>(path)->targetPhrases[i];
      if (tps) {
        BOOST_FOREACH(const TargetPhraseImpl *tp, *tps) {
          SCORE score = tp->GetFutureScore();
          if (score > bestScore) {
            bestScore = score;
          }
        }
      }
    }
    m_estimatedScores->SetValue(range.GetStartPos(), range.GetEndPos(), bestScore);
  }

  // now fill all the cells in the strictly upper triangle
  //   there is no way to modify the diagonal now, in the case
  //   where no translation option covers a single-word span,
  //   we leave the +inf in the matrix
  // like in chart parsing we want each cell to contain the highest score
  // of the full-span trOpt or the sum of scores of joining two smaller spans

  for (size_t colstart = 1; colstart < size; colstart++) {
    for (size_t diagshift = 0; diagshift < size - colstart; diagshift++) {
      size_t sPos = diagshift;
      size_t ePos = colstart + diagshift;
      for (size_t joinAt = sPos; joinAt < ePos; joinAt++) {
        float joinedScore = m_estimatedScores->GetValue(sPos, joinAt)
                            + m_estimatedScores->GetValue(joinAt + 1, ePos);
        // uncomment to see the cell filling scheme
        // TRACE_ERR("[" << sPos << "," << ePos << "] <-? ["
        // 	  << sPos << "," << joinAt << "]+["
        // 	  << joinAt+1 << "," << ePos << "] (colstart: "
        // 	  << colstart << ", diagshift: " << diagshift << ")"
        // 	  << endl);

        if (joinedScore > m_estimatedScores->GetValue(sPos, ePos)) m_estimatedScores->SetValue(
            sPos, ePos, joinedScore);
      }
    }
  }

  //cerr << "Square matrix:" << endl;
  //cerr << *m_estimatedScores << endl;
}

std::string Manager::OutputBest() const
{
  stringstream out;
  Moses2::FixPrecision(out);

  const Hypothesis *bestHypo = m_search->GetBestHypo();
  if (bestHypo) {
    if (system.options.output.ReportHypoScore) {
      out << bestHypo->GetScores().GetTotalScore() << " ";
    }

    bestHypo->OutputToStream(out);
    //cerr << "BEST TRANSLATION: " << *bestHypo;
  } else {
    if (system.options.output.ReportHypoScore) {
      out << "0 ";
    }
    //cerr << "NO TRANSLATION " << m_input->GetTranslationId() << endl;
  }

  return out.str();
  //cerr << endl;
}

std::string Manager::OutputNBest()
{
  arcLists.Sort();

  boost::unordered_set<size_t> distinctHypos;

  TrellisPaths<TrellisPath> contenders;
  m_search->AddInitialTrellisPaths(contenders);

  long transId = GetTranslationId();

  // MAIN LOOP
  stringstream out;
  //Moses2::FixPrecision(out);

  size_t maxIter = system.options.nbest.nbest_size * system.options.nbest.factor;
  size_t bestInd = 0;
  for (size_t i = 0; i < maxIter; ++i) {
    if (bestInd > system.options.nbest.nbest_size || contenders.empty()) {
      break;
    }

    //cerr << "bestInd=" << bestInd << endl;
    TrellisPath *path = contenders.Get();

    bool ok = false;
    if (system.options.nbest.only_distinct) {
      string tgtPhrase = path->OutputTargetPhrase(system);
      //cerr << "tgtPhrase=" << tgtPhrase << endl;
      boost::hash<std::string> string_hash;
      size_t hash = string_hash(tgtPhrase);

      if (distinctHypos.insert(hash).second) {
        ok = true;
      }
    } else {
      ok = true;
    }

    if (ok) {
      ++bestInd;
      out << transId << " ||| ";
      path->OutputToStream(out, system);
      out << "\n";
    }

    // create next paths
    path->CreateDeviantPaths(contenders, arcLists, GetPool(), system);

    delete path;
  }

  return out.str();
}

std::string Manager::OutputTransOpt()
{
  return "";
}

}

