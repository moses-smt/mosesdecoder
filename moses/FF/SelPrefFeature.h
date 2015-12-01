#ifndef moses_SelPrefFeature_h
#define moses_SelPrefFeature_h

#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Word.h"
#include "InternalTree.h"
//#include "moses/LM/Ken.h"
//#include "lm/model.hh"

#include <unordered_map>

namespace Moses{

typedef std::shared_ptr<std::unordered_map<InternalTree*, const Word*>> HeadsPointer;

class SelPrefFeature : public StatefulFeatureFunction{

public:

  SelPrefFeature(const std::string &line);
  ~SelPrefFeature(){};

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
      return NULL;
    }

  bool FindHeadRecursively(TreePointer tree, const std::vector<TreePointer> &previous_trees, const std::vector<HeadsPointer> &previous_heads, std::unordered_map<InternalTree*, const Word*> &childrenHeadWords, size_t &childId) const;
  void MakeTuples(TreePointer tree, std::unordered_map<InternalTree*,const Word*> &childrenHeadWords, std::vector<std::vector<std::string>> &depRelTuples) const;

  void EvaluateInIsolation(const Phrase &source
                  , const TargetPhrase &targetPhrase
                  , ScoreComponentCollection &scoreBreakdown
                  , ScoreComponentCollection &estimatedFutureScore) const {};
  void EvaluateWithSourceContext(const InputType &input
                  , const InputPath &inputPath
                  , const TargetPhrase &targetPhrase
                  , const StackVec *stackVec
                  , ScoreComponentCollection &scoreBreakdown
                  , ScoreComponentCollection *estimatedFutureScore = NULL) const {};

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
        , const TranslationOptionList &translationOptionList) const {
    }

  FFState* EvaluateWhenApplied(
      const Hypothesis& cur_hypo,
      const FFState* prev_state,
      ScoreComponentCollection* accumulator) const {UTIL_THROW(util::Exception, "Not implemented");};

  FFState* EvaluateWhenApplied(
      const ChartHypothesis& /* cur_hypo */,
      int /* featureID - used to index the state in the previous hypotheses */,
      ScoreComponentCollection* accumulator) const;

  void Load();

  void CleanUpAfterSentenceProcessing(const InputType& source);

protected:

};


class SelPrefState : public TreeState{
public:
	SelPrefState(TreePointer tree, HeadsPointer heads)
		: TreeState(tree)
		, m_heads(heads)
	{}
	HeadsPointer GetHeads() const{
		return m_heads;
	}
protected:
	// head words of non-terminal in pre-order traversal of non-terminals
	// maybe I should map to the TreePointer correspondong to the terminal node so not to save another Word
	HeadsPointer m_heads;

};

}
#endif
