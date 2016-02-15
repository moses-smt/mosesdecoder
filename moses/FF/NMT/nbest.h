#pragma once

#include <string>
#include <vector>

#include "vocab.h"

typedef std::vector <std::vector < size_t > > Batch;

class NBest {
  public:
    NBest(const std::string& nbestPath,
          const std::string& trgVocabPath);

    std::vector<Batch> GetBatches(const size_t index, size_t maxSize=64) const;
    size_t GetIndex(const size_t index) const {
      return indexes_[index];
    }

    std::vector<std::string> operator[](size_t index) {
      return data_[index];
    }

  private:
    void Parse_(const std::string& path);
    std::vector<std::vector<std::string>> SplitBatch(std::vector<std::string>& batch) const;

    Batch EncodeBatch(const std::vector<std::vector<std::string>>& batch) const;

    Batch MaskAndTransposeBatch(const Batch& batch) const;

    Batch ProcessBatch(std::vector<std::string>& batch) const;
  private:
    std::vector<std::vector<std::string> > data_;
    Vocab trgVocab_;
    std::vector<size_t> indexes_;

};
