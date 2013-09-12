#include "InternalStructFeature.h"

using namespace std;

namespace MosesTraining
{

InternalStructFeature::InternalStructFeature()
	:m_type(0){
	cout<<"InternalStructFeature: Construct "<<m_type<<"\n";
}

bool InternalStructFeature::equals(const PhraseAlignment& lhs, const PhraseAlignment& rhs) const{
	cout<<"InternalStructFeature: Equals\n";
	//don't know what it's used for and what we should compare
	//-> if the dense score is the same
	//-> if the sparse feature is set
	// compare phrases? with the internalStrucutre string?
	/** Return true if the two phrase pairs are equal from the point of this feature. Assume
	      that they already compare true according to PhraseAlignment.equals()
	   **/
	return true;
}

void InternalStructFeature::add(const ScoreFeatureContext& context,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	std::string *internalStruct=new string("(NP((DT)(NN)))");
	add(internalStruct, denseValues, sparseValues);
}

void InternalStructFeatureDense::add(std::string *internalStruct,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	cout<<internalStruct<<endl;

}

void InternalStructFeatureSparse::add(std::string *internalStruct,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const{
	cout<<internalStruct<<endl;
}


}
