#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <map>
#include <cmath>

#include "ScoreFeature.h"
#include "extract-ghkm/Node.h"

using namespace MosesTraining;
using namespace Moses;
using namespace GHKM;

namespace MosesTraining
{


class InternalStructFeature : public ScoreFeature
{
public:
	InternalStructFeature();
	/** Return true if the two phrase pairs are equal from the point of this feature. Assume
	      that they already compare true according to PhraseAlignment.equals()
	   **/
	bool equals(const PhraseAlignment& lhs, const PhraseAlignment& rhs) const;
	/** Add the values for this feature function. */
	void add(const ScoreFeatureContext& context,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const;


protected:
	/** Overriden in subclass */
	 virtual void add(std::string *internalStruct,
	                   std::vector<float>& denseValues,
	                   std::map<std::string,float>& sparseValues) const = 0;
	int m_type;

};

class InternalStructFeatureDense : public InternalStructFeature
{
public:
	InternalStructFeatureDense()
		:InternalStructFeature(){m_type=1;} //std::cout<<"InternalStructFeatureDense: Construct "<<m_type<<"\n";}
protected:
	virtual void add(std::string *internalStruct,
		             std::vector<float>& denseValues,
		             std::map<std::string,float>& sparseValues) const;
};

class InternalStructFeatureSparse : public InternalStructFeature
{
public:
	InternalStructFeatureSparse()
		:InternalStructFeature(){m_type=2;}// std::cout<<"InternalStructFeatureSparse: Construct "<<m_type<<"\n";}
protected:
	virtual void add(std::string *internalStruct,
		             std::vector<float>& denseValues,
		             std::map<std::string,float>& sparseValues) const;
};

}
