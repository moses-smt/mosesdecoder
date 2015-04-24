/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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
#ifndef MAIN_H_
#define MAIN_H_

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "moses/Word.h"
#include "moses/FF/FeatureFunction.h"
#include "Decoder.h"
#include "util/random.hh"

typedef std::map<const Moses::FeatureFunction*, std::vector< float > > ProducerWeightMap;
typedef std::pair<const Moses::FeatureFunction*, std::vector< float > > ProducerWeightPair;

template <class T> bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
{
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}

struct RandomIndex {
  ptrdiff_t operator()(ptrdiff_t max) {
    return util::rand_excl(max);
  }
};

//void OutputNBestList(const MosesChart::TrellisPathList &nBestList, const TranslationSystem* system, long translationId);
bool loadSentences(const std::string& filename, std::vector<std::string>& sentences);
bool evaluateModulo(size_t shard_position, size_t mix_or_dump_base, size_t actual_batch_size);
void printFeatureValues(std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues);
void ignoreCoreFeatures(std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, ProducerWeightMap &coreWeightMap);
void takeLogs(std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, size_t base);
void deleteTranslations(std::vector<std::vector<const Moses::Word*> > &translations);
void decodeHopeOrFear(size_t rank, size_t size, size_t decode, std::string decode_filename, std::vector<std::string> &inputSentences, Mira::MosesDecoder* decoder, size_t n, float bleuWeight);
void applyLearningRates(std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, float core_r0, float sparse_r0);
void applyPerFeatureLearningRates(std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, Moses::ScoreComponentCollection featureLearningRates, float sparse_r0);
void scaleFeatureScore(const Moses::FeatureFunction *sp, float scaling_factor,  std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch);
void scaleFeatureScores(const Moses::FeatureFunction *sp, float scaling_factor,  std::vector<std::vector<Moses::ScoreComponentCollection> > &featureValues, size_t rank, size_t epoch);

#endif /* MAIN_H_ */
