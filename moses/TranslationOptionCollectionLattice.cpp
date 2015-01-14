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

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionLattice::TranslationOptionCollectionLattice(
  const WordLattice &input
  , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  UTIL_THROW_IF2(StaticData::Instance().GetUseLegacyPT(),
                 "Not for models using the legqacy binary phrase table");

  const InputFeature &inputFeature = InputFeature::Instance();
  UTIL_THROW_IF2(&inputFeature == NULL, "Input feature must be specified");

  size_t maxPhraseLength = StaticData::Instance().GetMaxPhraseLength();
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

      WordsRange range(startPos, endPos);

      if (range.GetNumWordsCovered() > maxPhraseLength) {
        continue;
      }

      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      Phrase subphrase;
      subphrase.AddWord(word);

      const ScorePair &scores = col[i].second;
      ScorePair *inputScore = new ScorePair(scores);

      InputPath *path = new InputPath(subphrase, labels, range, NULL, inputScore);

      path->SetNextNode(nextNode);
      m_inputPathQueue.push_back(path);

      // recursive
      Extend(*path, input);

    }
  }
}

void TranslationOptionCollectionLattice::Extend(const InputPath &prevPath, const WordLattice &input)
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

    WordsRange range(startPos, endPos);

    size_t maxPhraseLength = StaticData::Instance().GetMaxPhraseLength();
    if (range.GetNumWordsCovered() > maxPhraseLength) {
      continue;
    }

    const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

    Phrase subphrase(prevPhrase);
    subphrase.AddWord(word);

    const ScorePair &scores = col[i].second;
    ScorePair *inputScore = new ScorePair(*prevInputScore);
    inputScore->PlusEquals(scores);

    InputPath *path = new InputPath(subphrase, labels, range, &prevPath, inputScore);

    path->SetNextNode(nextNode);
    m_inputPathQueue.push_back(path);

    // recursive
    Extend(*path, input);

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

    const TargetPhraseCollection *tpColl = path.GetTargetPhrases(phraseDictionary);
    const WordsRange &range = path.GetWordsRange();

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
  CalcFutureScore();

  // Cached lex reodering costs
  CacheLexReordering();

}

void TranslationOptionCollectionLattice::ProcessUnknownWord(size_t sourcePos)
{
  UTIL_THROW(util::Exception, "ProcessUnknownWord() not implemented for lattice");
}

void TranslationOptionCollectionLattice::CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
    , size_t startPosition
    , size_t endPosition
    , bool adhereTableLimit
    , size_t graphInd)
{
  UTIL_THROW(util::Exception, "CreateTranslationOptionsForRange() not implemented for lattice");
}

} // namespace


