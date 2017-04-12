// $Id$

#include <list>
#include "TranslationOptionCollectionLattice.h"
#include "ConfusionNet.h"
#include "WordLattice.h"
#include "DecodeGraph.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "FactorCollection.h"
#include "FF/InputFeature.h"
#include "TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "util/exception.hh"
#include "TranslationTask.h"

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionLattice
::TranslationOptionCollectionLattice
( ttasksptr const& ttask,   const WordLattice &input)
// , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(ttask, input)//
  // , maxNoTransOptPerCoverage, translationOptionThreshold)
{
  UTIL_THROW_IF2(StaticData::Instance().GetUseLegacyPT(),
                 "Not for models using the legqacy binary phrase table");

  size_t maxNoTransOptPerCoverage = ttask->options()->search.max_trans_opt_per_cov;
  float translationOptionThreshold = ttask->options()->search.trans_opt_threshold;
  const InputFeature *inputFeature = InputFeature::InstancePtr();
  UTIL_THROW_IF2(inputFeature == NULL, "Input feature must be specified");

  size_t maxPhraseLength = ttask->options()->search.max_phrase_length;  //StaticData::Instance().GetMaxPhraseLength();
  size_t size = input.GetSize();

  // 1-word phrases
  for (size_t startPos = 0; startPos < size; ++startPos) {

    const std::vector<size_t> &nextNodes = input.GetNextNodes(startPos);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    for (size_t i = 0; i < col.size(); ++i) {
      const Word &word = col[i].first;
      UTIL_THROW_IF2(word.IsEpsilon(), "Epsilon not supported");

      size_t nextNode = nextNodes[i];
      size_t endPos = startPos + nextNode - 1;

      Range range(startPos, endPos);

      if (range.GetNumWordsCovered() > maxPhraseLength) {
        continue;
      }

      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      Phrase subphrase;
      subphrase.AddWord(word);

      const ScorePair &scores = col[i].second;
      ScorePair *inputScore = new ScorePair(scores);

      InputPath *path
      = new InputPath(ttask.get(), subphrase, labels, range, NULL, inputScore);

      path->SetNextNode(nextNode);
      m_inputPathQueue.push_back(path);

      // recursive
      Extend(*path, input, ttask->options()->search.max_phrase_length);

    }
  }
}

void
TranslationOptionCollectionLattice::
Extend(const InputPath &prevPath, const WordLattice &input,
       size_t const maxPhraseLength)
{
  size_t nextPos = prevPath.GetWordsRange().GetEndPos() + 1;
  if (nextPos >= input.GetSize()) {
    return;
  }

  size_t startPos = prevPath.GetWordsRange().GetStartPos();
  const Phrase &prevPhrase = prevPath.GetPhrase();
  const ScorePair *prevInputScore = prevPath.GetInputScore();
  UTIL_THROW_IF2(prevInputScore == NULL,
                 "Null previous score");


  const std::vector<size_t> &nextNodes = input.GetNextNodes(nextPos);

  const ConfusionNet::Column &col = input.GetColumn(nextPos);
  for (size_t i = 0; i < col.size(); ++i) {
    const Word &word = col[i].first;
    UTIL_THROW_IF2(word.IsEpsilon(), "Epsilon not supported");

    size_t nextNode = nextNodes[i];
    size_t endPos = nextPos + nextNode - 1;

    Range range(startPos, endPos);

    // size_t maxPhraseLength = StaticData::Instance().GetMaxPhraseLength();
    if (range.GetNumWordsCovered() > maxPhraseLength) {
      continue;
    }

    const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

    Phrase subphrase(prevPhrase);
    subphrase.AddWord(word);

    const ScorePair &scores = col[i].second;
    ScorePair *inputScore = new ScorePair(*prevInputScore);
    inputScore->PlusEquals(scores);

    InputPath *path = new InputPath(prevPath.ttask, subphrase, labels,
                                    range, &prevPath, inputScore);

    path->SetNextNode(nextNode);
    m_inputPathQueue.push_back(path);

    // recursive
    Extend(*path, input, maxPhraseLength);

  }
}

void TranslationOptionCollectionLattice::CreateTranslationOptions()
{
  GetTargetPhraseCollectionBatch();

  VERBOSE(2,"Translation Option Collection\n " << *this << endl);
  const vector <DecodeGraph*> &decodeGraphs = StaticData::Instance().GetDecodeGraphs();
  UTIL_THROW_IF2(decodeGraphs.size() != 1, "Multiple decoder graphs not supported yet");
  const DecodeGraph &decodeGraph = *decodeGraphs[0];
  UTIL_THROW_IF2(decodeGraph.GetSize() != 1, "Factored decomposition not supported yet");

  const DecodeStep &decodeStep = **decodeGraph.begin();
  const PhraseDictionary &phraseDictionary = *decodeStep.GetPhraseDictionaryFeature();

  for (size_t i = 0; i < m_inputPathQueue.size(); ++i) {
    const InputPath &path = *m_inputPathQueue[i];

    TargetPhraseCollection::shared_ptr tpColl
    = path.GetTargetPhrases(phraseDictionary);
    const Range &range = path.GetWordsRange();

    if (tpColl && tpColl->GetSize()) {
      TargetPhraseCollection::const_iterator iter;
      for (iter = tpColl->begin(); iter != tpColl->end(); ++iter) {
        const TargetPhrase &tp = **iter;
        TranslationOption *transOpt = new TranslationOption(range, tp);
        transOpt->SetInputPath(path);
        transOpt->EvaluateWithSourceContext(m_source);

        Add(transOpt);
      }
    } else if (path.GetPhrase().GetSize() == 1) {
      // unknown word processing
      ProcessOneUnknownWord(path, path.GetWordsRange().GetStartPos(),  path.GetWordsRange().GetNumWordsCovered() , path.GetInputScore());
    }
  }

  // Prune
  Prune();

  Sort();

  // future score matrix
  CalcEstimatedScore();

  // Cached lex reodering costs
  CacheLexReordering();

}

void
TranslationOptionCollectionLattice::
ProcessUnknownWord(size_t sourcePos)
{
  UTIL_THROW(util::Exception, "ProcessUnknownWord() not implemented for lattice");
  // why??? UG
}

bool
TranslationOptionCollectionLattice::
CreateTranslationOptionsForRange
(const DecodeGraph &decodeStepList, size_t startPosition, size_t endPosition,
 bool adhereTableLimit, size_t graphInd)
{
  UTIL_THROW(util::Exception,
             "CreateTranslationOptionsForRange() not implemented for lattice");
}

} // namespace


