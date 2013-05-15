/*
 * OnlineLearner.h
 *
 */

/*
 * I need a pointer to Manager of current sentence
 * from that manager I can get all the hypothesis
 * and I need post edited sentence
 * 			- I concatenate the sentence with source e.g src_#_tgt
 *
 * This is a stateless feature because rescoring of the feature
 * does not depend on partial hypothesis, we just change it at the
 * end of translating sentence.
*/

#include "Util.h"
#include "TypeDef.h"
#include "FeatureFunction.h"
#include "Hypothesis.h"
#include "Factor.h"
#include "TrellisPath.h"
#include "TrellisPathList.h"
#include "Manager.h"
#include "OnlineLearning/SparseVec.h"
#include "OnlineLearning/Optimiser.h"

#ifndef ONLINELEARNER_H_
#define ONLINELEARNER_H_

typedef std::map<std::string, std::map<std::string, float> > pp_feature;
typedef std::map<std::string, std::map<std::string, int> > pp_list;
typedef float learningrate;

using namespace std;
using namespace Optimizer;
namespace Moses {

class Optimiser;
class Phrase;
class Search;

class OnlineLearner : public StatelessFeatureFunction {

private:
	OnlineAlgorithm implementation;
	pp_feature m_feature;
	pp_list m_featureIdx;
	pp_list PP_ORACLE, PP_BEST;
	learningrate flr, wlr;
	int m_PPindex;
	std::string m_postedited;
	bool m_learn, update_weights, update_features, m_perceptron, m_mira, sparse_weights_on;
	MiraOptimiser* optimiser;
	std::vector<std::string> function_words_english;
	std::vector<std::string> function_words_italian;
	void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;
	void ShootUp(std::string sp, std::string tp, float margin);
	void ShootDown(std::string sp, std::string tp, float margin);
	float calcMargin(Hypothesis* oracle, Hypothesis* bestHyp);
	void PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream);
	bool has_only_spaces(const std::string& str);
	float GetBleu(std::string hypothesis, std::string reference);
	void compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams);
	int getNGrams(std::string str, map<string, int>& ngrams);
	int split_marker_perl(string str, string marker, vector<string> &array);
	void ReadFunctionWords();
	void chop(string &str);
	void Decay(int);
	void Insert(std::string sp, std::string tp);
public:
	SparseVec sparsevector;
	OnlineLearner(float f_learningrate, float w_learningrate);
	OnlineLearner(OnlineAlgorithm implementation, float w_learningrate, float f_learningrate, float slack, float scale_margin, float scale_margin_precision,	float scale_update,
			float scale_update_precision, bool boost, bool normaliseMargin, int sigmoidParam);
	bool SetPostEditedSentence(std::string s);
	void RunOnlineLearning(Manager& manager);
	void RemoveJunk();
	virtual ~OnlineLearner();

	inline size_t GetNumScoreComponents() const { return 1; };
	void SetOnlineLearningTrue() { m_learn=true; };
	void SetOnlineLearningFalse() { m_learn=false; };
	bool GetOnlineLearning() const { return m_learn; };

	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "ol"; };
	void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
	void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;

	int RetrieveIdx(std::string sp, std::string tp);

};
}
#endif /* ONLINELEARNER_H_ */
