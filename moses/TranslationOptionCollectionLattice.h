// $Id$
#pragma once

#include "TranslationOptionCollection.h"
#include "InputPath.h"

namespace Moses
{

class WordLattice;

/** Holds all translation options, for all spans, of a lattice input. NOT confusion networks
 * No legacy phrase-tables, CANNOT be used with Zen's binary phrase-table.
 */
class TranslationOptionCollectionLattice : public TranslationOptionCollection
{
protected:
  /* forcibly create translation option for a 1 word.
  	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
  	* at a particular source position
  */
  void ProcessUnknownWord(size_t sourcePos); // do not implement

public:
  TranslationOptionCollectionLattice(ttasksptr const& ttask, const WordLattice &source);
  // , size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

  void CreateTranslationOptions();

  bool
  CreateTranslationOptionsForRange
  (const DecodeGraph &decodeStepList, size_t startPosition, size_t endPosition,
   bool adhereTableLimit, size_t graphInd); // do not implement

protected:
  void Extend(const InputPath &prevPath, const WordLattice &input,
              size_t const maxPhraseLength);

};

}

