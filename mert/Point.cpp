#include "Point.h"
#include<cmath>
#include<cstdlib>
#include <cassert>
using namespace std;


vector<unsigned> Point::optindices;

unsigned Point::dim=0;

map<unsigned,statscore_t> Point::fixedweights;
  
unsigned Point::pdim=0;
unsigned Point::ncall=0;

void Point::Randomize(const vector<parameter_t>& min,const vector<parameter_t>& max){
  assert(min.size()==Point::dim);
  assert(max.size()==Point::dim);
  for (unsigned int i=0; i<size(); i++)
    operator[](i)= min[i] + (float)random()/(float)RAND_MAX * (float)(max[i]-min[i]);
}

void Point::NormalizeL2(){
  parameter_t norm=0.0;
  for (unsigned int i=0; i<size(); i++)
    norm+= operator[](i)*operator[](i);
		if(norm!=0.0){
    norm=sqrt(norm);
    for (unsigned int i=0; i<size(); i++)
      operator[](i)/=norm;
  }
}


void Point::NormalizeL1(){
  parameter_t norm=0.0;
  for (unsigned int i=0; i<size(); i++)
    norm+= abs(operator[](i));
		if(norm!=0.0){
			for (unsigned int i=0; i<size(); i++)
				operator[](i)/=norm;
		}
}

//Can initialize from a vector of dim or pdim
Point::Point(const vector<parameter_t>& init):vector<parameter_t>(Point::dim){
  if(init.size()==dim){
    for (unsigned int i=0; i<Point::dim; i++)
      operator[](i)=init[i];
  }else{
    assert(init.size()==pdim);
    for (unsigned int i=0; i<Point::dim; i++)
      operator[](i)=init[optindices[i]];
  }
};


double Point::operator*(const FeatureStats& F)const{
  ncall++;//to track performance
  double prod=0.0;
  if(OptimizeAll())
    for (unsigned i=0; i<size(); i++)
      prod+= operator[](i)*F.get(i);
  else{
    for (unsigned i=0; i<size(); i++)
      prod+= operator[](i)*F.get(optindices[i]);
    for(map<unsigned,float >::iterator it=fixedweights.begin();it!=fixedweights.end();it++)
      prod+=it->second*F.get(it->first);
  }
  return prod;
}
Point Point::operator+(const Point& p2)const{
  assert(p2.size()==size());
  Point Res(*this);
  for(unsigned i=0;i<size();i++)
    Res[i]+=p2[i];
  Res.score=numeric_limits<statscore_t>::max();
  return Res;
};

Point Point::operator*(float l)const{
  Point Res(*this);
  for(unsigned i=0;i<size();i++)
    Res[i]*=l;
  Res.score=numeric_limits<statscore_t>::max();
  return Res;
};

 ostream& operator<<(ostream& o,const Point& P){
   vector<parameter_t> w=P.GetAllWeights();
//	 o << "[" << Point::pdim << "] ";
	 for(unsigned int i=0;i<Point::pdim;i++)
     o << w[i] << " ";
//	 o << "=> " << P.GetScore();
	 return o;
};

vector<parameter_t> Point::GetAllWeights()const{
  vector<parameter_t> w;
  if(OptimizeAll()){
    w=*this;
  }else{
    w.resize(pdim);
    for (unsigned int i=0; i<size(); i++)
      w[optindices[i]]=operator[](i);
      for(map<unsigned,float >::iterator it=fixedweights.begin();it!=fixedweights.end();it++)
        w[it->first]=it->second;
  }
  return w;
};


 
