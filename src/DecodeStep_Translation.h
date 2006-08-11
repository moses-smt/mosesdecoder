#ifndef _TRANSLATION_DECODE_STEP_H_
#define _TRANSLATION_DECODE_STEP_H_

#include "DecodeStep.h"

class PhraseDictionaryBase;
class TargetPhrase;

class TranslationDecodeStep : public DecodeStep
{
public:
	TranslationDecodeStep(PhraseDictionaryBase* dict, const DecodeStep* prev);

  /** returns phrase table (dictionary) for translation step */
  const PhraseDictionaryBase &GetPhraseDictionary() const;

  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
                              , TranslationOptionCollection *toc) const;
private:
	TranslationOption *MergeTranslation(const TranslationOption& oldTO, TargetPhrase &targetPhrase) const;
};

#endif
