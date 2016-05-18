// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include "util/exception.hh"
#include "util/string_stream.hh"
#include "ScoreComponentCollection.h"
#include "StaticData.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/FF/StatefulFeatureFunction.h"

using namespace std;
using namespace boost::algorithm;

namespace Moses
{
void ScorePair::PlusEquals(const ScorePair &other)
{
  PlusEquals(other.denseScores);
  std::map<StringPiece, float>::const_iterator iter;
  for (iter = other.sparseScores.begin(); iter != other.sparseScores.end(); ++iter) {
    PlusEquals(iter->first, iter->second);
  }
}

void ScorePair::PlusEquals(const StringPiece &key, float value)
{
  std::map<StringPiece, float>::iterator iter;
  iter = sparseScores.find(key);
  if (iter == sparseScores.end()) {
    sparseScores[key] = value;
  } else {
    float &existingval = iter->second;
    existingval += value;
  }
}

std::ostream& operator<<(std::ostream& os, const ScorePair& rhs)
{
  for (size_t i = 0; i < rhs.denseScores.size(); ++i) {
    os << rhs.denseScores[i] << ",";
  }

  std::map<StringPiece, float>::const_iterator iter;
  for (iter = rhs.sparseScores.begin(); iter != rhs.sparseScores.end(); ++iter) {
    os << iter->first << "=" << iter->second << ",";
  }

  return os;
}

//ScoreComponentCollection::ScoreIndexMap ScoreComponentCollection::s_scoreIndexes;
size_t ScoreComponentCollection::s_denseVectorSize = 0;

ScoreComponentCollection::
ScoreComponentCollection()
  : m_scores(s_denseVectorSize)
{}


void
ScoreComponentCollection::
RegisterScoreProducer(FeatureFunction* scoreProducer)
{
  size_t start = s_denseVectorSize;
  s_denseVectorSize = scoreProducer->SetIndex(s_denseVectorSize);
  VERBOSE(1, "FeatureFunction: "
          << scoreProducer->GetScoreProducerDescription()
          << " start: " << start
          << " end: "   << (s_denseVectorSize-1) << endl);
}


float
ScoreComponentCollection::
GetWeightedScore() const
{
  return m_scores.inner_product(StaticData::Instance().GetAllWeights().m_scores);
}

void ScoreComponentCollection::MultiplyEquals(float scalar)
{
  m_scores *= scalar;
}

// Multiply all weights of this sparse producer by a given scalar
void ScoreComponentCollection::MultiplyEquals(const FeatureFunction* sp, float scalar)
{
  std::string prefix = sp->GetScoreProducerDescription() + FName::SEP;
  for(FVector::FNVmap::const_iterator i = m_scores.cbegin(); i != m_scores.cend(); i++) {
    const std::string &name = i->first.name();
    if (starts_with(name, prefix))
      m_scores[i->first] = i->second * scalar;
  }
}

// Count weights belonging to this sparse producer
size_t ScoreComponentCollection::GetNumberWeights(const FeatureFunction* sp)
{
  std::string prefix = sp->GetScoreProducerDescription() + FName::SEP;
  size_t weights = 0;
  for(FVector::FNVmap::const_iterator i = m_scores.cbegin(); i != m_scores.cend(); i++) {
    const std::string &name = i->first.name();
    if (starts_with(name, prefix))
      weights++;
  }
  return weights;
}

void ScoreComponentCollection::DivideEquals(float scalar)
{
  m_scores /= scalar;
}

void ScoreComponentCollection::CoreDivideEquals(float scalar)
{
  m_scores.coreDivideEquals(scalar);
}

void ScoreComponentCollection::DivideEquals(const ScoreComponentCollection& rhs)
{
  m_scores.divideEquals(rhs.m_scores);
}

void ScoreComponentCollection::MultiplyEquals(const ScoreComponentCollection& rhs)
{
  m_scores *= rhs.m_scores;
}

void ScoreComponentCollection::MultiplyEqualsBackoff(const ScoreComponentCollection& rhs, float backoff)
{
  m_scores.multiplyEqualsBackoff(rhs.m_scores, backoff);
}

void ScoreComponentCollection::MultiplyEquals(float core_r0, float sparse_r0)
{
  m_scores.multiplyEquals(core_r0, sparse_r0);
}

std::ostream& operator<<(std::ostream& os, const ScoreComponentCollection& rhs)
{
  os << rhs.m_scores;
  return os;
}
void ScoreComponentCollection::L1Normalise()
{
  m_scores /= m_scores.l1norm_coreFeatures();
}

float ScoreComponentCollection::GetL1Norm() const
{
  return m_scores.l1norm();
}

float ScoreComponentCollection::GetL2Norm() const
{
  return m_scores.l2norm();
}

float ScoreComponentCollection::GetLInfNorm() const
{
  return m_scores.linfnorm();
}

size_t ScoreComponentCollection::L1Regularize(float lambda)
{
  return m_scores.l1regularize(lambda);
}

void ScoreComponentCollection::L2Regularize(float lambda)
{
  m_scores.l2regularize(lambda);
}

size_t ScoreComponentCollection::SparseL1Regularize(float lambda)
{
  return m_scores.sparseL1regularize(lambda);
}

void ScoreComponentCollection::SparseL2Regularize(float lambda)
{
  m_scores.sparseL2regularize(lambda);
}

void ScoreComponentCollection::Save(ostream& out, bool multiline) const
{
  string sep = " ";
  string linesep = "\n";
  if (!multiline) {
    sep = "=";
    linesep = " ";
  }

  std::vector<FeatureFunction*> const& all_ff
  = FeatureFunction::GetFeatureFunctions();
  BOOST_FOREACH(FeatureFunction const* ff, all_ff) {
    string name = ff->GetScoreProducerDescription();
    size_t i = ff->GetIndex();
    if (ff->GetNumScoreComponents() == 1)
      out << name << sep << m_scores[i] << linesep;
    else {
      size_t stop = i + ff->GetNumScoreComponents();
      boost::format fmt("%s_%d");
      for (size_t k = 1; i < stop; ++i, ++k)
        out << fmt % name % k << sep << m_scores[i] << linesep;
    }
  }
  // write sparse features
  m_scores.write(out,sep,linesep);
}

void ScoreComponentCollection::Save(const string& filename) const
{
  ofstream out(filename.c_str());
  if (!out) {
    util::StringStream msg;
    msg << "Unable to open " << filename;
    throw runtime_error(msg.str());
  }
  Save(out);
  out.close();
}

void
ScoreComponentCollection::
Assign(const FeatureFunction* sp, const string &line)
{
  istringstream istr(line);
  while(istr) {
    string namestring;
    FValue value;
    istr >> namestring;
    if (!istr) break;
    istr >> value;
    FName fname(sp->GetScoreProducerDescription(), namestring);
    m_scores[fname] = value;
  }
}

void
ScoreComponentCollection::
Assign(const FeatureFunction* sp, const std::vector<float>& scores)
{
  size_t numScores = sp->GetNumScoreComponents();
  size_t offset = sp->GetIndex();

  if (scores.size() != numScores) {
    UTIL_THROW(util::Exception, "Feature function "
               << sp->GetScoreProducerDescription() << " specified "
               << numScores << " dense scores or weights. Actually has "
               << scores.size());
  }

  for (size_t i = 0; i < scores.size(); ++i) {
    m_scores[i + offset] = scores[i];
  }
}

void
ScoreComponentCollection::
Assign(const FeatureFunction* sp, size_t idx, float sc)
{
  size_t numScores = sp->GetNumScoreComponents();
  size_t offset = sp->GetIndex();

  if (idx >= numScores) {
    UTIL_THROW(util::Exception, "Feature function "
               << sp->GetScoreProducerDescription() << " specified index "
               << idx << " dense scores or weights. Actually has "
               << numScores);
  }

  m_scores[idx + offset] = sc;
}

void ScoreComponentCollection::InvertDenseFeatures(const FeatureFunction* sp)
{

  Scores old_scores = GetScoresForProducer(sp);
  Scores new_scores(old_scores.size());

  for (size_t i = 0; i != old_scores.size(); ++i) {
    new_scores[i] = -old_scores[i];
  }

  Assign(sp, new_scores);
}

void ScoreComponentCollection::ZeroDenseFeatures(const FeatureFunction* sp)
{
  size_t numScores = sp->GetNumScoreComponents();
  Scores vec(numScores, 0);

  Assign(sp, vec);
}

//! get subset of scores that belong to a certain sparse ScoreProducer
FVector
ScoreComponentCollection::
GetVectorForProducer(const FeatureFunction* sp) const
{
  FVector fv(s_denseVectorSize);
  std::string prefix = sp->GetScoreProducerDescription() + FName::SEP;
  for(FVector::FNVmap::const_iterator i = m_scores.cbegin(); i != m_scores.cend(); i++) {
    std::stringstream name;
    name << i->first;
    if (starts_with(name.str(), prefix))
      fv[i->first] = i->second;
  }
  return fv;
}

void ScoreComponentCollection::PlusEquals(const FeatureFunction* sp, const ScorePair &scorePair)
{
  PlusEquals(sp, scorePair.denseScores);

  std::map<StringPiece, float>::const_iterator iter;
  for (iter = scorePair.sparseScores.begin(); iter != scorePair.sparseScores.end(); ++iter) {
    const StringPiece &key = iter->first;
    float value = iter->second;
    PlusEquals(sp, key, value);
  }
}

void
ScoreComponentCollection::
OutputAllFeatureScores(std::ostream &out, bool with_labels) const
{
  std::string lastName = "";
  const vector<const StatefulFeatureFunction*>& sff
  = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for( size_t i=0; i<sff.size(); i++ ) {
    const StatefulFeatureFunction *ff = sff[i];
    if (ff->IsTuneable()) {
      OutputFeatureScores(out, ff, lastName, with_labels);
    }
  }
  const vector<const StatelessFeatureFunction*>& slf
  = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for( size_t i=0; i<slf.size(); i++ ) {
    const StatelessFeatureFunction *ff = slf[i];
    if (ff->IsTuneable()) {
      OutputFeatureScores(out, ff, lastName, with_labels);
    }
  }
}

void
ScoreComponentCollection::
OutputFeatureScores(std::ostream& out, FeatureFunction const* ff,
                    std::string &lastName, bool with_labels) const
{
  // const StaticData &staticData = StaticData::Instance();
  // bool labeledOutput = staticData.options().nbest.include_feature_labels;

  // regular features (not sparse)
  if (ff->HasTuneableComponents()) {
    if( with_labels && lastName != ff->GetScoreProducerDescription() ) {
      lastName = ff->GetScoreProducerDescription();
      out << " " << lastName << "=";
    }
    vector<float> scores = GetScoresForProducer( ff );
    for (size_t j = 0; j<scores.size(); ++j) {
      if (ff->IsTuneableComponent(j)) {
        out << " " << scores[j];
      }
    }
  }

  // sparse features
  const FVector scores = GetVectorForProducer( ff );
  for(FVector::FNVmap::const_iterator i = scores.cbegin(); i != scores.cend(); i++) {
    out << " " << i->first << "= " << i->second;
  }
}

}


