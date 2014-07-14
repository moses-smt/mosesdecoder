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
    model.load(m_filePath);

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
    std::vector<int> ids;
    ids = mapper->convert(contextFactor);
    int word = ids.back();

    size_t context_width = m_nGramOrder - 1;
    ids.resize(context_width, kUNKNOWN);

    double score;
    score = model.predict(word, ids);

    LMResult ret;
    ret.score = score;
    ret.unknown = (word == kUNKNOWN);

    (*finalState) = (State*) 0;
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

  boost::shared_ptr<OXLMMapper> mapper;

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
