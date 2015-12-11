#pragma once

#include <string>
#include <cstring>
#include <cstdlib>
#include "StatefulFeatureFunction.h"
#include "FFState.h"

namespace Moses
{
class CdecFF;

class ExternalFeatureState : public FFState
{
protected:
  int m_stateSize;
  void *m_data;
public:
  ExternalFeatureState(int stateSize)
    :m_stateSize(stateSize)
    ,m_data(NULL) {
  }
  ExternalFeatureState(int stateSize, void *data);

  ~ExternalFeatureState() {
    free(m_data);
  }

  int Compare(const FFState& other) const {
    const ExternalFeatureState &otherFF = static_cast<const ExternalFeatureState&>(other);
    int ret = memcmp(m_data, otherFF.m_data, m_stateSize);
    return ret;
  }
};

// copied from cdec
class ExternalFeature : public StatefulFeatureFunction
{
public:
  ExternalFeature(const std::string &line)
    :StatefulFeatureFunction(line) {
    ReadParameters();
  }
  ~ExternalFeature();

  void Load(AllOptions const& opts);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void SetParameter(const std::string& key, const std::string& value);

  FFState* EvaluateWhenApplied(const Hypothesis& cur_hypo, const FFState* prev_state,
                               ScoreComponentCollection* accumulator) const;

  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  virtual const FFState* EmptyHypothesisState(const InputType &input) const {
    return new ExternalFeatureState(m_stateSize);
  }

protected:
  std::string m_path;
  void* lib_handle;
  CdecFF *ff_ext;
  int m_stateSize;
};

class CdecFF
{
public:
  virtual ~CdecFF() {}
  virtual int StateSize() const = 0;
};

}

