#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"
#include "moses/Util.h"
#include "moses/ChartHypothesis.h"

// lbl stuff
#include "corpus/corpus.h"
#include "lbl/lbl_features.h"
#include "lbl/model.h"
#include "lbl/process_identifier.h"
#include "lbl/query_cache.h"

#include "lbl/cdec_lbl_mapper.h"
#include "lbl/cdec_rule_converter.h"
#include "lbl/cdec_state_converter.h"

#include "oxlm/Mapper.h"

namespace Moses
{

class LBLLMState : public FFState
{
  std::vector<int> m_left, m_right;
public:
  LBLLMState()
  {}

  LBLLMState(const std::vector<int> &left, const std::vector<int> &right)
  :m_left(left)
  ,m_right(right)
  {}

  int Compare(const FFState& other) const;
};

/**
 * Wraps the feature values computed from the LBL language model.
 */
struct LBLFeatures {
  LBLFeatures() : LMScore(0), OOVScore(0) {}
  LBLFeatures(double lm_score, double oov_score)
      : LMScore(lm_score), OOVScore(oov_score) {}
  LBLFeatures& operator+=(const LBLFeatures& other) {
    LMScore += other.LMScore;
    OOVScore += other.OOVScore;
    return *this;
  }

  double LMScore;
  double OOVScore;
};

// FF class
template<class Model>
class LBLLM : public StatefulFeatureFunction
{
public:
	LBLLM(const std::string &line)
	:StatefulFeatureFunction(2, line)
	,m_order(5)
	{
	  ReadParameters();
	}

  void Load()
  {
    model.load(m_path);

    config = model.getConfig();
    int context_width = config->ngram_order - 1;
    // For each state, we store at most context_width word ids to the left and
    // to the right and a kSTAR separator. The last bit represents the actual
    // size of the state.
    //int max_state_size = (2 * context_width + 1) * sizeof(int) + 1;
    //FeatureFunction::SetStateSize(max_state_size);

    dict = model.getDict();
    mapper = boost::make_shared<OXLMMapper>(dict);
    //stateConverter = boost::make_shared<CdecStateConverter>(max_state_size - 1);
    //ruleConverter = boost::make_shared<CdecRuleConverter>(mapper, stateConverter);

    kSTART = dict.Convert("<s>");
    kSTOP = dict.Convert("</s>");
    kUNKNOWN = dict.Convert("<unk>");
    kSTAR = dict.Convert("<{STAR}>");
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new LBLLMState();
  }

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {

  }

  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {

  }

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const
  {

  }

  FFState* EvaluateWhenApplied(
    const ChartHypothesis &hypo,
    int featureID,
    ScoreComponentCollection* accumulator) const
  {
	  /*
	  std::vector<int> leftIds, rightIds;
	  Phrase leftPhrase, rightPhrase;
	  hypo.GetOutputPhrase(1, m_order, leftPhrase);
	  hypo.GetOutputPhrase(2, m_order, rightPhrase);

	  leftIds = mapper->convert(leftPhrase);
	  rightIds = mapper->convert(rightPhrase);

	  LBLFeatures leftScores = scoreFullContexts(leftIds);
	  LBLFeatures rightScores = scoreFullContexts(rightIds);

	  std::vector<float> scores(2);
	  scores[0] = leftScores.LMScore + rightScores.LMScore;
	  scores[1] = leftScores.OOVScore + rightScores.OOVScore;

	  accumulator->PlusEquals(this, scores);

	  LBLLMState *state = new LBLLMState(leftIds, rightIds);
	  return state;
	*/

	  // baseline non-optimized scoring
	  Phrase phrase;
	  hypo.GetOutputPhrase(phrase);
	  std::cerr << "phrase=" << phrase << std::endl;

	  std::vector<int> ids;
	  ids = mapper->convert(phrase);

	  LBLFeatures leftScores = scoreFullContexts(ids);
	  std::vector<float> scores(2);
	  scores[0] = leftScores.LMScore;
	  scores[1] = leftScores.OOVScore;

	  accumulator->Assign(this, scores);

	  LBLLMState *state = new LBLLMState();
	  return state;

  }

  void SetParameter(const std::string& key, const std::string& value)
  {
    if (key == "path") {
  	  m_path = value;
    }
    else if (key == "order") {
      m_order = Scan<int>(value);
    }
    else {
      StatefulFeatureFunction::SetParameter(key, value);
    }
  }


protected:
  std::string m_path;
  int m_order;

  int fid;
  int fidOOV;
  oxlm::Dict dict;
  boost::shared_ptr<oxlm::ModelData> config;
  Model model;

  boost::shared_ptr<OXLMMapper> mapper;
  /*
  boost::shared_ptr<oxlm::CdecRuleConverter> ruleConverter;
  boost::shared_ptr<oxlm::CdecStateConverter> stateConverter;
  */

  int kSTART;
  int kSTOP;
  int kUNKNOWN;
  int kSTAR;

  ////////////////////////////////////
  LBLFeatures scoreFullContexts(const vector<int>& symbols) const {
    LBLFeatures ret;
    int last_star = -1;
    int context_width = config->ngram_order - 1;
    for (size_t i = 0; i < symbols.size(); ++i) {
      if (symbols[i] == kSTAR) {
        last_star = i;
      } else if (i - last_star > context_width) {
        ret += scoreContext(symbols, i);
      }
    }

    return ret;
  }

  LBLFeatures scoreContext(const vector<int>& symbols, int position) const {
    int word = symbols[position];
    int context_width = config->ngram_order - 1;
    vector<int> context;
    for (int i = 1; i <= context_width && position - i >= 0; ++i) {
      assert(symbols[position - i] != kSTAR);
      context.push_back(symbols[position - i]);
    }

    if (!context.empty() && context.back() == kSTART) {
      context.resize(context_width, kSTART);
    } else {
      context.resize(context_width, kUNKNOWN);
    }

    double score;
    score = model.predict(word, context);
    return LBLFeatures(score, word == kUNKNOWN);
  }

};


}

