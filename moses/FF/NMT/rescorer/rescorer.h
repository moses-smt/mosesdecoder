#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>


class Vocab;
class NBest;
class Encoder;
class Decoder;
class Weights;

class Rescorer {
  public:
    Rescorer(
      const std::shared_ptr<Weights> model,
      const std::shared_ptr<NBest> nBest,
      const std::string& featureName = "NMT0");

    void Score(const size_t index);

  private:
    std::vector<float> ScoreBatch(
      void* SourceContext,
      const std::vector<std::vector<size_t> >& batch);

  private:
    std::shared_ptr<Weights> model_;
    std::shared_ptr<NBest> nbest_;
    std::shared_ptr<Encoder> encoder_;
    std::shared_ptr<Decoder> decoder_;
    const std::string& featureName_;

};
