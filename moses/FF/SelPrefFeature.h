#ifndef moses_SelPrefFeature_h
#define moses_SelPrefFeature_h

#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Word.h"
#include "InternalTree.h"
#include "moses/LM/Ken.h"
#include "lm/model.hh"

#include <unordered_map>
#include <map>
#include <set>

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

  bool FindHeadRecursively(TreePointer tree, const std::vector<TreePointer> &previous_trees, size_t &childId) const;
  std::vector<std::string> ProcessChild(TreePointer child, size_t childId, TreePointer currentNode, const std::vector<TreePointer> &previous_trees) const;
  void MakeTuples(TreePointer tree, const std::vector<TreePointer> &previous_trees, std::vector<std::vector<std::string>> &depRelTuples) const;

  float GetWBScore(std::vector<std::string>& depRel) const;

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

  void ReadLemmaMap();
  void ReadMIModel();

  void Load();

  void CleanUpAfterSentenceProcessing(const InputType& source);


  static float score;

protected:
  // WB model file in ARPA format
  std::string m_modelFileARPA;

  // Selectional Preference model file
  std::string m_MIModelFile;

  std::string m_lemmaFile;

  // Pointer to the dependency language model
  std::shared_ptr<lm::ngram::Model> m_WBmodel;

  // Pointer to the selectional preference model
  std::shared_ptr<std::map<std::vector<std::string>, std::vector<float>>> m_MIModel;

  // Dependency relations that are considered by this feature
  std::set<std::string> m_allowedLabels;

  // If the rule table is binarized then unbinarize each hypothesis before extracting the head words
  bool m_unbinarize;

  // todo: initalize
  std::shared_ptr<std::unordered_map<std::string, std::string>> m_lemmaMap;


};


class SelPrefState : public TreeState{
public:
	SelPrefState(TreePointer tree, size_t hash)
		: TreeState(tree)
		, m_depRelHash(hash)
	{}
	size_t GetHash() const{
		return m_depRelHash;
	}
	int Compare(const FFState& other) const {
	    if (m_depRelHash == static_cast<const SelPrefState*>(&other)->GetHash()) return 0;
	    else if (m_depRelHash > static_cast<const SelPrefState*>(&other)->GetHash()) return 1;
	    else return -1;
	  }
protected:
	// head words of non-terminal in pre-order traversal of non-terminals
	// maybe I should map to the TreePointer correspondong to the terminal node so not to save another Word
	size_t m_depRelHash;

};

}
#endif
