// $Id$

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

#ifndef moses_TranslationOptionCollection_h
#define moses_TranslationOptionCollection_h

#include <list>
#include <boost/unordered_map.hpp>
#include "TypeDef.h"
#include "TranslationOption.h"
#include "TranslationOptionList.h"
#include "SquareMatrix.h"
#include "WordsBitmap.h"
#include "PartialTranslOptColl.h"
#include "DecodeStep.h"
#include "InputPath.h"

namespace Moses
{

class LanguageModel;
class FactorCollection;
class GenerationDictionary;
class InputType;
class FactorMask;
class Word;
class DecodeGraph;
class PhraseDictionary;
class InputPath;

/** Contains all phrase translations applicable to current input type (a sentence or confusion network).
 * A key insight into efficient decoding is that various input
 * conditions (trelliss, factored input, normal text, xml markup)
 * all lead to the same decoding algorithm: hypotheses are expanded
 * by applying phrase translations, which can be precomputed.
 *
 * The precomputation of a collection of instances of such TranslationOption
 * depends on the input condition, but they all are presented to
 * decoding algorithm in the same form, using this class.
 *
 * This is a abstract class, and cannot be instantiated directly. Instantiate 1 of the inherited
 * classes instead, for a particular input type
 **/
class TranslationOptionCollection
{
  friend std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll);
  TranslationOptionCollection(const TranslationOptionCollection&); /*< no copy constructor */
protected:
  std::vector< std::vector< TranslationOptionList > >	m_collection; /*< contains translation options */
  InputType const			&m_source; /*< reference to the input */
  SquareMatrix				m_futureScore; /*< matrix of future costs for contiguous parts (span) of the input */
  const size_t				m_maxNoTransOptPerCoverage; /*< maximum number of translation options per input span */
  const float				m_translationOptionThreshold; /*< threshold for translation options with regard to best option for input span */
  std::vector<const Phrase*> m_unksrcs;
  InputPathList m_inputPathQueue;

  TranslationOptionCollection(InputType const& src, size_t maxNoTransOptPerCoverage,
                              float translationOptionThreshold);

  void CalcFutureScore();

  //! Force a creation of a translation option where there are none for a particular source position.
  void ProcessUnknownWord();
  //! special handling of ONE unknown words.
  virtual void ProcessOneUnknownWord(const InputPath &inputPath, size_t sourcePos, size_t length = 1, const ScorePair *inputScores = NULL);

  //! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
  void Prune();

  //! sort all trans opt in each list for cube pruning */
  void Sort();

  //! list of trans opt for a particular span
  TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos);
  const TranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos) const;
  void Add(TranslationOption *translationOption);

  //! implemented by inherited class, called by this class
  virtual void ProcessUnknownWord(size_t sourcePos)=0;

  void EvaluateWithSource();

  void CacheLexReordering();

  void GetTargetPhraseCollectionBatch();

  void CreateTranslationOptionsForRange(
    const DecodeGraph &decodeGraph
    , size_t startPos
    , size_t endPos
    , bool adhereTableLimit
    , size_t graphInd
    , InputPath &inputPath);

  void SetInputScore(const InputPath &inputPath, PartialTranslOptColl &oldPtoc);

public:
  virtual ~TranslationOptionCollection();

  //! input sentence/confusion network
  const InputType& GetSource() const {
    return m_source;
  }

  //!List of unknowns (OOVs)
  const std::vector<const Phrase*>& GetUnknownSources() const {
    return m_unksrcs;
  }

  //! Create all possible translations from the phrase tables
  virtual void CreateTranslationOptions();
  //! Create translation options that exactly cover a specific input span.
  virtual void CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
      , size_t startPosition
      , size_t endPosition
      , bool adhereTableLimit
      , size_t graphInd) = 0;

  //!Check if this range has XML options
  virtual bool HasXmlOptionsOverlappingRange(size_t startPosition, size_t endPosition) const;

  //! Check if a subsumed XML option constraint is satisfied
  virtual bool ViolatesXmlOptionsConstraint(size_t startPosition, size_t endPosition, TranslationOption *transOpt) const;

  //! Create xml-based translation options for the specific input span
  virtual void CreateXmlOptionsForRange(size_t startPosition, size_t endPosition);


  //! returns future cost matrix for sentence
  inline virtual const SquareMatrix &GetFutureScore() const {
    return m_futureScore;
  }

  //! list of trans opt for a particular span
  const TranslationOptionList &GetTranslationOptionList(const WordsRange &coverage) const {
    return GetTranslationOptionList(coverage.GetStartPos(), coverage.GetEndPos());
  }

  TO_STRING();
};

}

#endif

