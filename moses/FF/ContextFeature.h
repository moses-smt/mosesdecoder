/*
 * ContextFeature.h
 *
 *  Created on: Sep 18, 2013
 *      Author: braunefe
 */

#pragma once

#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/TypeDef.h"
#include "moses/ScoreComponentCollection.h"
#include "psd/FeatureExtractor.h"
#include "psd/FeatureConsumer.h"
#include <map>
#include <string>
#include <vector>


namespace Moses
{

class ChartTranslationOption;

class ContextFeature : public StatelessFeatureFunction
{
public:
	ContextFeature(const std::string &line);

	//Is this allowed (?)
	~ContextFeature();

	void Load();

	bool IsUseable(const FactorMask &mask) const
	{ return true; }

	void Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const;


	//Fabienne Braune : Do nothing in there
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown) const
	{}


    //Fabienne Braune : Call VW and re-score translation options
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ChartTranslationOptions &transOpts) const;

	virtual void Evaluate(const Hypothesis& hypo,
	                        ScoreComponentCollection* accumulator) const
	  {}
	  void EvaluateChart(const ChartHypothesis &hypo,
	                             ScoreComponentCollection* accumulator) const
	  {}

	  // initialize vw
	  bool Initialize(const std::string &modelFile, const std::string &indexFile, const std::string &configFile);


	  std::vector<ScoreComponentCollection> ScoreRules(
	                                                     size_t startSpan,
	                                                     size_t endSpan,
	                                                     const std::string &sourceSide,
	                                                     std::vector<std::string> *targetRepresentations,
	                                                     const InputType &source,
	                                                     std::map<std::string,ChartTranslationOption> * targetMap) const;


	   void CheckIndex(const std::string &targetRep);
	   PSD::ChartTranslation GetPSDTranslation(const std::string targetRep, const TargetPhrase * tp) const;
	   void Normalize(std::vector<float> &losses) const;
	   void Normalize0(std::vector<float> &losses) const;
	   void Normalize1(std::vector<float> &losses) const;
	   double LogAddition(double logA, double logB, double logAddPrecision) const;
	   void Interpolate(std::vector<float> &losses, std::vector<float> &pEgivenF, float interpolParam) const;

	private :
    PSD::TargetIndexType m_ruleIndex; //FB : this target index type remains empty during decoding
    PSD::FeatureExtractor *m_extractor, *m_debugExtractor;
    PSD::VWLibraryPredictConsumerFactory  *m_consumerFactory;
    PSD::VWFileTrainConsumer *m_debugConsumer;
    PSD::ExtractorConfig m_extractorConfig;
    bool IsOOV(const std::string &targetRep);
    bool LoadRuleIndex(const std::string &indexFile);
    std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now

    public :
    ScoreComponentCollection ScoreFactory(float score) const;

};

}
