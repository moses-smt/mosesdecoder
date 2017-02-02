// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011- University of Edinburgh

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


/**
  * This is part of the PRO implementation. It converts the features and scores
  * files into a form suitable for input into the megam maxent trainer.
  *
  *   For details of PRO, refer to Hopkins & May (EMNLP 2011)
 **/
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include <boost/program_options.hpp>

#include "BleuScorer.h"
#include "FeatureDataIterator.h"
#include "ScoreDataIterator.h"
#include "BleuScorer.h"
#include "Util.h"
#include "util/random.hh"

using namespace std;
using namespace MosesTuning;

namespace po = boost::program_options;

namespace MosesTuning
{

class SampledPair
{
private:
  pair<size_t,size_t> m_translation1;
  pair<size_t,size_t> m_translation2;
  float m_score_diff;

public:
  SampledPair(const pair<size_t,size_t>& t1, const pair<size_t,size_t>& t2, float diff ) {
    if (diff > 0) {
      m_translation1 = t1;
      m_translation2 = t2;
      m_score_diff = diff;
    } else {
      m_translation1 = t2;
      m_translation2 = t1;
      m_score_diff = -diff;
    }
  }

  float getDiff() const {
    return m_score_diff;
  }
  const pair<size_t,size_t>& getTranslation1() const {
    return m_translation1;
  }
  const pair<size_t,size_t>& getTranslation2() const {
    return m_translation2;
  }
};

static void outputSample(ostream& out, const FeatureDataItem& f1, const FeatureDataItem& f2)
{
  // difference in score in regular features
  for(unsigned int j=0; j<f1.dense.size(); j++)
    if (abs(f1.dense[j]-f2.dense[j]) > 0.00001)
      out << " F" << j << " " << (f1.dense[j]-f2.dense[j]);

  if (f1.sparse.size() || f2.sparse.size()) {
    out << " ";

    // sparse features
    const SparseVector &s1 = f1.sparse;
    const SparseVector &s2 = f2.sparse;
    SparseVector diff = s1 - s2;
    diff.write(out);
  }
}

}

int main(int argc, char** argv)
{
  bool help;
  vector<string> scoreFiles;
  vector<string> featureFiles;
  int seed;
  string outputFile;
  // TODO: Add these constants to options
  const unsigned int n_candidates = 5000; // Gamma, in Hopkins & May
  const unsigned int n_samples = 50; // Xi, in Hopkins & May
  const float min_diff = 0.05;
  bool smoothBP = false;
  const float bleuSmoothing = 1.0f;

  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
  ("scfile,S", po::value<vector<string> >(&scoreFiles), "Scorer data files")
  ("ffile,F", po::value<vector<string> > (&featureFiles), "Feature data files")
  ("random-seed,r", po::value<int>(&seed), "Seed for random number generation")
  ("output-file,o", po::value<string>(&outputFile), "Output file")
  ("smooth-brevity-penalty,b", po::value(&smoothBP)->zero_tokens()->default_value(false), "Smooth the brevity penalty, as in Nakov et al. (Coling 2012)")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  po::store(po::command_line_parser(argc,argv).
            options(cmdline_options).run(), vm);
  po::notify(vm);
  if (help) {
    cout << "Usage: " + string(argv[0]) +  " [options]" << endl;
    cout << desc << endl;
    exit(0);
  }

  if (vm.count("random-seed")) {
    cerr << "Initialising random seed to " << seed << endl;
    util::rand_init(seed);
  } else {
    cerr << "Initialising random seed from system clock" << endl;
    util::rand_init();
  }

  if (scoreFiles.size() == 0 || featureFiles.size() == 0) {
    cerr << "No data to process" << endl;
    exit(0);
  }

  if (featureFiles.size() != scoreFiles.size()) {
    cerr << "Error: Number of feature files (" << featureFiles.size() <<
         ") does not match number of score files (" << scoreFiles.size() << ")" << endl;
    exit(1);
  }

  ostream* out;
  ofstream outFile;
  if (!outputFile.empty() ) {
    outFile.open(outputFile.c_str());
    if (!(outFile)) {
      cerr << "Error: Failed to open " << outputFile << endl;
      exit(1);
    }
    out = &outFile;
  } else {
    out = &cout;
  }


  vector<FeatureDataIterator> featureDataIters;
  vector<ScoreDataIterator> scoreDataIters;
  for (size_t i = 0; i < featureFiles.size(); ++i) {
    featureDataIters.push_back(FeatureDataIterator(featureFiles[i]));
    scoreDataIters.push_back(ScoreDataIterator(scoreFiles[i]));
  }

  //loop through nbest lists
  size_t sentenceId = 0;
  while(1) {
    vector<pair<size_t,size_t> > hypotheses;
    //TODO: de-deuping. Collect hashes of score,feature pairs and
    //only add index if it's unique.
    if (featureDataIters[0] == FeatureDataIterator::end()) {
      break;
    }
    for (size_t i = 0; i < featureFiles.size(); ++i) {
      if (featureDataIters[i] == FeatureDataIterator::end()) {
        cerr << "Error: Feature file " << i << " ended prematurely" << endl;
        exit(1);
      }
      if (scoreDataIters[i] == ScoreDataIterator::end()) {
        cerr << "Error: Score file " << i << " ended prematurely" << endl;
        exit(1);
      }
      if (featureDataIters[i]->size() != scoreDataIters[i]->size()) {
        cerr << "Error: For sentence " << sentenceId << " features and scores have different size" << endl;
        exit(1);
      }
      for (size_t j = 0; j < featureDataIters[i]->size(); ++j) {
        hypotheses.push_back(pair<size_t,size_t>(i,j));
      }
    }

    //collect the candidates
    vector<SampledPair> samples;
    vector<float> scores;
    size_t n_translations = hypotheses.size();
    for(size_t  i=0; i<n_candidates; i++) {
      size_t rand1 = util::rand_excl(n_translations);
      pair<size_t,size_t> translation1 = hypotheses[rand1];
      float bleu1 = smoothedSentenceBleu(scoreDataIters[translation1.first]->operator[](translation1.second), bleuSmoothing, smoothBP);

      size_t rand2 = util::rand_excl(n_translations);
      pair<size_t,size_t> translation2 = hypotheses[rand2];
      float bleu2 = smoothedSentenceBleu(scoreDataIters[translation2.first]->operator[](translation2.second), bleuSmoothing, smoothBP);

      /*
      cerr << "t(" << translation1.first << "," << translation1.second << ") = " << bleu1 <<
        " t(" << translation2.first << "," << translation2.second << ") = " <<
          bleu2  << " diff = " << abs(bleu1-bleu2) << endl;
      */
      if (abs(bleu1-bleu2) < min_diff)
        continue;

      samples.push_back(SampledPair(translation1, translation2, bleu1-bleu2));
      scores.push_back(1.0-abs(bleu1-bleu2));
    }

    float sample_threshold = -1.0;
    if (samples.size() > n_samples) {
      NTH_ELEMENT3(scores.begin(), scores.begin() + (n_samples-1), scores.end());
      sample_threshold = 0.99999-scores[n_samples-1];
    }

    size_t collected = 0;
    for (size_t i = 0; collected < n_samples && i < samples.size(); ++i) {
      if (samples[i].getDiff() < sample_threshold) continue;
      ++collected;
      size_t file_id1 = samples[i].getTranslation1().first;
      size_t hypo_id1 = samples[i].getTranslation1().second;
      size_t file_id2 = samples[i].getTranslation2().first;
      size_t hypo_id2 = samples[i].getTranslation2().second;
      *out << "1";
      outputSample(*out, featureDataIters[file_id1]->operator[](hypo_id1),
                   featureDataIters[file_id2]->operator[](hypo_id2));
      *out << endl;
      *out << "0";
      outputSample(*out, featureDataIters[file_id2]->operator[](hypo_id2),
                   featureDataIters[file_id1]->operator[](hypo_id1));
      *out << endl;
    }
    //advance all iterators
    for (size_t i = 0; i < featureFiles.size(); ++i) {
      ++featureDataIters[i];
      ++scoreDataIters[i];
    }
    ++sentenceId;
  }

  outFile.close();

}
