#pragma once

#include <string>
#include <vector>
#include <memory>

#include <boost/shared_ptr.hpp>
class Vocab;

typedef std::vector <std::vector < size_t > > NBestBatch;

class NBest {
  public:
    NBest(
      const std::string& srcPath,
      const std::string& nbestPath,
      const boost::shared_ptr<Vocab> srcVocab,
      const boost::shared_ptr<Vocab> trgVocab,
      const size_t maxBatchSize=64);

    NBest(
      const boost::shared_ptr<Vocab> srcVocab,
      const boost::shared_ptr<Vocab> trgVocab,
      const std::vector<std::string>& nBestList,
      const size_t maxBatchSize=64);

    std::vector<NBestBatch> GetBatches(const size_t index) const;

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
    std::vector<NBestBatch> DivideNBestListIntoBatches() const;

  private:
    void Parse_(const std::string& path);
    std::vector<std::vector<std::string> > SplitBatch(std::vector<std::string>& batch) const;
    void ParseInputFile(const std::string& path);

    NBestBatch EncodeBatch(const std::vector<std::vector<std::string> >& batch) const;

    NBestBatch MaskAndTransposeBatch(const NBestBatch& batch) const;

    NBestBatch ProcessBatch(std::vector<std::string>& batch) const;
    NBestBatch ProcessBatch(std::vector<std::vector<std::string> >& batch) const;
  private:
    std::vector<std::vector<std::string> > data_;
    std::vector<std::string> srcSentences_;
    boost::shared_ptr<Vocab> srcVocab_;
    boost::shared_ptr<Vocab> trgVocab_;
    std::vector<size_t> indexes_;
    const size_t maxBatchSize_;

};
