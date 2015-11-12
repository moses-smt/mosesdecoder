/*
 * LanguageModel.h
 *
 *  Created on: 29 Oct 2015
 *      Author: hieu
 */

#ifndef LANGUAGEMODEL_H_
#define LANGUAGEMODEL_H_

#include "../FF/StatefulFeatureFunction.h"
#include "../TypeDef.h"
#include "../MorphoTrie/MorphTrie.h"
#include "moses/Factor.h"
#include "moses/TypeDef.h"

////////////////////////////////////////////////////////////////////////////////////////
struct LMScores
{
	LMScores()
	{}

	LMScores(const LMScores &copy)
	:prob(copy.prob)
	,backoff(copy.backoff)
	{}

	LMScores(float inProb, float inBackoff)
	:prob(inProb)
	,backoff(inBackoff)
	{}

	float prob, backoff;
};

inline std::ostream& operator<<(std::ostream &out, const LMScores &obj)
{
	out << "(" << obj.prob << "," << obj.backoff << ")" << std::flush;
	return out;
}

////////////////////////////////////////////////////////////////////////////////////////
class LanguageModel : public StatefulFeatureFunction
{
public:
	LanguageModel(size_t startInd, const std::string &line);
	virtual ~LanguageModel();

	virtual void Load(System &system);

	virtual void SetParameter(const std::string& key, const std::string& value);

    virtual FFState* BlankState(const Manager &mgr, const PhraseImpl &input) const;
    virtual void EmptyHypothesisState(FFState &state, const Manager &mgr, const PhraseImpl &input) const;

	  virtual void
	  EvaluateInIsolation(const System &system,
			  const Phrase &source, const TargetPhrase &targetPhrase,
	          Scores &scores,
	          Scores *estimatedScores) const;

	  virtual void EvaluateWhenApplied(const Manager &mgr,
	    const Hypothesis &hypo,
	    const FFState &prevState,
	    Scores &scores,
		FFState &state) const;

protected:
	std::string m_path;
	Moses::FactorType m_factorType;
	size_t m_order;

    MorphTrie<const Moses::Factor*, LMScores> m_root;
    SCORE m_oov;
    const Moses::Factor *m_bos;
    const Moses::Factor *m_eos;

    void ShiftOrPush(std::vector<const Moses::Factor*> &context, const Moses::Factor *factor) const;
    std::pair<SCORE, void*> Score(const std::vector<const Moses::Factor*> &context) const;
    SCORE BackoffScore(const std::vector<const Moses::Factor*> &context) const;

    void DebugContext(const std::vector<const Moses::Factor*> &context) const;
};

#endif /* LANGUAGEMODEL_H_ */
