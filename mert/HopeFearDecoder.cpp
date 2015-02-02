/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <algorithm>
#include <cmath>
#include <iterator>

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "util/exception.hh"
#include "util/file_piece.hh"

#include "Scorer.h"
#include "HopeFearDecoder.h"

using namespace std;
namespace fs = boost::filesystem;

namespace MosesTuning
{

static const ValType BLEU_RATIO = 5;

ValType HopeFearDecoder::Evaluate(const AvgWeightVector& wv)
{
  vector<ValType> stats(scorer_->NumberOfScores(),0);
  for(reset(); !finished(); next()) {
    vector<ValType> sent;
    MaxModel(wv,&sent);
    for(size_t i=0; i<sent.size(); i++) {
      stats[i]+=sent[i];
    }
  }
  return scorer_->calculateScore(stats);
}

NbestHopeFearDecoder::NbestHopeFearDecoder(
  const vector<string>& featureFiles,
  const vector<string>&  scoreFiles,
  bool streaming,
  bool  no_shuffle,
  bool safe_hope,
  Scorer* scorer
) : safe_hope_(safe_hope)
{
  scorer_ = scorer;
  if (streaming) {
    train_.reset(new StreamingHypPackEnumerator(featureFiles, scoreFiles));
  } else {
    train_.reset(new RandomAccessHypPackEnumerator(featureFiles, scoreFiles, no_shuffle));
  }
}


void NbestHopeFearDecoder::next()
{
  train_->next();
}

bool NbestHopeFearDecoder::finished()
{
  return train_->finished();
}

void NbestHopeFearDecoder::reset()
{
  train_->reset();
}

void NbestHopeFearDecoder::HopeFear(
  const std::vector<ValType>& backgroundBleu,
  const MiraWeightVector& wv,
  HopeFearData* hopeFear
)
{


  // Hope / fear decode
  ValType hope_scale = 1.0;
  size_t hope_index=0, fear_index=0, model_index=0;
  ValType hope_score=0, fear_score=0, model_score=0;
  for(size_t safe_loop=0; safe_loop<2; safe_loop++) {
    ValType hope_bleu, hope_model;
    for(size_t i=0; i< train_->cur_size(); i++) {
      const MiraFeatureVector& vec=train_->featuresAt(i);
      ValType score = wv.score(vec);
      ValType bleu = scorer_->calculateSentenceLevelBackgroundScore(train_->scoresAt(i),backgroundBleu);
      // Hope
      if(i==0 || (hope_scale*score + bleu) > hope_score) {
        hope_score = hope_scale*score + bleu;
        hope_index = i;
        hope_bleu = bleu;
        hope_model = score;
      }
      // Fear
      if(i==0 || (score - bleu) > fear_score) {
        fear_score = score - bleu;
        fear_index = i;
      }
      // Model
      if(i==0 || score > model_score) {
        model_score = score;
        model_index = i;
      }
    }
    // Outer loop rescales the contribution of model score to 'hope' in antagonistic cases
    // where model score is having far more influence than BLEU
    hope_bleu *= BLEU_RATIO; // We only care about cases where model has MUCH more influence than BLEU
    if(safe_hope_ && safe_loop==0 && abs(hope_model)>1e-8 && abs(hope_bleu)/abs(hope_model)<hope_scale)
      hope_scale = abs(hope_bleu) / abs(hope_model);
    else break;
  }
  hopeFear->modelFeatures = train_->featuresAt(model_index);
  hopeFear->hopeFeatures = train_->featuresAt(hope_index);
  hopeFear->fearFeatures = train_->featuresAt(fear_index);

  hopeFear->hopeStats = train_->scoresAt(hope_index);
  hopeFear->hopeBleu = scorer_->calculateSentenceLevelBackgroundScore(hopeFear->hopeStats, backgroundBleu);
  const vector<float>& fear_stats = train_->scoresAt(fear_index);
  hopeFear->fearBleu = scorer_->calculateSentenceLevelBackgroundScore(fear_stats, backgroundBleu);

  hopeFear->modelStats = train_->scoresAt(model_index);
  hopeFear->hopeFearEqual = (hope_index == fear_index);
}

void NbestHopeFearDecoder::MaxModel(const AvgWeightVector& wv, std::vector<ValType>* stats)
{
  // Find max model
  size_t max_index=0;
  ValType max_score=0;
  for(size_t i=0; i<train_->cur_size(); i++) {
    MiraFeatureVector vec(train_->featuresAt(i));
    ValType score = wv.score(vec);
    if(i==0 || score > max_score) {
      max_index = i;
      max_score = score;
    }
  }
  *stats = train_->scoresAt(max_index);
}



HypergraphHopeFearDecoder::HypergraphHopeFearDecoder
(
  const string& hypergraphDir,
  const vector<string>& referenceFiles,
  size_t num_dense,
  bool streaming,
  bool no_shuffle,
  bool safe_hope,
  size_t hg_pruning,
  const MiraWeightVector& wv,
  Scorer* scorer
) :
  num_dense_(num_dense)
{

  UTIL_THROW_IF(streaming, util::Exception, "Streaming not currently supported for hypergraphs");
  UTIL_THROW_IF(!fs::exists(hypergraphDir), HypergraphException, "Directory '" << hypergraphDir << "' does not exist");
  UTIL_THROW_IF(!referenceFiles.size(), util::Exception, "No reference files supplied");
  references_.Load(referenceFiles, vocab_);

  SparseVector weights;
  wv.ToSparse(&weights);
  scorer_ = scorer;

  static const string kWeights = "weights";
  fs::directory_iterator dend;
  size_t fileCount = 0;

  cerr << "Reading  hypergraphs" << endl;
  for (fs::directory_iterator di(hypergraphDir); di != dend; ++di) {
    const fs::path& hgpath = di->path();
    if (hgpath.filename() == kWeights) continue;
    //  cerr << "Reading " << hgpath.filename() << endl;
    Graph graph(vocab_);
    size_t id = boost::lexical_cast<size_t>(hgpath.stem().string());
    util::scoped_fd fd(util::OpenReadOrThrow(hgpath.string().c_str()));
    //util::FilePiece file(di->path().string().c_str());
    util::FilePiece file(fd.release());
    ReadGraph(file,graph);

    //cerr << "ref length " << references_.Length(id) << endl;
    size_t edgeCount = hg_pruning * references_.Length(id);
    boost::shared_ptr<Graph> prunedGraph;
    prunedGraph.reset(new Graph(vocab_));
    graph.Prune(prunedGraph.get(), weights, edgeCount);
    graphs_[id] = prunedGraph;
    // cerr << "Pruning to v=" << graphs_[id]->VertexSize() << " e=" << graphs_[id]->EdgeSize()  << endl;
    ++fileCount;
    if (fileCount % 10 == 0) cerr << ".";
    if (fileCount % 400 ==  0) cerr << " [count=" << fileCount << "]\n";
  }
  cerr << endl << "Done" << endl;

  sentenceIds_.resize(graphs_.size());
  for (size_t i = 0; i < graphs_.size(); ++i) sentenceIds_[i] = i;
  if (!no_shuffle) {
    random_shuffle(sentenceIds_.begin(), sentenceIds_.end());
  }

}

void HypergraphHopeFearDecoder::reset()
{
  sentenceIdIter_ = sentenceIds_.begin();
}

void HypergraphHopeFearDecoder::next()
{
  sentenceIdIter_++;
}

bool HypergraphHopeFearDecoder::finished()
{
  return sentenceIdIter_ == sentenceIds_.end();
}

void HypergraphHopeFearDecoder::HopeFear(
  const vector<ValType>& backgroundBleu,
  const MiraWeightVector& wv,
  HopeFearData* hopeFear
)
{
  size_t sentenceId = *sentenceIdIter_;
  SparseVector weights;
  wv.ToSparse(&weights);
  const Graph& graph = *(graphs_[sentenceId]);

  ValType hope_scale = 1.0;
  HgHypothesis hopeHypo, fearHypo, modelHypo;
  for(size_t safe_loop=0; safe_loop<2; safe_loop++) {

    //hope decode
    Viterbi(graph, weights, 1, references_, sentenceId, backgroundBleu, &hopeHypo);

    //fear decode
    Viterbi(graph, weights, -1, references_, sentenceId, backgroundBleu, &fearHypo);

    //Model decode
    Viterbi(graph, weights, 0, references_, sentenceId, backgroundBleu, &modelHypo);


    // Outer loop rescales the contribution of model score to 'hope' in antagonistic cases
    // where model score is having far more influence than BLEU
    //  hope_bleu *= BLEU_RATIO; // We only care about cases where model has MUCH more influence than BLEU
    //  if(safe_hope_ && safe_loop==0 && abs(hope_model)>1e-8 && abs(hope_bleu)/abs(hope_model)<hope_scale)
    //    hope_scale = abs(hope_bleu) / abs(hope_model);
    //  else break;
    //TODO: Don't currently get model and bleu so commented this out for now.
    break;
  }
  //modelFeatures, hopeFeatures and fearFeatures
  hopeFear->modelFeatures = MiraFeatureVector(modelHypo.featureVector, num_dense_);
  hopeFear->hopeFeatures = MiraFeatureVector(hopeHypo.featureVector, num_dense_);
  hopeFear->fearFeatures = MiraFeatureVector(fearHypo.featureVector, num_dense_);

  //Need to know which are to be mapped to dense features!

  //Only C++11
  //hopeFear->modelStats.assign(std::begin(modelHypo.bleuStats), std::end(modelHypo.bleuStats));
  vector<ValType> fearStats(scorer_->NumberOfScores());
  hopeFear->hopeStats.reserve(scorer_->NumberOfScores());
  hopeFear->modelStats.reserve(scorer_->NumberOfScores());
  for (size_t i = 0; i < fearStats.size(); ++i) {
    hopeFear->modelStats.push_back(modelHypo.bleuStats[i]);
    hopeFear->hopeStats.push_back(hopeHypo.bleuStats[i]);

    fearStats[i] = fearHypo.bleuStats[i];
  }
  /*
  cerr << "hope" << endl;;
  for (size_t i = 0; i < hopeHypo.text.size(); ++i) {
    cerr << hopeHypo.text[i]->first << " ";
  }
  cerr << endl;
  for (size_t i = 0; i < fearStats.size(); ++i) {
    cerr << hopeHypo.bleuStats[i] << " ";
  }
  cerr << endl;
  cerr << "fear";
  for (size_t i = 0; i < fearHypo.text.size(); ++i) {
    cerr << fearHypo.text[i]->first << " ";
  }
  cerr << endl;
  for (size_t i = 0; i < fearStats.size(); ++i) {
    cerr  << fearHypo.bleuStats[i] << " ";
  }
  cerr << endl;
  cerr << "model";
  for (size_t i = 0; i < modelHypo.text.size(); ++i) {
    cerr << modelHypo.text[i]->first << " ";
  }
  cerr << endl;
  for (size_t i = 0; i < fearStats.size(); ++i) {
    cerr << modelHypo.bleuStats[i] << " ";
  }
  cerr << endl;
  */
  hopeFear->hopeBleu = sentenceLevelBackgroundBleu(hopeFear->hopeStats, backgroundBleu);
  hopeFear->fearBleu = sentenceLevelBackgroundBleu(fearStats, backgroundBleu);

  //If fv and bleu stats are equal, then assume equal
  hopeFear->hopeFearEqual = true; //(hopeFear->hopeBleu - hopeFear->fearBleu) >= 1e-8;
  if (hopeFear->hopeFearEqual) {
    for (size_t i = 0; i < fearStats.size(); ++i) {
      if (fearStats[i] != hopeFear->hopeStats[i]) {
        hopeFear->hopeFearEqual = false;
        break;
      }
    }
  }
  hopeFear->hopeFearEqual = hopeFear->hopeFearEqual && (hopeFear->fearFeatures == hopeFear->hopeFeatures);
}

void HypergraphHopeFearDecoder::MaxModel(const AvgWeightVector& wv, vector<ValType>* stats)
{
  assert(!finished());
  HgHypothesis bestHypo;
  size_t sentenceId = *sentenceIdIter_;
  SparseVector weights;
  wv.ToSparse(&weights);
  vector<ValType> bg(scorer_->NumberOfScores());
  //cerr << "Calculating bleu on " << sentenceId << endl;
  Viterbi(*(graphs_[sentenceId]), weights, 0, references_, sentenceId, bg, &bestHypo);
  stats->resize(bestHypo.bleuStats.size());
  /*
  for (size_t i = 0; i < bestHypo.text.size(); ++i) {
    cerr << bestHypo.text[i]->first << " ";
  }
  cerr << endl;
  */
  for (size_t i = 0; i < bestHypo.bleuStats.size(); ++i) {
    (*stats)[i] = bestHypo.bleuStats[i];
  }
}



};
