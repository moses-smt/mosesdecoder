#include "Point.h"

#include <cmath>
#include <cstdlib>
#include "util/exception.hh"
#include "FeatureStats.h"
#include "Optimizer.h"

using namespace std;

namespace MosesTuning
{

vector<unsigned> Point::m_opt_indices;

unsigned Point::m_dim = 0;

map<unsigned,statscore_t> Point::m_fixed_weights;

unsigned Point::m_pdim = 0;
unsigned Point::m_ncall = 0;

vector<parameter_t> Point::m_min;
vector<parameter_t> Point::m_max;

Point::Point() : vector<parameter_t>(m_dim), m_score(0.0) {}

//Can initialize from a vector of dim or m_pdim
Point::Point(const vector<parameter_t>& init,
             const vector<parameter_t>& min,
             const vector<parameter_t>& max)
  : vector<parameter_t>(Point::m_dim), m_score(0.0)
{
  m_min.resize(Point::m_dim);
  m_max.resize(Point::m_dim);
  if (init.size() == m_dim) {
    for (unsigned int i = 0; i < Point::m_dim; i++) {
      operator[](i) = init[i];
      m_min[i] = min[i];
      m_max[i] = max[i];
    }
  } else {
	UTIL_THROW_IF(init.size() != m_pdim, util::Exception, "Error");
	UTIL_THROW_IF(m_opt_indices.size() != Point::m_dim, util::Exception, "Error");
    for (unsigned int i = 0; i < Point::m_dim; i++) {
      operator[](i) = init[m_opt_indices[i]];
      m_min[i] = min[m_opt_indices[i]];
      m_max[i] = max[m_opt_indices[i]];
    }
  }
}

Point::~Point() {}

void Point::Randomize()
{
  UTIL_THROW_IF(m_min.size() != Point::m_dim, util::Exception, "Error");
  UTIL_THROW_IF(m_max.size() != Point::m_dim, util::Exception, "Error");

  for (unsigned int i = 0; i < size(); i++) {
    operator[](i) = m_min[i] +
                    static_cast<float>(random()) / static_cast<float>(RAND_MAX) * (m_max[i] - m_min[i]);
  }
}

double Point::operator*(const FeatureStats& F) const
{
  m_ncall++; // to track performance
  double prod = 0.0;
  if (OptimizeAll())
    for (unsigned i=0; i<size(); i++)
      prod += operator[](i) * F.get(i);
  else {
    for (unsigned i = 0; i < size(); i++)
      prod += operator[](i) * F.get(m_opt_indices[i]);
    for(map<unsigned, float>::iterator it = m_fixed_weights.begin();
        it != m_fixed_weights.end(); ++it)
      prod += it->second * F.get(it->first);
  }
  return prod;
}

const Point Point::operator+(const Point& p2) const
{
  UTIL_THROW_IF(p2.size() != size(), util::Exception, "Error");

  Point Res(*this);
  for (unsigned i = 0; i < size(); i++) {
    Res[i] += p2[i];
  }

  Res.m_score = kMaxFloat;
  return Res;
}

void Point::operator+=(const Point& p2)
{
  UTIL_THROW_IF(p2.size() != size(), util::Exception, "Error");
  for (unsigned i = 0; i < size(); i++) {
    operator[](i) += p2[i];
  }
  m_score = kMaxFloat;
}

const Point Point::operator*(float l) const
{
  Point Res(*this);
  for (unsigned i = 0; i < size(); i++) {
    Res[i] *= l;
  }
  Res.m_score = kMaxFloat;
  return Res;
}

ostream& operator<<(ostream& o, const Point& P)
{
  vector<parameter_t> w;
  P.GetAllWeights(w);
  for (unsigned int i = 0; i < Point::m_pdim; i++) {
    o << w[i] << " ";
  }
  return o;
}

void Point::NormalizeL2()
{
  parameter_t norm=0.0;
  for (unsigned int i = 0; i < size(); i++)
    norm += operator[](i) * operator[](i);
  if (norm != 0.0) {
    norm = sqrt(norm);
    for (unsigned int i = 0; i < size(); i++)
      operator[](i) /= norm;
  }
}


void Point::NormalizeL1()
{
  parameter_t norm = 0.0;
  for (unsigned int i = 0; i < size(); i++)
    norm += abs(operator[](i));
  if (norm != 0.0) {
    for (unsigned int i = 0; i < size(); i++)
      operator[](i) /= norm;
  }
}


void Point::GetAllWeights(vector<parameter_t>& w) const
{
  if (OptimizeAll()) {
    w = *this;
  } else {
    w.resize(m_pdim);
    for (unsigned int i = 0; i < size(); i++)
      w[m_opt_indices[i]] = operator[](i);
    for (map<unsigned,float>::const_iterator it = m_fixed_weights.begin();
         it != m_fixed_weights.end(); ++it) {
      w[it->first]=it->second;
    }
  }
}

}

