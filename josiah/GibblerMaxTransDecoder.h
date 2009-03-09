#pragma once

#include <vector>
#include <utility>
#include <ext/hash_map>

#include "Gibbler.h"
#include "ScoreComponentCollection.h"
#include "Phrase.h"

namespace Moses { class Factor; }

namespace __gnu_cxx {
  template<> struct hash<std::vector<const Moses::Factor*> > {
    inline size_t operator()(const std::vector<const Moses::Factor*>& p) const {
      static const int primes[] = {8933, 8941, 8951, 8963, 8969, 8971, 8999, 9001, 9007, 9011};
      size_t h = 0;
      for (unsigned i = 0; i < p.size(); ++i)
        h += reinterpret_cast<size_t>(p[i]) * primes[i % 10];
      return h;
    }
  };
}

namespace Moses {

class GibblerMaxTransDecoder : public SampleCollector {
 public:
  GibblerMaxTransDecoder();
  virtual void collect(Sample& sample);
  virtual std::vector<const Factor*> Max();
  /** Output the max translation whenever it changes */
  void setOutputMaxChange(bool outputMaxChange){m_outputMaxChange = outputMaxChange;}

 private:
  int n;
  __gnu_cxx::hash_map<std::vector<const Factor*>, int> samples;
  int sent_num;
  
  bool m_outputMaxChange;
  std::vector<const Factor*> m_maxTranslation;
};

}
