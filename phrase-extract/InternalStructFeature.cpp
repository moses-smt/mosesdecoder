#include "InternalStructFeature.h"

using namespace std;

namespace MosesTraining
{

InternalStructFeature::InternalStructFeature()
	:m_type(0){
	//cout<<"InternalStructFeature: Construct "<<m_type<<"\n";
}

bool InternalStructFeature::equals(const PhraseAlignment& lhs, const PhraseAlignment& rhs) const{
	//cout<<"InternalStructFeature: Equals\n";
	//don't know what it's used for and what we should compare
	//-> if the dense score is the same
	//-> if the sparse feature is set
	// compare phrases? with the internalStrucutre string?
	/** Return true if the two phrase pairs are equal from the point of this feature. Assume
	      that they already compare true according to PhraseAlignment.equals()
	   **/

/*	if(lhs.ghkmParse==rhs.ghkmParse)
		return true;
	else
		return false;
*/
	//return true;
}

void InternalStructFeature::add(const ScoreFeatureContext& context,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	for(size_t i=0; i<context.phrasePair.size(); i++) {
		add(&context.phrasePair[i]->treeFragment, denseValues, sparseValues);
	}

}

void InternalStructFeatureDense::add(std::string *internalStruct,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	//cout<<"Dense: "<<*internalStruct<<endl;
	size_t start=0;
	int countNP=0;
	while((start = internalStruct->find("NP", start)) != string::npos) {
		countNP++;
		start+=2; //length of "NP"
	}
	//should add e^countNP so in the decoder I get log(e^countNP)=countNP -> but is log or ln?
	//should use this but don't know what it does? -> maybeLog( (bitmap == i) ? 2.718 : 1 )
	denseValues.push_back(exp(countNP));

}

void InternalStructFeatureSparse::add(std::string *internalStruct,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	//cout<<"Sparse: "<<*internalStruct<<endl;
	if(internalStruct->find("VBZ")!=std::string::npos)
		sparseValues["NTVBZ"] = 1;
	if(internalStruct->find("VBD")!=std::string::npos)
			sparseValues["NTVBD"] = 1;
	if(internalStruct->find("VBP")!=std::string::npos)
				sparseValues["NTVBP"] = 1;
	if(internalStruct->find("PP")!=std::string::npos)
				sparseValues["NTPP"] = 1;
	if(internalStruct->find("SBAR")!=std::string::npos)
				sparseValues["NTSBAR"] = 1;

}


}
