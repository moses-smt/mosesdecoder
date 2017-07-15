#include "InternalStructFeature.h"
#include <map>

using namespace std;

namespace MosesTraining
{

void InternalStructFeature::add(const ScoreFeatureContext& context,
                                std::vector<float>& denseValues,
                                std::map<std::string,float>& sparseValues) const
{
  const std::map<std::string,float> *allTrees = context.phrasePair.GetProperty("Tree"); // our would we rather want to take the most frequent one only?
  for ( std::map<std::string,float>::const_iterator iter=allTrees->begin();
        iter!=allTrees->end(); ++iter ) {
    add(&(iter->first), iter->second, denseValues, sparseValues);
  }
}

void InternalStructFeatureDense::add(const std::string *treeFragment,
                                     float count,
                                     std::vector<float>& denseValues,
                                     std::map<std::string,float>& sparseValues) const
{
  //cout<<"Dense: "<<*internalStruct<<endl;
  size_t start=0;
  int countNP=0;
  while((start = treeFragment->find("NP", start)) != string::npos) {
    countNP += count;
    start+=2; //length of "NP"
  }
  //should add e^countNP so in the decoder I get log(e^countNP)=countNP -> but is log or ln?
  //should use this but don't know what it does? -> maybeLog( (bitmap == i) ? 2.718 : 1 )
  denseValues.push_back(exp(countNP));

}

void InternalStructFeatureSparse::add(const std::string *treeFragment,
                                      float count,
                                      std::vector<float>& denseValues,
                                      std::map<std::string,float>& sparseValues) const
{
  //cout<<"Sparse: "<<*internalStruct<<endl;
  if(treeFragment->find("VBZ")!=std::string::npos)
    sparseValues["NTVBZ"] += count;
  if(treeFragment->find("VBD")!=std::string::npos)
    sparseValues["NTVBD"] += count;
  if(treeFragment->find("VBP")!=std::string::npos)
    sparseValues["NTVBP"] += count;
  if(treeFragment->find("PP")!=std::string::npos)
    sparseValues["NTPP"] += count;
  if(treeFragment->find("SBAR")!=std::string::npos)
    sparseValues["NTSBAR"] += count;
}


}
