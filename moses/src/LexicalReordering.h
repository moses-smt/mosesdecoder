#ifndef moses_LexicalReordering_h
#define moses_LexicalReordering_h

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"
#include "FeatureFunction.h"

#include "LexicalReorderingState.h"
#include "LexicalReorderingTable.h"

namespace Moses
{

class Factor;
class Phrase;
class Hypothesis;
class InputType;

/** implementation of lexical reordering (Tilman ...) for phrase-based decoding
 */
class LexicalReordering : public StatefulFeatureFunction
{
public:
  LexicalReordering(std::vector<FactorType>& f_factors,
                    std::vector<FactorType>& e_factors,
                    const std::string &modelType,
                    const std::string &filePath,
                    const std::vector<float>& weights);
  virtual ~LexicalReordering();

  virtual size_t GetNumScoreComponents() const {
    return m_configuration.GetNumScoreComponents();
  }

  virtual FFState* Evaluate(const Hypothesis& cur_hypo,
                            const FFState* prev_state,
                            ScoreComponentCollection* accumulator) const;
	
  virtual FFState* EvaluateChart(const ChartHypothesis&,
                                 int /* featureID */,
																 ScoreComponentCollection*) const {
		CHECK(0); // not valid for chart decoder
		return NULL;
	}

  virtual const FFState* EmptyHypothesisState(const InputType &input) const;

  virtual std::string GetScoreProducerDescription(unsigned) const {
    return "LexicalReordering_" + m_modelTypeString;
  }

  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "d";
  };

  void InitializeForInput(const InputType& i) {
    m_table->InitializeForInput(i);
  }

  Scores GetProb(const Phrase& f, const Phrase& e) const;

private:
  bool DecodeCondition(std::string s);
  bool DecodeDirection(std::string s);
  bool DecodeNumFeatureFunctions(std::string s);

  LexicalReorderingConfiguration m_configuration;
  std::string m_modelTypeString;
  std::vector<std::string> m_modelType;
  LexicalReorderingTable* m_table;
  size_t m_numScoreComponents;
  //std::vector<Direction> m_direction;
  std::vector<LexicalReorderingConfiguration::Condition> m_condition;
  //std::vector<size_t> m_scoreOffset;
  //bool m_oneScorePerDirection;
  std::vector<FactorType> m_factorsE, m_factorsF;
};

}

#endif
