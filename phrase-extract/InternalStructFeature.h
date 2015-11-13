#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <queue>
#include <map>
#include <cmath>

#include "ScoreFeature.h"
#include "extract-ghkm/Node.h"

namespace MosesTraining
{


class InternalStructFeature : public ScoreFeature
{
public:
  InternalStructFeature() : m_type(0) {};
  /** Add the values for this feature function. */
  void add(const ScoreFeatureContext& context,
           std::vector<float>& denseValues,
           std::map<std::string,float>& sparseValues) const;


protected:
  /** Overridden in subclass */
  virtual void add(const std::string *treeFragment,
                   float count,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const = 0;
  int m_type;
};

class InternalStructFeatureDense : public InternalStructFeature
{
public:
  InternalStructFeatureDense()
    :InternalStructFeature() {
    m_type=1;
  } //std::cout<<"InternalStructFeatureDense: Construct "<<m_type<<"\n";}
protected:
  virtual void add(const std::string *treeFragment,
                   float count,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};

class InternalStructFeatureSparse : public InternalStructFeature
{
public:
  InternalStructFeatureSparse()
    :InternalStructFeature() {
    m_type=2;
  }// std::cout<<"InternalStructFeatureSparse: Construct "<<m_type<<"\n";}
protected:
  virtual void add(const std::string *treeFragment,
                   float count,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};

}
