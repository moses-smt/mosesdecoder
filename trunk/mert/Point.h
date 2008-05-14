#ifndef POINT_H
#define POINT_H
#include <vector>
#include "FeatureStats.h"
typedef float lambda,statscore;

class Point: public std::vector<lambda>{
 public:
  statscore score;
  Point(unsigned s):std::vector<lambda>(s,0.0){};
  Point(vector<lambda> init):vector<lambda>(init),score(numeric_limits<statscore>::max()){};
  void randomize(const std::vector<lambda>&min,const std::vector<lambda>& max);
  double operator*(FeatureStats&)const;//compute the feature function
  void normalize();
};

#endif
