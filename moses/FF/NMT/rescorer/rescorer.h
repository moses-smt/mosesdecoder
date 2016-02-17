
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "model.h"
#include "encoder.h"
#include "decoder.h"
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
        encoder_(new Encoder(*model)),
        decoder_(new Decoder(*model)),
        featureName_(featureName),
        nbest_(nBest) {
  }

  void Score(const size_t index) {
    auto sIndexes = nbest_->GetEncodedTokens(index);

    mblas::Matrix SourceContext;
    encoder_->GetContext(sIndexes, SourceContext);
    size_t batchIndex = 0;
    for(auto& batch: nbest_->GetBatches(index)) {
      const auto scores = ScoreBatch(&SourceContext, batch);
      for (size_t j = 0; j < batch[0].size(); ++j) {
        std::cerr
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
        void* SourceContext,
        const std::vector<std::vector<size_t>>& batch) {
      mblas::Matrix PrevState;
      mblas::Matrix PrevEmbedding;

      mblas::Matrix AlignedSourceContext;
      mblas::Matrix Probs;

      mblas::Matrix State;
      mblas::Matrix Embedding;
      size_t batchSize = batch[0].size();

      decoder_->EmptyState(PrevState, *(mblas::Matrix*)SourceContext, batchSize);
      decoder_->EmptyEmbedding(PrevEmbedding, batchSize);

      std::vector<float> scores(batch[0].size(), 0.0f);
      size_t lengthIndex = 0;
      for (auto& w : batch) {
        decoder_->GetProbs(Probs, AlignedSourceContext,
                        PrevState, PrevEmbedding, *(mblas::Matrix*)SourceContext);

        for (size_t j = 0; j < w.size(); ++j) {
          if (batch[lengthIndex][j]) {
            float p = Probs(j, w[j]);
            scores[j] += log(p);
          }
        }

        decoder_->Lookup(Embedding, w);
        decoder_->GetNextState(State, Embedding,
                            PrevState, AlignedSourceContext);

        mblas::Swap(State, PrevState);
        mblas::Swap(Embedding, PrevEmbedding);
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

};
