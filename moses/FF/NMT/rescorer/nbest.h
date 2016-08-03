#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cmath>

class Vocab;

typedef std::vector <std::vector < size_t > > Batch;

class NBest {
  public:
    NBest(
      const std::string& srcPath,
      const std::string& nbestPath,
      const std::shared_ptr<Vocab> srcVocab,
      const std::shared_ptr<Vocab> trgVocab,
      const size_t maxBatchSize=64);

    NBest(
      const std::vector<std::string>& nBestList,
      const std::shared_ptr<Vocab> srcVocab,
      const std::shared_ptr<Vocab> trgVocab,
      const size_t maxBatchSize);

    std::vector<Batch> GetBatches(const size_t index) const;

    size_t GetIndex(const size_t index) const {
      return indexes_[index];
    }

    std::vector<std::string> operator[](size_t index) const {
      return data_[index];
    }

    size_t size() const;

    std::string GetSentence(const size_t index) const;

    std::vector<std::string> GetTokens(const size_t index) const;

    std::vector<size_t> GetEncodedTokens(const size_t index) const;

    std::vector<Batch> DivideNBestListIntoBatches() const;

  private:
    void Parse_(const std::string& path);
    std::vector<std::vector<std::string>> SplitBatch(std::vector<std::string>& batch) const;
    void ParseInputFile(const std::string& path);

    Batch EncodeBatch(const std::vector<std::vector<std::string> >& batch) const;

    Batch MaskAndTransposeBatch(const Batch& batch) const;

    Batch ProcessBatch(std::vector<std::string>& batch) const;
    Batch ProcessBatch(std::vector<std::vector<std::string> >& batch) const;
  private:
    std::vector<std::vector<std::string> > data_;
    std::vector<std::string> srcSentences_;
    std::shared_ptr<Vocab> srcVocab_;
    std::shared_ptr<Vocab> trgVocab_;
    std::vector<size_t> indexes_;
    const size_t maxBatchSize_;

};
