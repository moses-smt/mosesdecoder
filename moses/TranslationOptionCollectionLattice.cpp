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

  const InputFeature *inputFeature = StaticData::Instance().GetInputFeature();
  UTIL_THROW_IF2(inputFeature == NULL,
		  "Input feature must be specified");

  size_t maxPhraseLength = StaticData::Instance().GetMaxPhraseLength();
  size_t size = input.GetSize();

  // 1-word phrases
  for (size_t startPos = 0; startPos < size; ++startPos) {

    const std::vector<size_t> &nextNodes = input.GetNextNodes(startPos);

    WordsRange range(startPos, startPos);
    const NonTerminalSet &labels = input.GetLabelSet(startPos, startPos);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    for (size_t i = 0; i < col.size(); ++i) {
      const Word &word = col[i].first;
      UTIL_THROW_IF2(word.IsEpsilon(), "Epsilon not supported");

      Phrase subphrase;
      subphrase.AddWord(word);

      const ScorePair &scores = col[i].second;
      ScorePair *inputScore = new ScorePair(scores);

      InputPath *path = new InputPath(subphrase, labels, range, NULL, inputScore);

      size_t nextNode = nextNodes[i];
      path->SetNextNode(nextNode);

      m_inputPathQueue.push_back(path);
    }
  }

  // iteratively extend all paths
    for (size_t endPos = 1; endPos < size; ++endPos) {
      const std::vector<size_t> &nextNodes = input.GetNextNodes(endPos);

      // loop thru every previous paths
      size_t numPrevPaths = m_inputPathQueue.size();

      for (size_t i = 0; i < numPrevPaths; ++i) {
        //for (size_t pathInd = 0; pathInd < prevPaths.size(); ++pathInd) {
        const InputPath &prevPath = *m_inputPathQueue[i];

        size_t nextNode = prevPath.GetNextNode();
        if (prevPath.GetWordsRange().GetEndPos() + nextNode != endPos) {
        	continue;
        }

        size_t startPos = prevPath.GetWordsRange().GetStartPos();

        if (endPos - startPos + 1 > maxPhraseLength) {
        	continue;
        }

        WordsRange range(startPos, endPos);
        const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

        const Phrase &prevPhrase = prevPath.GetPhrase();
        const ScorePair *prevInputScore = prevPath.GetInputScore();
        UTIL_THROW_IF2(prevInputScore == NULL,
        		"Null previous score");

        // loop thru every word at this position
        const ConfusionNet::Column &col = input.GetColumn(endPos);

        for (size_t i = 0; i < col.size(); ++i) {
          const Word &word = col[i].first;
          Phrase subphrase(prevPhrase);
          subphrase.AddWord(word);

          const ScorePair &scores = col[i].second;
          ScorePair *inputScore = new ScorePair(*prevInputScore);
          inputScore->PlusEquals(scores);

          InputPath *path = new InputPath(subphrase, labels, range, &prevPath, inputScore);

          size_t nextNode = nextNodes[i];
          path->SetNextNode(nextNode);

          m_inputPathQueue.push_back(path);
        } // for (size_t i = 0; i < col.size(); ++i) {

      } // for (size_t i = 0; i < numPrevPaths; ++i) {
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

    if (tpColl) {
    	TargetPhraseCollection::const_iterator iter;
    	for (iter = tpColl->begin(); iter != tpColl->end(); ++iter) {
    		const TargetPhrase &tp = **iter;
    		TranslationOption *transOpt = new TranslationOption(range, tp);
    		transOpt->SetInputPath(path);
    		transOpt->Evaluate(m_source);

    		Add(transOpt);
    	}
    }
    else if (path.GetPhrase().GetSize() == 1) {
    	// unknown word processing
    	ProcessOneUnknownWord(path, path.GetWordsRange().GetEndPos(), 1, path.GetInputScore());
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


