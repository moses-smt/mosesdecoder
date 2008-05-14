#include "Point.h"
#include<cmath>
#include <cassert>
using namespace std;
void Point::randomize(const vector<lambda>& min,const vector<lambda>& max){
  for (int i=0; i<size(); i++)
    operator[](i)= min[i] + (float)random()/RAND_MAX * (max[i]-min[i]);
}

void Point::normalize(){
  lambda norm=0.0;
  for (int i=0; i<size(); i++)
    norm+= operator[](i)*operator[](i);
  if(norm!=0.0){
    norm=sqrt(norm);
    for (int i=0; i<size(); i++)
      operator[](i)/=norm;
  }
}

double Point::operator*(FeatureStats& F)const{
  double prod=0.0;
for (int i=0; i<size(); i++)
    prod+= operator[](i)*F.get(i);
}

Point Point::operator+(const Point& p2)const{
  assert(p2.size()==size());
  Point Res(*this);
  for(unsigned i=0;i<size();i++)
    Res[i]+=p2[i];
  Res.score=numeric_limits<statscore>::max();
  return Res;
};

Point Point::operator*(float& l)const{
  Point Res(*this);
  for(unsigned i=0;i<size();i++)
    Res[i]*=l;
  Res.score=numeric_limits<statscore>::max();
  return Res;
};
