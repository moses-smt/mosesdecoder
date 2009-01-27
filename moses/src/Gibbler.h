#pragma once

namespace Moses {

class Hypothesis;
class TranslationOptionCollection;

class Sample {
 private:
  Hypothesis* target_head;
  Hypothesis* target_tail;

  Hypothesis* source_head;
  Hypothesis* source_tail;

 public:
  Sample(Hypothesis* target_head);
};

class Sampler {
 private:

 public:
  void Run(Hypothesis* starting, const TranslationOptionCollection* options);
};

}

