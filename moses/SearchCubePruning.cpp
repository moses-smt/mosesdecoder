#include "Manager.h"
#include "Util.h"
#include "SearchCubePruning.h"
#include "StaticData.h"
#include "InputType.h"
#include "TranslationOptionCollection.h"

using namespace std;

namespace Moses
{
class BitmapContainerOrderer
{
public:
  bool operator()(const BitmapContainer* A, const BitmapContainer* B) const {
    if (B->Empty()) {
      if (A->Empty()) {
        return A < B;
      }
      return false;
    }
    if (A->Empty()) {
      return true;
    }

    // Compare the top hypothesis of each bitmap container using the TotalScore, which includes future cost
    const float scoreA = A->Top()->GetHypothesis()->GetTotalScore();
    const float scoreB = B->Top()->GetHypothesis()->GetTotalScore();

    if (scoreA < scoreB) {
      return true;
    } else if (scoreA > scoreB) {
      return false;
    } else {
      return A < B;
    }
  }
};

SearchCubePruning::SearchCubePruning(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl)
  :Search(manager)
  ,m_source(source)
  ,m_hypoStackColl(source.GetSize() + 1)
  ,m_start(clock())
  ,m_transOptColl(transOptColl)
{
  const StaticData &staticData = StaticData::Instance();

  std::vector < HypothesisStackCubePruning >::iterator iterStack;
  for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind) {
    HypothesisStackCubePruning *sourceHypoColl = new HypothesisStackCubePruning(m_manager);
    sourceHypoColl->SetMaxHypoStackSize(staticData.GetMaxHypoStackSize());
    sourceHypoColl->SetBeamWidth(staticData.GetBeamWidth());

    m_hypoStackColl[ind] = sourceHypoColl;
  }
}

SearchCubePruning::~SearchCubePruning()
{
  RemoveAllInColl(m_hypoStackColl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchCubePruning::ProcessSentence()
{
  const StaticData &staticData = StaticData::Instance();

  // initial seed hypothesis: nothing translated, no words produced
  Hypothesis *hypo = Hypothesis::Create(m_manager,m_source, m_initialTransOpt);

  HypothesisStackCubePruning &firstStack = *static_cast<HypothesisStackCubePruning*>(m_hypoStackColl.front());
  firstStack.AddInitial(hypo);
  // Call this here because the loop below starts at the second stack.
  firstStack.CleanupArcList();
  CreateForwardTodos(firstStack);

  const size_t PopLimit = StaticData::Instance().GetCubePruningPopLimit();
  VERBOSE(3,"Cube Pruning pop limit is " << PopLimit << std::endl)

  const size_t Diversity = StaticData::Instance().GetCubePruningDiversity();
  VERBOSE(3,"Cube Pruning diversity is " << Diversity << std::endl)

  // go through each stack
  size_t stackNo = 1;
  std::vector < HypothesisStack* >::iterator iterStack;
  for (iterStack = ++m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack) {
    // check if decoding ran out of time
    double _elapsed_time = GetUserTime();
    if (_elapsed_time > staticData.GetTimeoutThreshold()) {
      VERBOSE(1,"Decoding is out of time (" << _elapsed_time << "," << staticData.GetTimeoutThreshold() << ")" << std::endl);
      return;
    }
    HypothesisStackCubePruning &sourceHypoColl = *static_cast<HypothesisStackCubePruning*>(*iterStack);

    // priority queue which has a single entry for each bitmap container, sorted by score of top hyp
    std::priority_queue< BitmapContainer*, std::vector< BitmapContainer* >, BitmapContainerOrderer> BCQueue;

    _BMType::const_iterator bmIter;
    const _BMType &accessor = sourceHypoColl.GetBitmapAccessor();

    for(bmIter = accessor.begin(); bmIter != accessor.end(); ++bmIter) {
      bmIter->second->InitializeEdges();
      BCQueue.push(bmIter->second);

      // old algorithm
      // bmIter->second->EnsureMinStackHyps(PopLimit);
    }

    // main search loop, pop k best hyps
    for (size_t numpops = 1; numpops <= PopLimit && !BCQueue.empty(); numpops++) {
      BitmapContainer *bc = BCQueue.top();
      BCQueue.pop();
      bc->ProcessBestHypothesis();
      if (!bc->Empty())
        BCQueue.push(bc);
    }

    // ensure diversity, a minimum number of inserted hyps for each bitmap container;
    //    NOTE: diversity doesn't ensure they aren't pruned at some later point
    if (Diversity > 0) {
      for(bmIter = accessor.begin(); bmIter != accessor.end(); ++bmIter) {
        bmIter->second->EnsureMinStackHyps(Diversity);
      }
    }

    // the stack is pruned before processing (lazy pruning):
    VERBOSE(3,"processing hypothesis from next stack");
    // VERBOSE("processing next stack at ");
    sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
    VERBOSE(3,std::endl);
    sourceHypoColl.CleanupArcList();

    CreateForwardTodos(sourceHypoColl);

    stackNo++;
  }

  //PrintBitmapContainerGraph();

  // some more logging
  IFVERBOSE(2) {
    m_manager.GetSentenceStats().SetTimeTotal( clock()-m_start );
  }
  VERBOSE(2, m_manager.GetSentenceStats());
}

void SearchCubePruning::CreateForwardTodos(HypothesisStackCubePruning &stack)
{
  const _BMType &bitmapAccessor = stack.GetBitmapAccessor();
  _BMType::const_iterator iterAccessor;
  size_t size = m_source.GetSize();

  stack.AddHypothesesToBitmapContainers();

  for (iterAccessor = bitmapAccessor.begin() ; iterAccessor != bitmapAccessor.end() ; ++iterAccessor) {
    const WordsBitmap &bitmap = iterAccessor->first;
    BitmapContainer &bitmapContainer = *iterAccessor->second;

    if (bitmapContainer.GetHypothesesSize() == 0) {
      // no hypothese to expand. don't bother doing it
      continue;
    }

    // Sort the hypotheses inside the Bitmap Container as they are being used by now.
    bitmapContainer.SortHypotheses();

    // check bitamp and range doesn't overlap
    size_t startPos, endPos;
    for (startPos = 0 ; startPos < size ; startPos++) {
      if (bitmap.GetValue(startPos))
        continue;

      // not yet covered
      WordsRange applyRange(startPos, startPos);
      if (CheckDistortion(bitmap, applyRange)) {
        // apply range
        CreateForwardTodos(bitmap, applyRange, bitmapContainer);
      }

      size_t maxSize = size - startPos;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, maxSizePhrase);

      for (endPos = startPos+1; endPos < startPos + maxSize; endPos++) {
        if (bitmap.GetValue(endPos))
          break;

        WordsRange applyRange(startPos, endPos);
        if (CheckDistortion(bitmap, applyRange)) {
          // apply range
          CreateForwardTodos(bitmap, applyRange, bitmapContainer);
        }
      }
    }
  }
}

void SearchCubePruning::CreateForwardTodos(const WordsBitmap &bitmap, const WordsRange &range, BitmapContainer &bitmapContainer)
{
  WordsBitmap newBitmap = bitmap;
  newBitmap.SetValue(range.GetStartPos(), range.GetEndPos(), true);

  size_t numCovered = newBitmap.GetNumWordsCovered();
  const TranslationOptionList &transOptList = m_transOptColl.GetTranslationOptionList(range);
  const SquareMatrix &futureScore = m_transOptColl.GetFutureScore();

  if (transOptList.size() > 0) {
    HypothesisStackCubePruning &newStack = *static_cast<HypothesisStackCubePruning*>(m_hypoStackColl[numCovered]);
    newStack.SetBitmapAccessor(newBitmap, newStack, range, bitmapContainer, futureScore, transOptList);
  }
}

bool SearchCubePruning::CheckDistortion(const WordsBitmap &hypoBitmap, const WordsRange &range) const
{
  // since we check for reordering limits, its good to have that limit handy
  int maxDistortion = StaticData::Instance().GetMaxDistortion();

  // if there are reordering limits, make sure it is not violated
  // the coverage bitmap is handy here (and the position of the first gap)
  const size_t	hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
                                  , startPos				= range.GetStartPos()
                                      , endPos					= range.GetEndPos();

  // if reordering constraints are used (--monotone-at-punctuation or xml), check if passes all
  if (! m_source.GetReorderingConstraint().Check( hypoBitmap, startPos, endPos ) ) {
    return false;
  }

  // no limit of reordering: no problem
  if (maxDistortion < 0) {
    return true;
  }

  bool leftMostEdge = (hypoFirstGapPos == startPos);
  // any length extension is okay if starting at left-most edge
  if (leftMostEdge) {
    return true;
  }
  // starting somewhere other than left-most edge, use caution
  // the basic idea is this: we would like to translate a phrase starting
  // from a position further right than the left-most open gap. The
  // distortion penalty for the following phrase will be computed relative
  // to the ending position of the current extension, so we ask now what
  // its maximum value will be (which will always be the value of the
  // hypothesis starting at the left-most edge).  If this vlaue is than
  // the distortion limit, we don't allow this extension to be made.
  WordsRange bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);
  int required_distortion =
    m_source.ComputeDistortionDistance(range, bestNextExtension);

  if (required_distortion > maxDistortion) {
    return false;
  }
  return true;
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *SearchCubePruning::GetBestHypothesis() const
{
  //	const HypothesisStackCubePruning &hypoColl = m_hypoStackColl.back();
  const HypothesisStack &hypoColl = *m_hypoStackColl.back();
  return hypoColl.GetBestHypothesis();
}

/**
 * Logging of hypothesis stack sizes
 */
void SearchCubePruning::OutputHypoStackSize()
{
  std::vector < HypothesisStack* >::const_iterator iterStack = m_hypoStackColl.begin();
  TRACE_ERR( "Stack sizes: " << (int)(*iterStack)->size());
  for (++iterStack; iterStack != m_hypoStackColl.end() ; ++iterStack) {
    TRACE_ERR( ", " << (int)(*iterStack)->size());
  }
  TRACE_ERR( endl);
}

void SearchCubePruning::PrintBitmapContainerGraph()
{
  HypothesisStackCubePruning &lastStack = *static_cast<HypothesisStackCubePruning*>(m_hypoStackColl.back());
  const _BMType &bitmapAccessor = lastStack.GetBitmapAccessor();

  _BMType::const_iterator iterAccessor;
  for (iterAccessor = bitmapAccessor.begin(); iterAccessor != bitmapAccessor.end(); ++iterAccessor) {
    cerr << iterAccessor->first << endl;
    //BitmapContainer &container = *iterAccessor->second;
  }

}

/**
 * Logging of hypothesis stack contents
 * \param stack number of stack to be reported, report all stacks if 0
 */
void SearchCubePruning::OutputHypoStack(int stack)
{
  if (stack >= 0) {
    TRACE_ERR( "Stack " << stack << ": " << endl << m_hypoStackColl[stack] << endl);
  } else {
    // all stacks
    int i = 0;
    vector < HypothesisStack* >::iterator iterStack;
    for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack) {
      HypothesisStackCubePruning &hypoColl = *static_cast<HypothesisStackCubePruning*>(*iterStack);
      TRACE_ERR( "Stack " << i++ << ": " << endl << hypoColl << endl);
    }
  }
}

const std::vector < HypothesisStack* >& SearchCubePruning::GetHypothesisStacks() const
{
  return m_hypoStackColl;
}

}

