/*
 * OnlineLearner.cpp
 *
 */

#include "OnlineLearner.h"
#include "StaticData.h"
#include "math.h"

using namespace Optimizer;

namespace Moses {

void OnlineLearner::chop(string &str) {
	int i = 0;
	while (isspace(str[i]) != 0) {
		str.replace(i, 1, "");
	}
	while (isspace(str[str.length() - 1]) != 0) {
		str.replace(str.length() - 1, 1, "");
	}
	return;
}

int OnlineLearner::split_marker_perl(string str, string marker, vector<string> &array) {
	int found = str.find(marker), prev = 0;
	while (found != string::npos) // warning!
	{
		array.push_back(str.substr(prev, found - prev));
		prev = found + marker.length();
		found = str.find(marker, found + marker.length());
	}
	array.push_back(str.substr(prev));
	return array.size() - 1;
}

int OnlineLearner::getNGrams(std::string str, map<string, int>& ngrams) {
	vector<string> words;
	int numWords = split_marker_perl(str, " ", words);
	for (int i = 1; i <= 4; i++) {
		for (int start = 0; start < numWords - i + 1; start++) {
			string str = "";
			for (int pos = 0; pos < i; pos++) {
				str += words[start + pos] + " ";
			}
			ngrams[str]++;
		}
	}
	return str.size();
}

void OnlineLearner::compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams) {
	for (map<string, int>::const_iterator itr = hyp.begin(); itr != hyp.end(); itr++) {
		vector<string> temp;
		int ngrams = split_marker_perl((*itr).first, " ", temp);
		TotalNgrams[ngrams] += hyp[(*itr).first];
		if (ref.find((*itr).first) != ref.end()) {
			if (hyp[(*itr).first] >= ref[(*itr).first]) {
				countNgrams[ngrams] += hyp[(*itr).first];
			} else {
				countNgrams[ngrams] += ref[(*itr).first];
			}
		}
	}
	for (int i = 1; i <= 4; i++) {
		TotalNgrams[i]+=0.1;
		countNgrams[i]+=0.1;
	}
	return;
}

float OnlineLearner::GetBleu(std::string hypothesis, std::string reference)
{
	double bp=1;
	map<string, int> hypNgrams, refNgrams;
	map<int, float> countNgrams, TotalNgrams;
	map<int, double> BLEU;
	int length_translation = getNGrams(hypothesis, hypNgrams);
	int length_reference = getNGrams(reference, refNgrams);
	compareNGrams(hypNgrams, refNgrams, countNgrams, TotalNgrams);

	for(int i=1; i<=4; i++)
	{
		BLEU[i] = (countNgrams[i]*1.0)/(TotalNgrams[i]*1.0);
	}
	double ratio = ((length_reference*1.0+1.0) / (length_translation*1.0+1.0) );
	if(length_translation < length_reference)
		bp = exp(1 - ratio);
	return ((bp * exp( ( log(BLEU[1]) + log(BLEU[2]) + log(BLEU[3]) + log(BLEU[4]) ) / 4))*100);
}


bool OnlineLearner::has_only_spaces(const std::string& str) {
	return (str.find_first_not_of (' ') == str.npos);
}

void OnlineLearner::ReadFunctionWords()
{
	cerr<<"Loading function words...\n";
	std::ifstream fin("/home/prashant/Documents/function_words_english.txt");
	if (fin) {
		std::stringstream ss;
		ss << fin.rdbuf();
		function_words_english.push_back(ss.str());
	}
	fin.close();

	std::ifstream fit("/home/prashant/Documents/function_words_italian.txt");
	if (fit) {
		std::stringstream ss;
		ss << fit.rdbuf();
		function_words_italian.push_back(ss.str());
	}
	fit.close();

	return;
}

/*-----------x-------------x-------------*/


bool OnlineLearner::SetPostEditedSentence(std::string s)
{
	if(m_postedited.empty())
	{
		m_postedited=s;
		return true;
	}
	else
	{
		cerr<<"post edited already exists.. "<<m_postedited<<endl;
		return false;
	}
}

OnlineLearner::OnlineLearner(float learningrate):StatelessFeatureFunction("OnlineLearner",1){
	flr = learningrate;
	m_learn=false;
	m_PPindex=0;
	update_weights=false;
	update_features=true;
	m_perceptron=true;
	m_mira=false;
	ReadFunctionWords();
	cerr<<"Initialization Online Learning Model\n";
}

OnlineLearner::OnlineLearner(float w_learningrate, float f_learningrate, float slack, float scale_margin, float scale_margin_precision,	float scale_update,
		float scale_update_precision, bool boost, bool normaliseMargin, int sigmoidParam):StatelessFeatureFunction("OnlineLearner",1){
	flr = f_learningrate;
	wlr = w_learningrate;
	update_weights=true;
	update_features=true;
	m_PPindex=0;
	m_learn=false;
	m_mira=true;
	m_perceptron=false;
	optimiser = new Optimizer::MiraOptimiser(slack, scale_margin, scale_margin_precision, scale_update,
			scale_update_precision, boost, normaliseMargin, sigmoidParam);
	cerr<<"Initialization Online Learning Model\n";
}


void OnlineLearner::ShootUp(std::string sp, std::string tp, float margin)
{
	if (binary_search(function_words_italian.begin(),function_words_italian.end(), sp) ||
	    binary_search(function_words_english.begin(),function_words_english.end(), tp)) {
		return;
	}
	if(m_feature.find(sp)!=m_feature.end())
	{
		if(m_feature[sp].find(tp)!=m_feature[sp].end())
		{
			float val=m_feature[sp][tp];
			val += flr * margin;
			m_feature[sp][tp]=val;
		}
		else
		{
			m_feature[sp][tp]=flr*margin;
		}
	}
	else
	{
		m_feature[sp][tp]=flr*margin;
	}
	//if(m_feature[sp][tp]>1){m_feature[sp][tp]==1;}
}
void OnlineLearner::ShootDown(std::string sp, std::string tp, float margin)
{
	if (binary_search(function_words_italian.begin(),function_words_italian.end(), sp) ||
		binary_search(function_words_english.begin(),function_words_english.end(), tp)) {
		return;
	}
	if(m_feature.find(sp)!=m_feature.end())
	{
		if(m_feature[sp].find(tp)!=m_feature[sp].end())
		{
			float val=m_feature[sp][tp];
			val -= flr * margin;
			m_feature[sp][tp]=val;
		}
		else
		{
			m_feature[sp][tp]=0;
		}
	}
	else
	{
		m_feature[sp][tp]=0;
	}
	//if(m_feature[sp][tp]<0){m_feature[sp][tp]==0;}
}
float OnlineLearner::calcMargin(Hypothesis* oracle, Hypothesis* bestHyp)
{
	return (oracle->GetScore()-bestHyp->GetScore());
}
// clears history
void OnlineLearner::RemoveJunk()
{
	m_postedited.clear();
	PP_ORACLE.clear();
	PP_BEST.clear();
}
// insertion of sparse features .. for now its the phrase pair
// have to generalize this later !
void OnlineLearner::Insert(std::string sp, std::string tp)
{
	if(m_featureIdx.find(sp)==m_featureIdx.end())
	{
		if(m_featureIdx[sp].find(tp)==m_featureIdx[sp].end())
		{
			m_featureIdx[sp][tp]=sparsevector.GetSize();
			sparsevector.AddFeat(0.0);
			m_PPindex++;
			return;
		}
	}
	else if(m_featureIdx[sp].find(tp)==m_featureIdx[sp].end())
	{
		m_featureIdx[sp][tp]=sparsevector.GetSize();
		sparsevector.AddFeat(0.0);
		m_PPindex++;
		return;
	}
	return;
}

int OnlineLearner::RetrieveIdx(std::string sp,std::string tp)
{
	if(m_featureIdx.find(sp)!=m_featureIdx.end())
	{
		if(m_featureIdx[sp].find(tp)!=m_featureIdx[sp].end())
		{
			return m_featureIdx[sp][tp];
		}
	}
	return m_PPindex;
}

OnlineLearner::~OnlineLearner() {
	pp_feature::iterator iter;
	for(iter=m_feature.begin(); iter!=m_feature.end(); iter++)
	{
		(*iter).second.clear();
	}
}

void OnlineLearner::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
	float score=0.0;
	std::string s="",t="";
	size_t endpos = tp.GetSize();
	for (size_t pos = 0 ; pos < endpos ; ++pos) {
		if (pos > 0){ t += " "; }
		t += tp.GetWord(pos).GetFactor(0)->GetString();
	}
	endpos = tp.GetSourcePhrase().GetSize();
	for (size_t pos = 0 ; pos < endpos ; ++pos) {
		if (pos > 0){ s += " "; }
		s += tp.GetSourcePhrase().GetWord(pos).GetFactor(0)->GetString();
	}

	if(update_features && m_mira)
	{
		size_t idx;
		pp_list::const_iterator it;
		it=m_featureIdx.find(s);
		if(it!=m_featureIdx.end())
		{
			std::map<std::string, int>::const_iterator it2;
			it2=it->second.find(t);
			if(it2!=it->second.end())
			{
				score=sparsevector.getElement(it2->second);
			}
		}
	}
	else if(update_features && m_perceptron)
	{
		pp_feature::const_iterator it;
		it=m_feature.find(s);
		if(it!=m_feature.end())
		{
			std::map<std::string, float>::const_iterator it2;
			it2=it->second.find(t);
			if(it2!=it->second.end())
			{
				score=it2->second;
			}
		}
	}
	//cerr<<"Source : "<<s<<"\tTarget : "<<t<<"\tScore : "<<score<<endl;
	out->PlusEquals(this, score);
}


void OnlineLearner::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

void OnlineLearner::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

void OnlineLearner::PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream) {
	if (hypo->GetPrevHypo() != NULL) {
		PrintHypo(hypo->GetPrevHypo(), HypothesisStringStream);
		Phrase p = hypo->GetCurrTargetPhrase();
		for (size_t pos = 0; pos < p.GetSize(); pos++) {
			const Factor *factor = p.GetFactor(pos, 0);
			HypothesisStringStream << *factor << " ";
		}
		std::string sourceP = hypo->GetSourcePhraseStringRep();
		std::string targetP = hypo->GetTargetPhraseStringRep();
		PP_BEST[sourceP][targetP]=1;
		Insert(sourceP, targetP);
	}
}
void OnlineLearner::Decay(int lineNum)
{
	float decay_value = 1.0/(exp(lineNum)*1.0);
	pp_feature::iterator itr1=m_feature.begin();
	while(itr1!=m_feature.end())
	{
		std::map<std::string, float>::iterator itr2=(*itr1).second.begin();
		while(itr2!=(*itr1).second.end())
		{
			float score=m_feature[itr1->first][itr2->first];
			score *= decay_value;
			m_feature[itr1->first][itr2->first]=score;
			itr2++;
		}
		itr1++;
	}
}
void OnlineLearner::RunOnlineLearning(Manager& manager)
{
	const TranslationSystem &trans_sys = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
	const StaticData& staticData = StaticData::Instance();
	const std::vector<Moses::FactorType>& outputFactorOrder=staticData.GetOutputFactorOrder();
	ScoreComponentCollection weightUpdate = staticData.GetAllWeights();
	std::vector<const ScoreProducer*> sps = trans_sys.GetFeatureFunctions();
	ScoreProducer* sp = const_cast<ScoreProducer*>(sps[0]);
	for(int i=0;i<sps.size();i++)
	{
		if(sps[i]->GetScoreProducerDescription().compare("OnlineLearner")==0)
		{
			sp=const_cast<ScoreProducer*>(sps[i]);
			break;
		}
	}
	const Hypothesis* hypo = manager.GetBestHypothesis();
//	Decay(manager.m_lineNumber);
	stringstream bestHypothesis;
	PP_BEST.clear();
	PrintHypo(hypo, bestHypothesis);
	float bestbleu = GetBleu(bestHypothesis.str(), m_postedited);
	float bestScore=hypo->GetScore();
	cerr<<"Best Hypothesis : "<<bestHypothesis.str()<<endl;
	cerr<<"Post Edit       : "<<m_postedited<<endl;
	TrellisPathList nBestList;
	int nBestSize = 100;
	manager.CalcNBest(nBestSize, nBestList, true);

	std::string bestOracle;
	std::vector<string> HypothesisList, HypothesisHope, HypothesisFear;
	std::vector<float> loss, BleuScore, BleuScoreHope, BleuScoreFear, oracleBleuScores, lossHope, lossFear, modelScore, oracleModelScores;
	std::vector<std::vector<float> > losses, BleuScores, BleuScoresHope, BleuScoresFear, lossesHope, lossesFear, modelScores;
	std::vector<ScoreComponentCollection> featureValue,featureValueHope, featureValueFear, oraclefeatureScore;
	std::vector<std::vector<ScoreComponentCollection> > featureValues, featureValuesHope, featureValuesFear;
	vector<pp_list> OracleList;
	TrellisPathList::const_iterator iter;
	pp_list BestOracle,ShootemUp, ShootemDown;
	float maxBleu=0.0, maxScore=0.0,oracleScore=0.0;
	for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {
		const TrellisPath &path = **iter;
		PP_ORACLE.clear();
		const std::vector<const Hypothesis *> &edges = path.GetEdges();
		stringstream oracle;
		for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
			const Hypothesis &edge = *edges[currEdge];
			CHECK(outputFactorOrder.size() > 0);
			size_t size = edge.GetCurrTargetPhrase().GetSize();
			for (size_t pos = 0; pos < size; pos++) {
				const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
				oracle << *factor;
				oracle << " ";
			}
			std::string sourceP = edge.GetSourcePhraseStringRep();  // Source Phrase
			std::string targetP = edge.GetTargetPhraseStringRep();  // Target Phrase
			if(!has_only_spaces(sourceP) && !has_only_spaces(targetP) )
			{
				PP_ORACLE[sourceP][targetP]=1;
				Insert(sourceP, targetP);
			}
		}
		oracleScore=path.GetTotalScore();
		OracleList.push_back(PP_ORACLE);
		float oraclebleu = GetBleu(oracle.str(), m_postedited);
		if(m_mira || update_weights){
			HypothesisList.push_back(oracle.str());
			BleuScore.push_back(oraclebleu);
			featureValue.push_back(path.GetScoreBreakdown());
			modelScore.push_back(oracleScore);
		}

		if(oraclebleu > maxBleu)
		{
			cerr<<"NBEST : "<<oracle.str()<<"\t|||\tBLEU : "<<oraclebleu<<endl;
			maxBleu=oraclebleu;
			maxScore=oracleScore;
			bestOracle = oracle.str();
			pp_list::const_iterator it1;
			ShootemUp.clear();ShootemDown.clear();
			for(it1=PP_ORACLE.begin(); it1!=PP_ORACLE.end(); it1++)
			{
				std::map<std::string, int>::const_iterator itr1;
				for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
				{
					if(PP_BEST[it1->first][itr1->first]!=1)
					{
						ShootemUp[it1->first][itr1->first]=1;
					}
				}
			}
			for(it1=PP_BEST.begin(); it1!=PP_BEST.end(); it1++)
			{
				std::map<std::string, int>::const_iterator itr1;
				for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
				{
					if(PP_ORACLE[it1->first][itr1->first]!=1)
					{
						ShootemDown[it1->first][itr1->first]=1;
					}
				}
			}
			oracleBleuScores.clear();
			oraclefeatureScore.clear();
			BestOracle=PP_ORACLE;
			oracleBleuScores.push_back(oraclebleu);
			oraclefeatureScore.push_back(path.GetScoreBreakdown());
		}
	}
//	Update the features
	if(update_features && m_perceptron){
		pp_list::const_iterator it1;
		for(it1=ShootemUp.begin(); it1!=ShootemUp.end(); it1++)
		{
			std::map<std::string, int>::const_iterator itr1;
			for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
			{
				ShootUp(it1->first, itr1->first, abs(maxScore-bestScore));
				cerr<<"SHOOTING UP : "<<it1->first<<" ||| "<<itr1->first<<" ||| "<<maxScore-bestScore<<"\n";
			}
		}
		for(it1=ShootemDown.begin(); it1!=ShootemDown.end(); it1++)
		{
			std::map<std::string, int>::const_iterator itr1;
			for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
			{
				ShootDown(it1->first, itr1->first, abs(maxScore-bestScore));
				cerr<<"SHOOTING DOWN : "<<it1->first<<" ||| "<<itr1->first<<" ||| "<<maxScore-bestScore<<"\n";
			}
		}
	}
//	---------------------------------------------------------------------------------- //

	if(update_features && m_mira)
	{
		for (int i=0;i<HypothesisList.size();i++) // same loop used for feature values, modelscores
		{
			float bleuscore = BleuScore[i];
			loss.push_back(maxBleu-bleuscore);
		}
		vector<vector<int> > FeatureIdxVec, oracleFeatureIdxVec;
		pp_list::const_iterator it1;
		for(int i=0;i<OracleList.size();i++){
			for(it1=OracleList[i].begin(); it1!=OracleList[i].end(); it1++)
			{
				std::map<std::string, int>::const_iterator itr1;
				for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
				{
					int idx=RetrieveIdx(it1->first, itr1->first);
					if(idx!=m_featureIdx.size()){
						FeatureIdxVec[i].push_back(idx);
					}
				}
			}
		}
		for(it1=BestOracle.begin(); it1!=BestOracle.end(); it1++)
		{
			std::map<std::string, int>::const_iterator itr1;
			for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
			{
				int idx=RetrieveIdx(it1->first, itr1->first);
				if(idx!=m_featureIdx.size()){
					oracleFeatureIdxVec[0].push_back(idx);
				}
			}
		}
		size_t update_status=optimiser->updateFeatures(sparsevector, FeatureIdxVec, loss, BleuScore,
				modelScore, oracleFeatureIdxVec, oracleBleuScores[0], maxScore, flr);
	}

//	---------------------------------------------------------------------------------- //

//	Update the weights
	if(update_weights)
	{
		for (int i=0;i<HypothesisList.size();i++) // same loop used for feature values, modelscores
		{
			float bleuscore = BleuScore[i];
			loss.push_back(maxBleu-bleuscore);
		}
		modelScores.push_back(modelScore);
		featureValues.push_back(featureValue);
		BleuScores.push_back(BleuScore);
		losses.push_back(loss);
		oracleModelScores.push_back(maxScore);
		size_t update_status = optimiser->updateWeights(weightUpdate,sp,featureValues, losses,
				BleuScores, modelScores, oraclefeatureScore,oracleBleuScores, oracleModelScores,wlr);
		cerr<<"\nWeight : "<<weightUpdate.GetScoreForProducer(sp)<<"\n";
		StaticData::InstanceNonConst().SetAllWeights(weightUpdate);
	}
	return;
}

}
