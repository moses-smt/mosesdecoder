#pragma once

namespace Moses {

class Hypothesis;
class TranslationOptionCollection;

class Sampler {
 private:

 public:
  void Run(Hypothesis* starting, const TranslationOptionCollection* options);

};

}

