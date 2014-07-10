#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/FFState.h"

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
  int m_targetLen;
public:
  LBLLMState(int targetLen)
  {}

  int Compare(const FFState& other) const;
};

template<class Model>
class LBLLM : public StatefulFeatureFunction
{
public:
	LBLLM(const std::string &line)
	:StatefulFeatureFunction(2, line)
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
    return new LBLLMState(0);
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
    const ChartHypothesis &cur_hypo,
    int featureID,
    ScoreComponentCollection* accumulator) const
  {

  }

  void SetParameter(const std::string& key, const std::string& value)
  {
    if (key == "path") {
  	  m_path = value;
    }
    else {
      StatefulFeatureFunction::SetParameter(key, value);
    }
  }


protected:
  std::string m_path;

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

};


}

