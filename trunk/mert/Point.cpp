#include "Point.h"
#include<cmath>
#include <cassert>
using namespace std;


vector<unsigned> Point::optindices;

unsigned Point::dim=0;

map<unsigned,statscore_t> Point::fixedweights;
  
unsigned Point::pdim=0;
unsigned Point::ncall=0;

void Point::Randomize(const parameters_t& min,const parameters_t& max){
  for (int i=0; i<size(); i++)
    operator[](i)= min[i] + (float)random()/(float)RAND_MAX * (float)(max[i]-min[i]);
}

void Point::Normalize(){
  parameter_t norm=0.0;
  for (int i=0; i<size(); i++)
    norm+= operator[](i)*operator[](i);
  if(norm!=0.0){
    norm=sqrt(norm);
    for (int i=0; i<size(); i++)
      operator[](i)/=norm;
  }
}

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
   parameters_t w=P.GetAllWeights();
   for(int i=0;i<Point::pdim;i++)
     o<<w[i]<<' ';
   o<<endl;
   return o;
};

parameters_t Point::GetAllWeights()const{
  parameters_t w;
  if(OptimizeAll()){
    w=*this;
  }else{
    w.resize(pdim);
    for (int i=0; i<size(); i++)
      w[optindices[i]]=operator[](i);
    for(map<unsigned,float >::iterator it=fixedweights.begin();it!=fixedweights.end();it++)
      w[it->first]=it->second;
  }
  return w;
};


 
