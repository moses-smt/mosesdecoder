// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <algorithm>
#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LM/Base.h"
#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "LexicalReordering.h"
#include "Util.h"
#include "StaticData.h"
#include "DecodeStepTranslation.h"
#include "DecodeGraph.h"

using namespace std;

namespace Moses
{
/** helper for pruning */
bool CompareTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
  return a->GetFutureScore() > b->GetFutureScore();
}

/** constructor; since translation options are indexed by coverage span, the corresponding data structure is initialized here
	* This fn should be called by inherited classes
*/
TranslationOptionCollection::TranslationOptionCollection(const TranslationSystem* system,
    InputType const& src, size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : m_system(system),
    m_source(src)
    ,m_futureScore(src.GetSize())
    ,m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
    ,m_translationOptionThreshold(translationOptionThreshold)
{
  // create 2-d vector
  size_t size = src.GetSize();
  for (size_t startPos = 0 ; startPos < size ; ++startPos) {
    m_collection.push_back( vector< TranslationOptionList >() );

    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = 0 ; endPos < maxSize ; ++endPos) {
      m_collection[startPos].push_back( TranslationOptionList() );
    }
  }
}

/** destructor, clears out data structures */
TranslationOptionCollection::~TranslationOptionCollection()
{
  RemoveAllInColl(m_unksrcs);
}

void TranslationOptionCollection::Prune()
{
  // quit, if max size, threshold
  if (m_maxNoTransOptPerCoverage == 0 && m_translationOptionThreshold == -std::numeric_limits<float>::infinity())
    return;

  // bookkeeping for how many options used, pruned
  size_t total = 0;
  size_t totalPruned = 0;

  // loop through all spans
  size_t size = m_source.GetSize();
  for (size_t startPos = 0 ; startPos < size; ++startPos) {
    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
      // consider list for a span
      TranslationOptionList &fullList = GetTranslationOptionList(startPos, endPos);
      total += fullList.size();

      // size pruning
      if (m_maxNoTransOptPerCoverage > 0 &&
          fullList.size() > m_maxNoTransOptPerCoverage) {
        // sort in vector
        nth_element(fullList.begin(), fullList.begin() + m_maxNoTransOptPerCoverage, fullList.end(), CompareTranslationOption);
        totalPruned += fullList.size() - m_maxNoTransOptPerCoverage;

        // delete the rest
        for (size_t i = m_maxNoTransOptPerCoverage ; i < fullList.size() ; ++i) {
          delete fullList.Get(i);
        }
        fullList.resize(m_maxNoTransOptPerCoverage);
      }

      // threshold pruning
      if (fullList.size() > 1 && m_translationOptionThreshold != -std::numeric_limits<float>::infinity()) {
        // first, find the best score
        float bestScore = -std::numeric_limits<float>::infinity();
        for (size_t i=0; i < fullList.size() ; ++i) {
          if (fullList.Get(i)->GetFutureScore() > bestScore)
            bestScore = fullList.Get(i)->GetFutureScore();
        }
        //std::cerr << "best score for span " << startPos << "-" << endPos << " is " << bestScore << "\n";
        // then, remove items that are worse than best score + threshold
        for (size_t i=0; i < fullList.size() ; ++i) {
          if (fullList.Get(i)->GetFutureScore() < bestScore + m_translationOptionThreshold) {
            //std::cerr << "\tremoving item " << i << ", score " << fullList.Get(i)->GetFutureScore() << ": " << fullList.Get(i)->GetTargetPhrase() << "\n";
            delete fullList.Get(i);
            fullList.Remove(i);
            total--;
            totalPruned++;
            i--;
          }
          //else
          //{
          //	std::cerr << "\tkeeping item " << i << ", score " << fullList.Get(i)->GetFutureScore() << ": " << fullList.Get(i)->GetTargetPhrase() << "\n";
          //}
        }
      } // end of threshold pruning
    }
  } // end of loop through all spans

  VERBOSE(2,"       Total translation options: " << total << std::endl
          << "Total translation options pruned: " << totalPruned << std::endl);
}

/** Force a creation of a translation option where there are none for a particular source position.
* ie. where a source word has not been translated, create a translation option by
*				1. not observing the table limits on phrase/generation tables
*				2. using the handler ProcessUnknownWord()
* Call this function once translation option collection has been filled with translation options
*
* This function calls for unknown words is complicated by the fact it must handle different input types.
* The call stack is
*		Base::ProcessUnknownWord()
*			Inherited::ProcessUnknownWord(position)
*				Base::ProcessOneUnknownWord()
*
* \param decodeStepList list of decoding steps
* \param factorCollection input sentence with all factors
*/

void TranslationOptionCollection::ProcessUnknownWord()
{
  const vector<DecodeGraph*>& decodeGraphList = m_system->GetDecodeGraphs();
  size_t size = m_source.GetSize();
  // try to translation for coverage with no trans by expanding table limit
  for (size_t graphInd = 0 ; graphInd < decodeGraphList.size() ; graphInd++) {
    const DecodeGraph &decodeGraph = *decodeGraphList[graphInd];
    for (size_t pos = 0 ; pos < size ; ++pos) {
      TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
      size_t numTransOpt = fullList.size();
      if (numTransOpt == 0) {
        CreateTranslationOptionsForRange(decodeGraph, pos, pos, false, graphInd);
      }
    }
  }

  bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
  // create unknown words for 1 word coverage where we don't have any trans options
  for (size_t pos = 0 ; pos < size ; ++pos) {
    TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
    if (fullList.size() == 0 || alwaysCreateDirectTranslationOption)
      ProcessUnknownWord(pos);
  }
}

/** special handling of ONE unknown words. Either add temporarily add word to translation table,
	* or drop the translation.
	* This function should be called by the ProcessOneUnknownWord() in the inherited class
	* At the moment, this unknown word handler is a bit of a hack, if copies over each factor from source
	* to target word, or uses the 'UNK' factor.
	* Ideally, this function should be in a class which can be expanded upon, for example,
	* to create a morphologically aware handler.
	*
	* \param sourceWord the unknown word
	* \param sourcePos
	* \param length length covered by this word (may be > 1 for lattice input)
	* \param inputScores a set of scores associated with unknown word (input scores from latties/CNs)
 */
void TranslationOptionCollection::ProcessOneUnknownWord(const Word &sourceWord,size_t sourcePos, size_t length, const Scores *inputScores)

{
	// unknown word, add as trans opt
	FactorCollection &factorCollection = FactorCollection::Instance();

	size_t isDigit = 0;
	
	const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
	const string &s = f->GetString();
	bool isEpsilon = (s=="" || s==EPSILON);
	if (StaticData::Instance().GetDropUnknown())
	{


		isDigit = s.find_first_of("0123456789");
		if (isDigit == 1) 
			isDigit = 1;
		else 
			isDigit = 0;
		// modify the starting bitmap
	}
	
	Phrase* m_unksrc = new Phrase(1);
  m_unksrc->AddWord() = sourceWord;
	m_unksrcs.push_back(m_unksrc);

	TranslationOption *transOpt;
	TargetPhrase targetPhrase;
	targetPhrase.SetSourcePhrase(*m_unksrc);
	if (inputScores != NULL) {
		targetPhrase.SetScore(m_system,*inputScores);
	} else {
		targetPhrase.SetScore(m_system);
	}
	
	if (!(StaticData::Instance().GetDropUnknown() || isEpsilon) || isDigit)
	{
		// add to dictionary

		Word &targetWord = targetPhrase.AddWord();
					
		for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			
			const Factor *sourceFactor = sourceWord[currFactor];
			if (sourceFactor == NULL)
				targetWord[factorType] = factorCollection.AddFactor(UNKNOWN_FACTOR);
			else
				targetWord[factorType] = factorCollection.AddFactor(sourceFactor->GetString());
		}
		//create a one-to-one alignment between UNKNOWN_FACTOR and its verbatim translation	
        
		targetPhrase.SetAlignmentInfo("0-0");
		
	}
	else 
	{ 
		// drop source word. create blank trans opt

		//targetPhrase.SetAlignment();

	}
	transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos + length - 1), targetPhrase, m_source
  , m_system->GetUnknownWordPenaltyProducer());	
	transOpt->CalcScore(m_system);
	Add(transOpt);


}

/** compute future score matrix in a dynamic programming fashion.
	* This matrix used in search.
	* Call this function once translation option collection has been filled with translation options
*/
void TranslationOptionCollection::CalcFutureScore()
{
  // setup the matrix (ignore lower triangle, set upper triangle to -inf
  size_t size = m_source.GetSize(); // the width of the matrix

  for(size_t row=0; row<size; row++) {
    for(size_t col=row; col<size; col++) {
      m_futureScore.SetScore(row, col, -numeric_limits<float>::infinity());
    }
  }

  // walk all the translation options and record the cheapest option for each span
  for (size_t startPos = 0 ; startPos < size ; ++startPos) {
    size_t maxSize = m_source.GetSize() - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
      TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);

      TranslationOptionList::const_iterator iterTransOpt;
      for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) {
        const TranslationOption &transOpt = **iterTransOpt;
        float score = transOpt.GetFutureScore();
        if (score > m_futureScore.GetScore(startPos, endPos))
          m_futureScore.SetScore(startPos, endPos, score);
      }
    }
  }

  // now fill all the cells in the strictly upper triangle
  //   there is no way to modify the diagonal now, in the case
  //   where no translation option covers a single-word span,
  //   we leave the +inf in the matrix
  // like in chart parsing we want each cell to contain the highest score
  // of the full-span trOpt or the sum of scores of joining two smaller spans

  for(size_t colstart = 1; colstart < size ; colstart++) {
    for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) {
      size_t startPos = diagshift;
      size_t endPos = colstart+diagshift;
      for(size_t joinAt = startPos; joinAt < endPos ; joinAt++)  {
        float joinedScore = m_futureScore.GetScore(startPos, joinAt)
                            + m_futureScore.GetScore(joinAt+1, endPos);
        /* // uncomment to see the cell filling scheme
        TRACE_ERR( "[" <<startPos<<","<<endPos<<"] <-? ["<<startPos<<","<<joinAt<<"]+["<<joinAt+1<<","<<endPos
          << "] (colstart: "<<colstart<<", diagshift: "<<diagshift<<")"<<endl);
        */
        if (joinedScore > m_futureScore.GetScore(startPos, endPos))
          m_futureScore.SetScore(startPos, endPos, joinedScore);
      }
    }
  }

  IFVERBOSE(3) {
    int total = 0;
    for(size_t row=0; row<size; row++) {
      size_t maxSize = size - row;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, maxSizePhrase);

      for(size_t col=row; col<row+maxSize; col++) {
        int count = GetTranslationOptionList(row, col).size();
        TRACE_ERR( "translation options spanning from  "
                   << row <<" to "<< col <<" is "
                   << count <<endl);
        total += count;
      }
    }
    TRACE_ERR( "translation options generated in total: "<< total << endl);

    for(size_t row=0; row<size; row++)
      for(size_t col=row; col<size; col++)
        TRACE_ERR( "future cost from "<< row <<" to "<< col <<" is "<< m_futureScore.GetScore(row, col) <<endl);
  }
}



/** Create all possible translations from the phrase tables
 * for a particular input sentence. This implies applying all
 * translation and generation steps. Also computes future cost matrix.
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 */
void TranslationOptionCollection::CreateTranslationOptions()
{
  // loop over all substrings of the source sentence, look them up
  // in the phraseDictionary (which is the- possibly filtered-- phrase
  // table loaded on initialization), generate TranslationOption objects
  // for all phrases

  // there may be multiple decoding graphs (factorizations of decoding)
  const vector <DecodeGraph*> &decodeGraphList = m_system->GetDecodeGraphs();
  const vector <size_t> &decodeGraphBackoff = m_system->GetDecodeGraphBackoff();

  // length of the sentence
  size_t size = m_source.GetSize();

  // loop over all decoding graphs, each generates translation options
  for (size_t graphInd = 0 ; graphInd < decodeGraphList.size() ; graphInd++) {
    if (decodeGraphList.size() > 1) {
      VERBOSE(3,"Creating translation options from decoding graph " << graphInd << endl);
    }

    const DecodeGraph &decodeGraph = *decodeGraphList[graphInd];
    // generate phrases that start at startPos ...
    for (size_t startPos = 0 ; startPos < size; startPos++) {
      size_t maxSize = size - startPos; // don't go over end of sentence
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, maxSizePhrase);

      // ... and that end at endPos
      for (size_t endPos = startPos ; endPos < startPos + maxSize ; endPos++) {
        if (graphInd > 0 && // only skip subsequent graphs
            decodeGraphBackoff[graphInd] != 0 && // use of backoff specified
            (endPos-startPos+1 >= decodeGraphBackoff[graphInd] || // size exceeds backoff limit or ...
             m_collection[startPos][endPos-startPos].size() > 0)) { // no phrases found so far
          VERBOSE(3,"No backoff to graph " << graphInd << " for span [" << startPos << ";" << endPos << "]" << endl);
          // do not create more options
          continue;
        }

        // create translation options for that range
        CreateTranslationOptionsForRange( decodeGraph, startPos, endPos, true, graphInd);
      }
    }
  }

  VERBOSE(2,"Translation Option Collection\n " << *this << endl);

  // Incorporate distributed lm scores.
  IncorporateDLMScores();

  ProcessUnknownWord();

  // Prune
  Prune();

  Sort();

  // future score matrix
  CalcFutureScore();

  // Cached lex reodering costs
  CacheLexReordering();

  // stateless feature scores
  PreCalculateScores();
}

void TranslationOptionCollection::IncorporateDLMScores() {
    // Build list of dlms.
    const vector<const StatefulFeatureFunction*>& ffs =
           m_system->GetStatefulFeatureFunctions();
    std::map<int, LanguageModel*> dlm_ffs;
    for (unsigned i = 0; i < ffs.size(); ++i) {
        if (ffs[i]->GetScoreProducerDescription() == "DLM_5gram") {
            dlm_ffs[i] = const_cast<LanguageModel*>(static_cast<const LanguageModel* const>(ffs[i]));
            dlm_ffs[i]->SetFFStateIdx(i);
        }
    }

    // Don't need to do anything if we don't have any distributed
    // language models.
    if (dlm_ffs.size() == 0) {
        return;
    }

    // Iterate over all translation options in the collection.
    std::vector< std::vector< TranslationOptionList > >::iterator start_iter; 
    for (start_iter = m_collection.begin();
         start_iter != m_collection.end();
         ++start_iter) {
        std::vector< TranslationOptionList >::iterator end_iter;
        for (end_iter = (*start_iter).begin();
             end_iter != (*start_iter).end();
             ++end_iter) {
            std::vector< TranslationOption* >::iterator option_iter;
            for (option_iter = (*end_iter).begin();
                 option_iter != (*end_iter).end();
                 ++option_iter) {

                // Get a handle on the current translation option.
                TranslationOption* option = *option_iter;

                std::map<int, LanguageModel*>::iterator dlm_iter;
                for (dlm_iter = dlm_ffs.begin();
                     dlm_iter != dlm_ffs.end();
                     ++dlm_iter) {
                    LanguageModel* dlm = (*dlm_iter).second;


                    float full_score;
                    float ngram_score;
                    size_t oov_count;
                    TargetPhrase& phrase =
                        const_cast<TargetPhrase&>(option->GetTargetPhrase());
                    dlm->CalcScoreFromCache(phrase,
                                            full_score,
                                            ngram_score,
                                            oov_count);
                    ScoreComponentCollection& option_scores = 
                        const_cast<ScoreComponentCollection&>(option->GetScoreBreakdown());
                    option_scores.Assign(dlm, ngram_score);
                    ScoreComponentCollection& phrase_scores =
                        const_cast<ScoreComponentCollection&>(phrase.GetScoreBreakdown());
                    phrase_scores.Assign(dlm, ngram_score);

                    float weighted_score = full_score * dlm->GetWeight();
                    phrase.SetFutureScore(phrase.GetFutureScore() + weighted_score);
                }
            }
        }
    }
}

void TranslationOptionCollection::Sort()
{
  size_t size = m_source.GetSize();
  for (size_t startPos = 0 ; startPos < size; ++startPos) {
    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = startPos ; endPos < startPos + maxSize; ++endPos) {
      TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);
      std::sort(transOptList.begin(), transOptList.end(), CompareTranslationOption);
    }
  }
}


/** create translation options that exactly cover a specific input span.
 * Called by CreateTranslationOptions() and ProcessUnknownWord()
 * \param decodeGraph list of decoding steps
 * \param factorCollection input sentence with all factors
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence
 * \param adhereTableLimit whether phrase & generation table limits are adhered to
 */
void TranslationOptionCollection::CreateTranslationOptionsForRange(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos)) {
    Phrase *sourcePhrase = NULL; // can't initialise with substring, in case it's confusion network

    // consult persistent (cross-sentence) cache for stored translation options
    bool skipTransOptCreation = false
                                , useCache = StaticData::Instance().GetUseTransOptCache();
    if (useCache) {
      const WordsRange wordsRange(startPos, endPos);
      sourcePhrase = new Phrase(m_source.GetSubString(wordsRange));

      const TranslationOptionList *transOptList = StaticData::Instance().FindTransOptListInCache(decodeGraph, *sourcePhrase);
      // is phrase in cache?
      if (transOptList != NULL) {
        skipTransOptCreation = true;
        TranslationOptionList::const_iterator iterTransOpt;
        for (iterTransOpt = transOptList->begin() ; iterTransOpt != transOptList->end() ; ++iterTransOpt) {
          TranslationOption *transOpt = new TranslationOption(**iterTransOpt, wordsRange);
          Add(transOpt);
        }
      }
    } // useCache

    if (!skipTransOptCreation) {
      // partial trans opt stored in here
      PartialTranslOptColl* oldPtoc = new PartialTranslOptColl;
      size_t totalEarlyPruned = 0;

      // initial translation step
      list <const DecodeStep* >::const_iterator iterStep = decodeGraph.begin();
      const DecodeStep &decodeStep = **iterStep;

      static_cast<const DecodeStepTranslation&>(decodeStep).ProcessInitialTranslation
      (m_system, m_source, *oldPtoc
       , startPos, endPos, adhereTableLimit );

      // do rest of decode steps
      int indexStep = 0;
      for (++iterStep ; iterStep != decodeGraph.end() ; ++iterStep) {
        const DecodeStep &decodeStep = **iterStep;
        PartialTranslOptColl* newPtoc = new PartialTranslOptColl;

        // go thru each intermediate trans opt just created
        const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
        vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
        for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt) {
          TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;
          decodeStep.Process(m_system, inputPartialTranslOpt
                             , decodeStep
                             , *newPtoc
                             , this
                             , adhereTableLimit);
        }
        // last but 1 partial trans not required anymore
        totalEarlyPruned += newPtoc->GetPrunedCount();
        delete oldPtoc;
        oldPtoc = newPtoc;
        indexStep++;
      } // for (++iterStep

      // add to fully formed translation option list
      PartialTranslOptColl &lastPartialTranslOptColl	= *oldPtoc;
      const vector<TranslationOption*>& partTransOptList = lastPartialTranslOptColl.GetList();
      vector<TranslationOption*>::const_iterator iterColl;
      for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl) {
        TranslationOption *transOpt = *iterColl;
        transOpt->CalcScore(m_system);
        Add(transOpt);
      }

      // storing translation options in persistent cache (kept across sentences)
      if (useCache) {
        if (partTransOptList.size() > 0) {
          TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);
          StaticData::Instance().AddTransOptListToCache(decodeGraph, *sourcePhrase, transOptList);
        }
      }

      lastPartialTranslOptColl.DetachAll();
      totalEarlyPruned += oldPtoc->GetPrunedCount();
      delete oldPtoc;
      // TRACE_ERR( "Early translation options pruned: " << totalEarlyPruned << endl);
    } // if (!skipTransOptCreation)

    if (useCache)
      delete sourcePhrase;
  } // if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos))

  if (graphInd == 0 && StaticData::Instance().GetXmlInputType() != XmlPassThrough && HasXmlOptionsOverlappingRange(startPos,endPos)) {
    CreateXmlOptionsForRange(startPos, endPos);
  }
}

/** Check if this range overlaps with any XML options. This doesn't need to be an exact match, only an overlap.
 * by default, we don't support XML options. subclasses need to override this function.
 * called by CreateTranslationOptionsForRange()
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence
 * \param adhereTableLimit whether phrase & generation table limits are adhered to
 */
bool TranslationOptionCollection::HasXmlOptionsOverlappingRange(size_t, size_t) const
{
  return false;
  //not implemented for base class
}

/** Populates the current Collection with XML options exactly covering the range specified. Default implementation does nothing.
 * called by CreateTranslationOptionsForRange()
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence
 */
void TranslationOptionCollection::CreateXmlOptionsForRange(size_t, size_t)
{
  //not implemented for base class
};




/** add translation option to the list
 * \param translationOption translation option to be added */
void TranslationOptionCollection::Add(TranslationOption *translationOption)
{
  const WordsRange &coverage = translationOption->GetSourceWordsRange();
  CHECK(coverage.GetEndPos() - coverage.GetStartPos() < m_collection[coverage.GetStartPos()].size());
  m_collection[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()].Add(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
  size_t size = coll.GetSize();
  for (size_t startPos = 0 ; startPos < size ; ++startPos) {
    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
      const TranslationOptionList& fullList = coll.GetTranslationOptionList(startPos, endPos);
      size_t sizeFull = fullList.size();
      for (size_t i = 0; i < sizeFull; i++) {
        out << *fullList.Get(i) << std::endl;
      }
    }
  }

  //std::vector< std::vector< TranslationOptionList > >::const_iterator i = coll.m_collection.begin();
  //size_t j = 0;
  //for (; i!=coll.m_collection.end(); ++i) {
  //out << "s[" << j++ << "].size=" << i->size() << std::endl;
  //}

  return out;
}

const std::vector<Phrase*>& TranslationOptionCollection::GetUnknownSources() const 
{
  return m_unksrcs;
}

void TranslationOptionCollection::CacheLexReordering()
{
  const vector<LexicalReordering*> &lexReorderingModels = m_system->GetReorderModels();
  std::vector<LexicalReordering*>::const_iterator iterLexreordering;

  size_t size = m_source.GetSize();
  for (iterLexreordering = lexReorderingModels.begin() ; iterLexreordering != lexReorderingModels.end() ; ++iterLexreordering) {
    LexicalReordering &lexreordering = **iterLexreordering;

    for (size_t startPos = 0 ; startPos < size ; startPos++) {
      size_t maxSize =  size - startPos;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, maxSizePhrase);

      for (size_t endPos = startPos ; endPos < startPos + maxSize; endPos++) {
        TranslationOptionList &transOptList = GetTranslationOptionList( startPos, endPos);
        TranslationOptionList::iterator iterTransOpt;
        for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) {
          TranslationOption &transOpt = **iterTransOpt;
          //Phrase sourcePhrase =  m_source.GetSubString(WordsRange(startPos,endPos));
          const Phrase *sourcePhrase = transOpt.GetSourcePhrase();
          if (sourcePhrase) {
            Scores score = lexreordering.GetProb(*sourcePhrase
                                                 , transOpt.GetTargetPhrase());
            if (!score.empty())
              transOpt.CacheScores(lexreordering, score);
          }
        }
      }
    }
  }
}

void TranslationOptionCollection::PreCalculateScores() 
{
  //Figure out which features need to be precalculated
  const vector<const StatelessFeatureFunction*>& sfs =
    m_system->GetStatelessFeatureFunctions();
  vector<const StatelessFeatureFunction*> precomputedFeatures;
  for (unsigned i = 0; i < sfs.size(); ++i) {
    if (sfs[i]->ComputeValueInTranslationOption() && 
        !sfs[i]->ComputeValueInTranslationTable()) {
      precomputedFeatures.push_back(sfs[i]);
    }
  }
  //empty coverage vector
  WordsBitmap coverage(m_source.GetSize());

  if (precomputedFeatures.size()) {
    //Go through translation options and precompute features
    for (size_t i = 0; i < m_collection.size(); ++i) {
      for (size_t j = 0; j < m_collection[i].size(); ++j) {
        for (size_t k = 0; k < m_collection[i][j].size(); ++k) {
          const TranslationOption* toption =  m_collection[i][j].Get(k);
          ScoreComponentCollection& breakdown = m_precalculatedScores[*toption];
          PhraseBasedFeatureContext context(*toption, m_source);
          for (size_t si = 0; si < precomputedFeatures.size(); ++si) {
            precomputedFeatures[si]->Evaluate(context, &breakdown);
          }
        }
      }
    }
  }
}

void TranslationOptionCollection::InsertPreCalculatedScores
  (const TranslationOption& translationOption, ScoreComponentCollection* scoreBreakdown) 
    const
{
  if (m_precalculatedScores.size()) {
    boost::unordered_map<TranslationOption,ScoreComponentCollection>::const_iterator scoreIter = 
      m_precalculatedScores.find(translationOption);
    if (scoreIter != m_precalculatedScores.end()) {
      scoreBreakdown->PlusEquals(scoreIter->second);
    } else {
      TRACE_ERR("ERROR: " << translationOption << " missing from precalculation cache" << endl);
      assert(0);  
    }
  }
}

//! list of trans opt for a particular span
TranslationOptionList &TranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos)
{
  size_t maxSize = endPos - startPos;
  size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
  maxSize = std::min(maxSize, maxSizePhrase);

  CHECK(maxSize < m_collection[startPos].size());
  return m_collection[startPos][maxSize];
}
const TranslationOptionList &TranslationOptionCollection::GetTranslationOptionList(size_t startPos, size_t endPos) const
{
  size_t maxSize = endPos - startPos;
  size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
  maxSize = std::min(maxSize, maxSizePhrase);

  CHECK(maxSize < m_collection[startPos].size());
  return m_collection[startPos][maxSize];
}

}

