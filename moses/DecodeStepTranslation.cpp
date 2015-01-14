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
#include "util/exception.hh"

using namespace std;

namespace Moses
{
DecodeStepTranslation::DecodeStepTranslation(PhraseDictionary* pdf,
    const DecodeStep* prev,
    const std::vector<FeatureFunction*> &features)
  : DecodeStep(pdf, prev, features)
{
  // don't apply feature functions that are from current phrase table.It should already have been
  // dont by the phrase table.
  const std::vector<FeatureFunction*> &pdfFeatures = pdf->GetFeaturesToApply();
  for (size_t i = 0; i < pdfFeatures.size(); ++i) {
    FeatureFunction *ff = pdfFeatures[i];
    RemoveFeature(ff);
  }
}

void DecodeStepTranslation::Process(const TranslationOption &inputPartialTranslOpt
                                    , const DecodeStep &decodeStep
                                    , PartialTranslOptColl &outputPartialTranslOptColl
                                    , TranslationOptionCollection *toc
                                    , bool adhereTableLimit
                                    , const TargetPhraseCollection *phraseColl) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0) {
    // word deletion
    outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
    return;
  }

  // normal trans step
  const WordsRange &sourceWordsRange        = inputPartialTranslOpt.GetSourceWordsRange();
  const InputPath &inputPath = inputPartialTranslOpt.GetInputPath();
  const PhraseDictionary* phraseDictionary  =
    decodeStep.GetPhraseDictionaryFeature();
  const TargetPhrase &inPhrase = inputPartialTranslOpt.GetTargetPhrase();
  const size_t currSize = inPhrase.GetSize();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  if (phraseColl != NULL) {
    TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
    iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;

    for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != iterEnd; ++iterTargetPhrase) {
      const TargetPhrase& targetPhrase = **iterTargetPhrase;
      // const ScoreComponentCollection &transScores = targetPhrase.GetScoreBreakdown();
      // skip if the
      if (targetPhrase.GetSize() != currSize) continue;

      TargetPhrase outPhrase(inPhrase);

      if (IsFilteringStep()) {
        if (!inputPartialTranslOpt.IsCompatible(targetPhrase, m_conflictFactors))
          continue;
      }

      outPhrase.Merge(targetPhrase, m_newOutputFactors);
      outPhrase.EvaluateInIsolation(inputPath.GetPhrase(), m_featuresToApply); // need to do this as all non-transcores would be screwed up

      TranslationOption *newTransOpt = new TranslationOption(sourceWordsRange, outPhrase);
      assert(newTransOpt != NULL);

      newTransOpt->SetInputPath(inputPath);

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
  , size_t startPos, size_t endPos, bool adhereTableLimit
  , const InputPath &inputPath
  , const TargetPhraseCollection *phraseColl) const
{
  const PhraseDictionary* phraseDictionary = GetPhraseDictionaryFeature();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  const WordsRange wordsRange(startPos, endPos);

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

      transOpt->SetInputPath(inputPath);

      outputPartialTranslOptColl.Add (transOpt);

      VERBOSE(3,"\t" << targetPhrase << "\n");
    }
    VERBOSE(3,std::endl);
  }
}

void DecodeStepTranslation::ProcessInitialTranslationLEGACY(
  const InputType &source
  ,PartialTranslOptColl &outputPartialTranslOptColl
  , size_t startPos, size_t endPos, bool adhereTableLimit
  , const InputPathList &inputPathList) const
{
  const PhraseDictionary* phraseDictionary = GetPhraseDictionaryFeature();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  const WordsRange wordsRange(startPos, endPos);
  const TargetPhraseCollectionWithSourcePhrase *phraseColl =	phraseDictionary->GetTargetPhraseCollectionLEGACY(source,wordsRange);

  if (phraseColl != NULL) {
    IFVERBOSE(3) {
      if(StaticData::Instance().GetInputType() == SentenceInput)
        TRACE_ERR("[" << source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
      else
        TRACE_ERR("[" << startPos << "-" << endPos << "]" << std::endl);
    }

    const std::vector<Phrase> &sourcePhrases = phraseColl->GetSourcePhrases();

    TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
    std::vector<Phrase>::const_iterator iterSourcePhrase;
    iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;

    for (iterTargetPhrase = phraseColl->begin(), iterSourcePhrase =  sourcePhrases.begin()
         ; iterTargetPhrase != iterEnd
         ; ++iterTargetPhrase, ++iterSourcePhrase) {
      assert(iterSourcePhrase != sourcePhrases.end());

      const TargetPhrase	&targetPhrase = **iterTargetPhrase;
      const Phrase			&sourcePhrase = *iterSourcePhrase;

      const InputPath &inputPath = GetInputPathLEGACY(targetPhrase, sourcePhrase, inputPathList);

      TranslationOption *transOpt = new TranslationOption(wordsRange, targetPhrase);
      transOpt->SetInputPath(inputPath);

      outputPartialTranslOptColl.Add (transOpt);

      VERBOSE(3,"\t" << targetPhrase << "\n");
    }
    VERBOSE(3,std::endl);
  }
}

const InputPath &DecodeStepTranslation::GetInputPathLEGACY(
  const TargetPhrase targetPhrase,
  const Phrase sourcePhrase,
  const InputPathList &inputPathList) const
{
  const Word &wordFromPt =  sourcePhrase.GetWord(0);

  InputPathList::const_iterator iter;
  for (iter = inputPathList.begin(); iter != inputPathList.end(); ++iter) {
    const InputPath &inputPath = **iter;
    const Phrase &phraseFromIP = inputPath.GetPhrase();

    const Word *wordIP = NULL;
    for (size_t i = 0; i < phraseFromIP.GetSize(); ++i) {
      const Word &tempWord =  phraseFromIP.GetWord(i);
      if (!tempWord.IsEpsilon()) {
        wordIP = &tempWord;
        break;
      }
    }

    // const WordsRange &range = inputPath.GetWordsRange();

    if (wordIP && *wordIP == wordFromPt) {
      return inputPath;
    }
  }

  UTIL_THROW(util::Exception, "Input path not found");
}

void DecodeStepTranslation::ProcessLEGACY(const TranslationOption &inputPartialTranslOpt
    , const DecodeStep &decodeStep
    , PartialTranslOptColl &outputPartialTranslOptColl
    , TranslationOptionCollection *toc
    , bool adhereTableLimit) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0) {
    // word deletion
    outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));
    return;
  }

  // normal trans step
  const WordsRange &sourceWordsRange        = inputPartialTranslOpt.GetSourceWordsRange();
  const InputPath &inputPath = inputPartialTranslOpt.GetInputPath();
  const PhraseDictionary* phraseDictionary  =
    decodeStep.GetPhraseDictionaryFeature();
  const TargetPhrase &inPhrase = inputPartialTranslOpt.GetTargetPhrase();
  const size_t currSize = inPhrase.GetSize();
  const size_t tableLimit = phraseDictionary->GetTableLimit();

  const TargetPhraseCollectionWithSourcePhrase *phraseColl
    = phraseDictionary->GetTargetPhraseCollectionLEGACY(toc->GetSource(),sourceWordsRange);


  if (phraseColl != NULL) {
    TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
    iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;

    for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != iterEnd; ++iterTargetPhrase) {
      const TargetPhrase& targetPhrase = **iterTargetPhrase;
      // const ScoreComponentCollection &transScores = targetPhrase.GetScoreBreakdown();
      // skip if the
      if (targetPhrase.GetSize() != currSize) continue;

      TargetPhrase outPhrase(inPhrase);

      if (IsFilteringStep()) {
        if (!inputPartialTranslOpt.IsCompatible(targetPhrase, m_conflictFactors))
          continue;
      }

      outPhrase.Merge(targetPhrase, m_newOutputFactors);
      outPhrase.EvaluateInIsolation(inputPath.GetPhrase(), m_featuresToApply); // need to do this as all non-transcores would be screwed up


      TranslationOption *newTransOpt = new TranslationOption(sourceWordsRange, outPhrase);
      assert(newTransOpt != NULL);

      newTransOpt->SetInputPath(inputPath);

      outputPartialTranslOptColl.Add(newTransOpt );

    }
  } else if (sourceWordsRange.GetNumWordsCovered() == 1) {
    // unknown handler
    //toc->ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
  }
}
}



