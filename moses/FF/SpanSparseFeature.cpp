#include <boost/shared_ptr.hpp>
#include "SpanSparseFeature.h"
#include "moses/StaticData.h"
#include "moses/Word.h"
#include "moses/ChartCellLabel.h"
#include "moses/WordsRange.h"
#include "moses/StackVec.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/PhraseProperty.h"
#include "moses/WordsRange.h"
#include "moses/ChartHypothesis.h"
#include "moses/InputPath.h"
#include "moses/TargetPhrase.h"
#include <iostream>

using namespace std;

namespace Moses
{
SpanSparseFeature::SpanSparseFeature(const std::string &line)
:StatelessFeatureFunction(0, line)
{
  ReadParameters();
}

void SpanSparseFeature::SetParameter(const std::string& key, const std::string& value){
	if (key == "tuneable") {
	    m_tuneable = Scan<bool>(value);
	  }
	else {
	    UTIL_THROW(util::Exception, "Unknown argument " << key << "=" << value);
	  }
}

void SpanSparseFeature::EvaluateInIsolation(const Phrase &source
						, const TargetPhrase &targetPhrase
						, ScoreComponentCollection &scoreBreakdown
						, ScoreComponentCollection &estimatedFutureScore) const{
	  //TAKE THIS OUT IF NOT USING IT
	  //targetPhrase.SetRuleSource(source);
}

//what the sparse feature should encode
// NT label + target span
//NT label + target span + source span
//NT label + child label ? + target span
//NT label + span + position in the sentence -> given right branching you should have
//a large span towards the end of the sentence not starting at the begining of the sentence
//and stopping 3 words before the end
//NT label + target span + length of source sentence -> if source is 10 and I get and S of 2 words might be wrong ?
void SpanSparseFeature::EvaluateWhenApplied(const ChartHypothesis &cur_hypo,
	                             ScoreComponentCollection* accumulator) const{

	//Get the translation rule that created the current hypothesis
	const ChartTranslationOption &transOpt = cur_hypo.GetTranslationOption();

	//Get rule LHS
	const Word &ntLHS = cur_hypo.GetTargetLHS();
	const std::vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
	std::string LHS_string = ntLHS.GetString(outputFactorOrder, false);

	//Get the source span
	const WordsRange &ntRange = cur_hypo.GetCurrSourceRange();
	size_t spanSource = ntRange.GetEndPos()-ntRange.GetStartPos()+1;

	//Get the target span
	Phrase outputPhrase = cur_hypo.GetOutputPhrase();
	size_t spanTarget = outputPhrase.GetSize();

	//Compose the sparse feature name
	stringstream nameTS,nameSTS,nameSS,nameSSb,nameTSb,nameSTSdiff;
	//nameSS <<LHS_string<<"_S_"<<spanSource;
	nameSSb <<LHS_string<<"_Sb_"<<spanSource/3;
	//nameTS <<LHS_string<<"_T_"<<spanTarget;
	nameTSb <<LHS_string<<"_Tb_"<<spanTarget/3;
	//nameSTS <<LHS_string<<"_"<<spanSource<<"_"<<spanTarget;
	nameSTSdiff <<LHS_string<<"_d_"<<int(spanSource-spanTarget);


	//Add sparse features
	//accumulator->PlusEquals(this,nameSS.str(),1); //LHS with the source span length
	//accumulator->PlusEquals(this,nameTS.str(),1); //LHS with the target span length
	//accumulator->PlusEquals(this,nameSTS.str(),1); //LHS with the source and target span length
	accumulator->PlusEquals(this,nameSSb.str(),1); //LHS with the source span length bined
	accumulator->PlusEquals(this,nameTSb.str(),1); //LHS with the target span length bined
	accumulator->PlusEquals(this,nameSTSdiff.str(),1); //LHS with difference of the source and target span length

	//Output all kind of information about the current rule and hypothesis
/*
	cout<<"Target rule: "<<transOpt.GetPhrase()<<endl;
	//TODO assert(transOpt.GetPhrase().GetRuleSource())
	cout<<"Source rule: "<<*transOpt.GetPhrase().GetRuleSource()<<endl;
	cout<<"Source phrase: "<<transOpt.GetInputPath()->GetPhrase()<<endl;
	cout<<"Source phrase length: "<<spanSource<<endl;
	cout<<"LHS label: "<<LHS_string<<endl;
	cout<<"Output phrase: "<<outputPhrase<<endl;
	cout<<"Output phrase length: "<<spanTarget<<endl;

	cout<<nameSS.str()<<endl;
	cout<<nameTS.str()<<endl;
	cout<<nameSTS.str()<<endl;
	cout<<nameSSb.str()<<endl;
	cout<<nameTSb.str()<<endl;
	cout<<nameSTSdiff.str()<<endl;
*/
}



}

