// $Id$
#pragma once

#include <vector>
#include <boost/functional/hash.hpp>
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
class LBLLM : public LanguageModelSingleFactor
{
protected:

public:
	LBLLM(const std::string &line)
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

  ~LBLLM()
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
  }


  virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = 0) const
  {
    std::vector<int> context;
    int word;
    mapper->convert(contextFactor, context, word);

    size_t context_width = m_nGramOrder - 1;

    if (!context.empty() && context.back() == kSTART) {
      context.resize(context_width, kSTART);
    } else {
      context.resize(context_width, kUNKNOWN);
    }


    double score;
    score = model.predict(word, context);

    /*
	std::string str = DebugContextFactor(contextFactor);
    std::cerr << "contextFactor=" << str << " " << score << std::endl;
	*/

    LMResult ret;
    ret.score = score;
    ret.unknown = (word == kUNKNOWN);

    // calc state from hash of last n-1 words
    size_t seed = 0;
    boost::hash_combine(seed, word);
    for (size_t i = 0; i < context.size() && i < context_width - 1; ++i) {
    	int id = context[i];
    	boost::hash_combine(seed, id);
    }

    (*finalState) = (State*) seed;
    return ret;
  }

protected:
  oxlm::Dict dict;
  boost::shared_ptr<oxlm::ModelData> config;
  Model model;

  int kSTART;
  int kSTOP;
  int kUNKNOWN;

  boost::shared_ptr<OXLMMapper> mapper;

};


}
