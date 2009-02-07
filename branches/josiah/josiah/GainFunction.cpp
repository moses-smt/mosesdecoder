#include "GainFunction.h"

#include "Phrase.h"
#include "Factor.h"

using namespace std;
using namespace Moses;

namespace Josiah {

void GainFunction::ConvertStringToFactorArray(const string& str, vector<const Factor*>* out) {
  Phrase phrase(Output);
  vector<FactorType> ft(1, 0);
  phrase.CreateFromString(ft, str, "|");
  out->resize(phrase.GetSize());
  FactorType type = ft.front();
  for (unsigned i = 0; i < phrase.GetSize(); ++i)
    (*out)[i] = phrase.GetFactor(i, type);
}

GainFunction::~GainFunction() {}

}

