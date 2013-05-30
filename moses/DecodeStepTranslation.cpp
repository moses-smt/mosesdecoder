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

#include "DecodeStepTranslation.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "FactorCollection.h"

using namespace std;

namespace Moses
{
DecodeStepTranslation::DecodeStepTranslation(const PhraseDictionary* pdf,
											const DecodeStep* prev,
											const std::vector<FeatureFunction*> &features)
: DecodeStep(pdf, prev, features)
{
}

void DecodeStepTranslation::Process(const TranslationOption &inputPartialTranslOpt
                                    , const DecodeStep &decodeStep
                                    , PartialTranslOptColl &outputPartialTranslOptColl
                                    , TranslationOptionCollection *toc
                                    , bool adhereTableLimit
                                    , const Phrase &src) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0) {
    // word deletion
    outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
    return;
  }

  // normal trans step
  const WordsRange &sourceWordsRange        = inputPartialTranslOpt.GetSourceWordsRange();
  const PhraseDictionary* phraseDictionary  =
    decodeStep.GetPhraseDictionaryFeature();
  const TargetPhrase &inPhrase = inputPartialTranslOpt.GetTargetPhrase();
  const size_t currSize = inPhrase.GetSize();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  const TargetPhraseCollection *phraseColl=
    phraseDictionary->GetTargetPhraseCollection(toc->GetSource(),sourceWordsRange);

  if (phraseColl != NULL) {
    TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
    iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;

    for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != iterEnd; ++iterTargetPhrase) {
      const TargetPhrase& targetPhrase = **iterTargetPhrase;
      const ScoreComponentCollection &transScores = targetPhrase.GetScoreBreakdown();
      // skip if the
      if (targetPhrase.GetSize() != currSize) continue;

      TargetPhrase outPhrase(inPhrase);

      if (IsFilteringStep()) {
        if (!inputPartialTranslOpt.IsCompatible(targetPhrase, m_conflictFactors))
          continue;
      }

      outPhrase.GetScoreBreakdown().PlusEquals(transScores);
      outPhrase.Evaluate(src); // need to do this as all non-transcores would be screwed up

      outPhrase.MergeFactors(targetPhrase, m_newOutputFactors);

      TranslationOption *newTransOpt = new TranslationOption(sourceWordsRange, outPhrase);
      assert(newTransOpt != NULL);

      outputPartialTranslOptColl.Add(newTransOpt );

    }
  } else if (sourceWordsRange.GetNumWordsCovered() == 1) {
    // unknown handler
    //toc->ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
  }
}


void DecodeStepTranslation::ProcessInitialTranslation(
  const InputType &source
  ,PartialTranslOptColl &outputPartialTranslOptColl
  , size_t startPos, size_t endPos, bool adhereTableLimit) const
{
  const PhraseDictionary* phraseDictionary = GetPhraseDictionaryFeature();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  const WordsRange wordsRange(startPos, endPos);
  const TargetPhraseCollection *phraseColl =	phraseDictionary->GetTargetPhraseCollection(source,wordsRange);

  if (phraseColl != NULL) {
    IFVERBOSE(3) {
      if(StaticData::Instance().GetInputType() == SentenceInput)
        TRACE_ERR("[" << source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
      else
        TRACE_ERR("[" << startPos << "-" << endPos << "]" << std::endl);
    }

    TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
    iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;

    for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != iterEnd ; ++iterTargetPhrase) {
      const TargetPhrase	&targetPhrase = **iterTargetPhrase;
      TranslationOption *transOpt = new TranslationOption(wordsRange, targetPhrase);

      outputPartialTranslOptColl.Add (transOpt);

      VERBOSE(3,"\t" << targetPhrase << "\n");
    }
    VERBOSE(3,std::endl);
  }
}

}



