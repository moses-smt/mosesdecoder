#include "DecodeStep_Translation.h"
#include "PhraseDictionary.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "FactorCollection.h"

TranslationDecodeStep::TranslationDecodeStep(PhraseDictionaryBase* dict, const DecodeStep* prev)
: DecodeStep(dict, prev)
{
}

const PhraseDictionaryBase &TranslationDecodeStep::GetPhraseDictionary() const
{
  return *static_cast<const PhraseDictionaryBase*>(m_ptr);
}

TranslationOption *TranslationDecodeStep::MergeTranslation(const TranslationOption& oldTO, const TargetPhrase &targetPhrase) const
{
  if (IsFilteringStep()) {
    if (!oldTO.IsCompatible(targetPhrase, m_conflictFactors)) return 0;
  }

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeNewFeatures(targetPhrase, targetPhrase.GetScoreBreakdown(), m_newOutputFactors);
  return newTransOpt;
}


void TranslationDecodeStep::Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
                              , TranslationOptionCollection *toc) const
{
  //TRACE_ERR(inputPartialTranslOpt << endl);
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
    { // word deletion

      outputPartialTranslOptColl.Add(new TranslationOption(inputPartialTranslOpt));

      return;
    }

  // normal trans step
  const WordsRange &sourceWordsRange        = inputPartialTranslOpt.GetSourceWordsRange();
  const PhraseDictionaryBase &phraseDictionary  = decodeStep.GetPhraseDictionary();
  const TargetPhraseCollection *phraseColl= phraseDictionary.GetTargetPhraseCollection(toc->GetSource(),sourceWordsRange);
	const size_t currSize = inputPartialTranslOpt.GetTargetPhrase().GetSize();

  if (phraseColl != NULL)
    {
      TargetPhraseCollection::const_iterator iterTargetPhrase;

      for (iterTargetPhrase = phraseColl->begin(); iterTargetPhrase != phraseColl->end(); ++iterTargetPhrase)
        {
          const TargetPhrase& targetPhrase = *iterTargetPhrase;
					// skip if the 
					if (targetPhrase.GetSize() != currSize) continue;

          TranslationOption *newTransOpt = MergeTranslation(inputPartialTranslOpt, targetPhrase);
          if (newTransOpt != NULL)
            {
              outputPartialTranslOptColl.Add( newTransOpt );
            }
        }
    }
  else if (sourceWordsRange.GetWordsCount() == 1)
    { // unknown handler
      //toc->ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
    }
}

