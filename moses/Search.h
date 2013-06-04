#ifndef moses_Search_h
#define moses_Search_h

#include <vector>
#include "TypeDef.h"

namespace Moses
{

class HypothesisStack;
class Hypothesis;
class InputType;
class TranslationOptionCollection;
class Manager;
class Phrase;

/** Base search class used in the phrase-based decoder.
 *
 * Actual search class that implement the cube pruning algorithm (SearchCubePruning)
 * or standard beam search (SearchNormal) should inherits from this class, and
 * override pure virtual functions.
 */
class Search
{
public:
  virtual const std::vector<HypothesisStack*>& GetHypothesisStacks() const = 0;
  virtual const Hypothesis *GetBestHypothesis() const = 0;

  //! Decode the sentence according to the specified search algorithm.
  virtual void ProcessSentence() = 0;

  explicit Search(Manager& manager) : m_manager(manager) {}
  virtual ~Search() {}

  // Factory method
  static Search *CreateSearch(Manager& manager, const InputType &source, SearchAlgorithm searchAlgorithm,
                              const TranslationOptionCollection &transOptColl);

protected:
  const Phrase *m_constraint;
  Manager& m_manager;
};

}
#endif
