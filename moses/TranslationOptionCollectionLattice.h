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
  void ProcessUnknownWord();
  void ProcessUnknownWord(size_t sourcePos);

public:
  TranslationOptionCollectionLattice(const WordLattice &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

  void CreateTranslationOptions();

  void CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
      , size_t startPosition
      , size_t endPosition
      , bool adhereTableLimit
      , size_t graphInd);

protected:

};

}

