#ifndef moses_SearchCubePruning_h
#define moses_SearchCubePruning_h

#include <vector>
#include "Search.h"
#include "HypothesisStackCubePruning.h"
#include "SentenceStats.h"

namespace Moses
{

class InputType;
class TranslationOptionCollection;

/** Functions and variables you need to decoder an input using the phrase-based decoder with cube-pruning
 *  Instantiated by the Manager class
 */
class SearchCubePruning: public Search
{
protected:
  const InputType &m_source;
  std::vector < HypothesisStack* > m_hypoStackColl; /**< stacks to store hypotheses (partial translations) */
  // no of elements = no of words in source + 1
  clock_t m_start; /**< used to track time spend on translation */
  const TranslationOptionCollection &m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */

  //! go thru all bitmaps in 1 stack & create backpointers to bitmaps in the stack
  void CreateForwardTodos(HypothesisStackCubePruning &stack);
  //! create a back pointer to this bitmap, with edge that has this words range translation
  void CreateForwardTodos(const WordsBitmap &bitmap, const WordsRange &range, BitmapContainer &bitmapContainer);
  bool CheckDistortion(const WordsBitmap &bitmap, const WordsRange &range) const;

  void PrintBitmapContainerGraph();

public:
  SearchCubePruning(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl);
  ~SearchCubePruning();

  void ProcessSentence();

  void OutputHypoStackSize();
  void OutputHypoStack(int stack);

  virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
  virtual const Hypothesis *GetBestHypothesis() const;
};


}
#endif
