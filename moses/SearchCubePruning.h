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

class SearchCubePruning;

class FunctorCube {
  public:
    FunctorCube(SearchCubePruning* search) : m_search(search) {}
    
    virtual void operator()(const WordsBitmap &bitmap,
                            const WordsRange &range,
                            BitmapContainer &bitmapContainer) = 0;
    
    virtual bool IsCollector() = 0;
    
  protected:
    SearchCubePruning* m_search;
};

class ExpanderCube : public FunctorCube {
  public:
    ExpanderCube(SearchCubePruning* search) : FunctorCube(search) {}
    virtual void operator()(const WordsBitmap &bitmap,
                            const WordsRange &range,
                            BitmapContainer &bitmapContainer);
    
    virtual bool IsCollector() { return false; }
};

class CollectorCube : public FunctorCube, public Collector {
  public:
    CollectorCube(SearchCubePruning* search) : FunctorCube(search) {}
    virtual void operator()(const WordsBitmap &bitmap,
                            const WordsRange &range,
                            BitmapContainer &bitmapContainer);

    virtual bool IsCollector() { return true; }
                            
    std::vector<const Hypothesis*> GetHypotheses() {
      return m_hypotheses;                   
    }
    
    std::vector<const TranslationOptionList*>& GetOptions(int hypId) {
      return m_options[hypId];
    }
    
  private:
    std::vector<const Hypothesis*> m_hypotheses;
    std::map<size_t, std::vector<const TranslationOptionList*> > m_options;
};

/** Functions and variables you need to decoder an input using the phrase-based decoder with cube-pruning
 *  Instantiated by the Manager class
 */
class SearchCubePruning: public Search
{
protected:
  friend ExpanderCube;
  friend CollectorCube;

  const InputType &m_source;
  std::vector < HypothesisStack* > m_hypoStackColl; /**< stacks to store hypotheses (partial translations) */
  // no of elements = no of words in source + 1
  const TranslationOptionCollection &m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */

  //! go thru all bitmaps in 1 stack & create backpointers to bitmaps in the stack
  void CreateForwardTodos(HypothesisStackCubePruning &stack, FunctorCube* functor);
  //! create a back pointer to this bitmap, with edge that has this words range translation
  void CreateForwardTodos(const WordsBitmap &bitmap, const WordsRange &range, BitmapContainer &bitmapContainer);

  void CacheForNeural(Collector& collector);
  
  void ProcessStackForNeuro(HypothesisStackCubePruning*& stack);

  
  //void CreateForwardTodos2(HypothesisStackCubePruning &stack);
  //! create a back pointer to this bitmap, with edge that has this words range translation
  //void CreateForwardTodos2(const WordsBitmap &bitmap, const WordsRange &range, BitmapContainer &bitmapContainer);

  bool CheckDistortion(const WordsBitmap &bitmap, const WordsRange &range) const;

  void PrintBitmapContainerGraph();

public:
  SearchCubePruning(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl);
  ~SearchCubePruning();

  void Decode();

  void OutputHypoStackSize();
  void OutputHypoStack(int stack);

  virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
  virtual const Hypothesis *GetBestHypothesis() const;
};


}
#endif
