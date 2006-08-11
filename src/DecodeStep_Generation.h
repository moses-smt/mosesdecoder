#ifndef _Generation_DECODE_STEP_H_
#define _Generation_DECODE_STEP_H_

#include "DecodeStep.h"

class GenerationDictionary;
class Phrase;
class ScoreComponentCollection2;

class GenerationDecodeStep : public DecodeStep
{
public:
	GenerationDecodeStep(GenerationDictionary* dict, const DecodeStep* prev);

  /** returns phrase table (dictionary) for translation step */
  const GenerationDictionary &GetGenerationDictionary() const;

  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
                              , TranslationOptionCollection *toc) const;

private:
  TranslationOption *MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection2& generationScore) const;

};

#endif
