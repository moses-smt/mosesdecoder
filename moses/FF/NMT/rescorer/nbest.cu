#include "nbest.h"

#include <algorithm>

#include "utils.h"
#include "vocab.h"

NBest::NBest(
    const std::string& srcPath,
    const std::string& nbestPath,
    const std::shared_ptr<Vocab> srcVocab,
    const std::shared_ptr<Vocab> trgVocab,
    const size_t maxBatchSize)
    : srcVocab_(srcVocab),
      trgVocab_(trgVocab),
      maxBatchSize_(maxBatchSize) {
  ParseInputFile(srcPath);
  Parse_(nbestPath);
}

void NBest::ParseInputFile(const std::string& path) {
    std::ifstream file(path);
    srcSentences_.clear();
    std::string line;
    while (std::getline(file, line).good()) {
      Trim(line);
      srcSentences_.push_back(line);
    }
}

std::string NBest::GetSentence(const size_t index) const {
  return srcSentences_[index];
}

std::vector<std::string> NBest::GetTokens(const size_t index) const {
  std::vector<std::string> tokens;
  Split(srcSentences_[index], tokens);
  return tokens;
}

std::vector<size_t> NBest::GetEncodedTokens(const size_t index) const {
  std::vector<std::string> tokens;
  Split(srcSentences_[index], tokens);
  return srcVocab_->Encode(tokens);
}

void NBest::Parse_(const std::string& path) {
  std::ifstream file(path);

  std::string line;
  size_t lineCount = 0;
  indexes_.push_back(lineCount);

  while (std::getline(file, line).good()) {
    boost::trim(line);
    std::vector<std::string> fields;
    Split(line, fields, " ||| ");
    if (lineCount && (data_.back()[0] != fields[0])) {
      indexes_.push_back(lineCount);
    }
    data_.push_back(fields);
  }
  indexes_.push_back(data_.size());
}


inline std::vector< std::vector< std::string > > NBest::SplitBatch(std::vector<std::string>& batch) const {
  std::vector< std::vector< std::string > > splittedBatch;
  for (auto& sentence : batch) {
    Trim(sentence);
    std::vector<std::string> words;
    Split(sentence, words);
    splittedBatch.push_back(words);
  }
  return splittedBatch;
}

inline Batch NBest::EncodeBatch(const std::vector<std::vector<std::string>>& batch) const {
  Batch encodedBatch;
  for (auto& sentence: batch) {
    encodedBatch.push_back(trgVocab_->Encode(sentence, true));
  }
  return encodedBatch;
}

inline Batch NBest::MaskAndTransposeBatch(const Batch& batch) const {
  size_t maxLength = 0;
  for (auto& sentence: batch) {
    maxLength = max(maxLength, sentence.size());
  }
  Batch masked;
  for (size_t i = 0; i < maxLength; ++i) {
      masked.emplace_back(batch.size(), 0);
      for (size_t j = 0; j < batch.size(); ++j) {
          if (i < batch[j].size()) {
              masked[i][j] = batch[j][i];
          }
      }
  }
  return masked;
}


Batch NBest::ProcessBatch(std::vector<std::string>& batch) const {
  return MaskAndTransposeBatch(EncodeBatch(SplitBatch(batch)));
}

std::vector<Batch> NBest::GetBatches(const size_t index) const {
  std::vector<Batch> batches;
  std::vector<std::string> sBatch;
  for (size_t i = indexes_[index]; i <= indexes_[index+1]; ++i) {
    if (sBatch.size() == maxBatchSize_ || i == indexes_[index+1]) {
      batches.push_back(ProcessBatch(sBatch));
      sBatch.clear();
      if (i == indexes_[index+1]) {
        break;
      }
    }
    sBatch.push_back(data_[i][1]);
  }
  return batches;
}


size_t NBest::size() const {
  return srcSentences_.size();
}
