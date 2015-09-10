#ifndef moses_Search_h
#define moses_Search_h

#include <vector>
#include "TypeDef.h"
#include "TranslationOption.h"
#include "Phrase.h"
#include "InputPath.h"

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
  virtual void Decode() = 0;

  explicit Search(Manager& manager);
  virtual ~Search() {}

  // Factory method
  static Search *CreateSearch(Manager& manager, const InputType &source, SearchAlgorithm searchAlgorithm,
                              const TranslationOptionCollection &transOptColl);

protected:
  Manager& m_manager;
  InputPath m_inputPath; // for initial hypo
  TranslationOption m_initialTransOpt; /**< used to seed 1st hypo */
  AllOptions const& m_options;

  /** flag indicating that decoder ran out of time (see switch -time-out) */
  size_t interrupted_flag;

  bool out_of_time();
};

}
#endif
