
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "dl4mt.h"
#include "vocab.h"
#include "utils.h"
#include "states.h"
#include "nbest.h"

class Rescorer {
  public:
  Rescorer(
    const std::shared_ptr<Weights> model,
    const std::shared_ptr<NBest> nBest,
    const std::string& featureName)
      : model_(model),
        nbest_(nBest),
        encoder_(new Encoder(*model)),
        decoder_(new Decoder(*model)),
        featureName_(featureName) {
  }

  void Score(const size_t index) {
    std::vector<size_t> sIndexes = nbest_->GetEncodedTokens(index);

    encoder_->GetContext(sIndexes, SourceContext_);
    size_t batchIndex = 0;
    for (auto& batch: nbest_->GetBatches(index)) {
      const auto scores = ScoreBatch(batch);
      for (size_t j = 0; j < batch[0].size(); ++j) {
        std::cout
          << (*nbest_)[nbest_->GetIndex(index) + batchIndex + j][0] << " ||| "
          << (*nbest_)[nbest_->GetIndex(index) + batchIndex + j][1] << " ||| "
          << (*nbest_)[nbest_->GetIndex(index) + batchIndex + j][2] << " "
          << featureName_ << "= " << scores[j] << " ||| "
          << (*nbest_)[nbest_->GetIndex(index) + batchIndex + j][3]
          << std::endl;
      }
      batchIndex += batch[0].size();
    }
  }


  private:
  std::vector<float> ScoreBatch(
        const std::vector<std::vector<size_t>>& batch) {
      size_t batchSize = batch[0].size();

      decoder_->EmptyState(PrevState_, SourceContext_, batchSize);
      decoder_->EmptyEmbedding(PrevEmbedding_, batchSize);

      std::vector<float> scores(batch[0].size(), 0.0f);
      size_t lengthIndex = 0;
      for (auto& w : batch) {
        decoder_->MakeStep(State_, Probs_,
                           PrevState_, PrevEmbedding_, SourceContext_);

        for (size_t j = 0; j < w.size(); ++j) {
          if (batch[lengthIndex][j]) {
            float p = Probs_(j, w[j]);
            scores[j] += p;
          }
        }

        decoder_->Lookup(Embedding_, w);

        mblas::Swap(State_, PrevState_);
        mblas::Swap(Embedding_, PrevEmbedding_);
        ++lengthIndex;
      }
      return scores;
    }

  private:
    std::shared_ptr<Weights> model_;
    std::shared_ptr<NBest> nbest_;
    std::shared_ptr<Encoder> encoder_;
    std::shared_ptr<Decoder> decoder_;
    const std::string& featureName_;

    mblas::Matrix SourceContext_;
    mblas::Matrix PrevState_;
    mblas::Matrix PrevEmbedding_;
    mblas::Matrix Probs_;
    mblas::Matrix State_;
    mblas::Matrix Embedding_;
};
