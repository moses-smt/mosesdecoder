#include "Point.h"
#include<cmath>

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
