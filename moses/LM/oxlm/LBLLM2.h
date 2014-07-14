// $Id$
#pragma once

#include <vector>
#include "moses/LM/SingleFactor.h"
#include "moses/FactorCollection.h"

// lbl stuff
#include "corpus/corpus.h"
#include "lbl/lbl_features.h"
#include "lbl/model.h"
#include "lbl/process_identifier.h"
#include "lbl/query_cache.h"

#include "lbl/cdec_lbl_mapper.h"
#include "lbl/cdec_rule_converter.h"
#include "lbl/cdec_state_converter.h"

#include "Mapper.h"

namespace Moses
{

template<class Model>
class LBLLM2 : public LanguageModelSingleFactor
{
protected:

public:
	LBLLM2(const std::string &line)
	:LanguageModelSingleFactor(line)
	{
		ReadParameters();

		FactorCollection &factorCollection = FactorCollection::Instance();

		// needed by parent language model classes. Why didn't they set these themselves?
		m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
		m_sentenceStartWord[m_factorType] = m_sentenceStart;

		m_sentenceEnd		= factorCollection.AddFactor(Output, m_factorType, EOS_);
		m_sentenceEndWord[m_factorType] = m_sentenceEnd;
	}

  ~LBLLM2()
  {}

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


  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const
  {
    LMResult ret;
    ret.score = contextFactor.size();
    ret.unknown = false;

    // use last word as state info
    const Factor *factor;
    size_t hash_value(const Factor &f);
    if (contextFactor.size()) {
      factor = contextFactor.back()->GetFactor(m_factorType);
    } else {
      factor = NULL;
    }

    (*finalState) = (State*) factor;

    return ret;
  }

protected:
  oxlm::Dict dict;
  boost::shared_ptr<oxlm::ModelData> config;
  Model model;

  int kSTART;
  int kSTOP;
  int kUNKNOWN;
  int kSTAR;

};


}
