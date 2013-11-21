#include "ExternalFeature.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

namespace Moses
{
ExternalFeatureState::ExternalFeatureState(int stateSize, void *data)
{
  m_stateSize = stateSize;
  m_data = malloc(stateSize);
  memcpy(m_data, data, stateSize);
}

void ExternalFeature::Load()
{
  string nparam = "testing";

  if (m_path.size() < 1) {
    cerr << "External requires a path to a dynamic library!\n";
    abort();
  }
  lib_handle = dlopen(m_path.c_str(), RTLD_LAZY);
  if (!lib_handle) {
    cerr << "dlopen reports: " << dlerror() << endl;
    cerr << "Did you provide a full path to the dynamic library?\n";
    abort();
  }
  CdecFF* (*fn)(const string&) =
    (CdecFF* (*)(const string&))(dlsym(lib_handle, "create_ff"));
  if (!fn) {
    cerr << "dlsym reports: " << dlerror() << endl;
    abort();
  }
  ff_ext = (*fn)(nparam);
  m_stateSize = ff_ext->StateSize();

}

ExternalFeature::~ExternalFeature()
{
  delete ff_ext;
  dlclose(lib_handle);
}

void ExternalFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_path = value;
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

FFState* ExternalFeature::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  return new ExternalFeatureState(m_stateSize);
}

FFState* ExternalFeature::EvaluateChart(
  const ChartHypothesis& /* cur_hypo */,
  int /* featureID - used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
  return new ExternalFeatureState(m_stateSize);
}


}

