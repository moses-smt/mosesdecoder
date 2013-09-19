#include "Point.h"

#include <cmath>
#include <cstdlib>
#include "util/check.hh"
#include <limits>
#include "FeatureStats.h"

using namespace std;

vector<unsigned> Point::optindices;

unsigned Point::dim = 0;

map<unsigned,statscore_t> Point::fixedweights;

unsigned Point::pdim = 0;
unsigned Point::ncall = 0;

vector<parameter_t> Point::m_min;
vector<parameter_t> Point::m_max;

Point::Point() : vector<parameter_t>(dim), score_(0.0) {}

//Can initialize from a vector of dim or pdim
Point::Point(const vector<parameter_t>& init,
             const vector<parameter_t>& min,
             const vector<parameter_t>& max)
    : vector<parameter_t>(Point::dim), score_(0.0)
{
  m_min.resize(Point::dim);
  m_max.resize(Point::dim);
  if(init.size()==dim) {
    for (unsigned int i=0; i<Point::dim; i++) {
      operator[](i)=init[i];
      m_min[i] = min[i];
      m_max[i] = max[i];
    }
  } else {
    CHECK(init.size()==pdim);
    for (unsigned int i=0; i<Point::dim; i++) {
      operator[](i)=init[optindices[i]];
      m_min[i] = min[optindices[i]];
      m_max[i] = max[optindices[i]];
    }
  }
}

Point::~Point() {}

void Point::Randomize()
{
  CHECK(m_min.size()==Point::dim);
  CHECK(m_max.size()==Point::dim);
  for (unsigned int i=0; i<size(); i++) {
    operator[](i) = m_min[i] +
                   (float)random()/(float)RAND_MAX * (float)(m_max[i]-m_min[i]);
  }
}

double Point::operator*(const FeatureStats& F) const
{
  ncall++; // to track performance
  double prod=0.0;
  if(OptimizeAll())
    for (unsigned i=0; i<size(); i++)
      prod+= operator[](i)*F.get(i);
  else {
    for (unsigned i=0; i<size(); i++)
      prod+= operator[](i)*F.get(optindices[i]);
    for(map<unsigned,float >::iterator it=fixedweights.begin(); it!=fixedweights.end(); it++)
      prod+=it->second*F.get(it->first);
  }
  return prod;
}

Point Point::operator+(const Point& p2) const
{
  CHECK(p2.size() == size());
  Point Res(*this);
  for (unsigned i = 0; i < size(); i++) {
    Res[i] += p2[i];
  }

  Res.score_ = numeric_limits<statscore_t>::max();
  return Res;
}

void Point::operator+=(const Point& p2)
{
  CHECK(p2.size() == size());
  for (unsigned i = 0; i < size(); i++) {
    operator[](i) += p2[i];
  }
  score_ = numeric_limits<statscore_t>::max();
}

Point Point::operator*(float l) const
{
  Point Res(*this);
  for (unsigned i = 0; i < size(); i++) {
    Res[i] *= l;
  }
  Res.score_ = numeric_limits<statscore_t>::max();
  return Res;
}

ostream& operator<<(ostream& o, const Point& P)
{
  vector<parameter_t> w = P.GetAllWeights();
  for (unsigned int i = 0; i < Point::pdim; i++) {
    o << w[i] << " ";
  }
  return o;
}

void Point::NormalizeL2()
{
  parameter_t norm=0.0;
  for (unsigned int i=0; i<size(); i++)
    norm+= operator[](i)*operator[](i);
  if(norm!=0.0) {
    norm=sqrt(norm);
    for (unsigned int i=0; i<size(); i++)
      operator[](i)/=norm;
  }
}


void Point::NormalizeL1()
{
  parameter_t norm=0.0;
  for (unsigned int i=0; i<size(); i++)
    norm+= abs(operator[](i));
  if(norm!=0.0) {
    for (unsigned int i=0; i<size(); i++)
      operator[](i)/=norm;
  }
}


vector<parameter_t> Point::GetAllWeights()const
{
  vector<parameter_t> w;
  if(OptimizeAll()) {
    w=*this;
  } else {
    w.resize(pdim);
    for (unsigned int i=0; i<size(); i++)
      w[optindices[i]]=operator[](i);
    for(map<unsigned,float >::iterator it=fixedweights.begin(); it!=fixedweights.end(); it++)
      w[it->first]=it->second;
  }
  return w;
}
