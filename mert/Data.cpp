/*
 *  Data.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <cassert>
#include <fstream>

#include "Scorer.h"
#include "ScorerFactory.h"
#include "Data.h"
#include "Util.h"


Data::Data(Scorer& ptr):
  theScorer(&ptr),
  _sparse_flag(false)
{
  score_type = (*theScorer).getName();
  TRACE_ERR("Data::score_type " << score_type << std::endl);

  TRACE_ERR("Data::Scorer type from Scorer: " << theScorer->getName() << endl);
  featdata=new FeatureData;
  scoredata=new ScoreData(*theScorer);
};

void Data::loadnbest(const std::string &file)
{
  TRACE_ERR("loading nbest from " << file << std::endl);

  FeatureStats featentry;
  ScoreStats scoreentry;
  std::string sentence_index;

  inputfilestream inp(file); // matches a stream with a file. Opens the file

  if (!inp.good())
    throw runtime_error("Unable to open: " + file);

  std::string substring, subsubstring, stringBuf;
  std::string theSentence;
  std::string::size_type loc;

  while (getline(inp,stringBuf,'\n')) {
    if (stringBuf.empty()) continue;

//		TRACE_ERR("stringBuf: " << stringBuf << std::endl);

    getNextPound(stringBuf, substring, "|||"); //first field
    sentence_index = substring;

    getNextPound(stringBuf, substring, "|||"); //second field
    theSentence = substring;

// adding statistics for error measures
    featentry.reset();
    scoreentry.clear();

    theScorer->prepareStats(sentence_index, theSentence, scoreentry);

    scoredata->add(scoreentry, sentence_index);

    getNextPound(stringBuf, substring, "|||"); //third field

    // examine first line for name of features
    if (!existsFeatureNames()) {
      std::string stringsupport=substring;
      std::string features="";
      std::string tmpname="";

      size_t tmpidx=0;
      while (!stringsupport.empty()) {
        //			TRACE_ERR("Decompounding: " << substring << std::endl);
        getNextPound(stringsupport, subsubstring);

        // string ending with ":" are skipped, because they are the names of the features
        if ((loc = subsubstring.find_last_of(":")) != subsubstring.length()-1) {
          features+=tmpname+"_"+stringify(tmpidx)+" ";
          tmpidx++;
        }
        // ignore sparse feature name
        else if (subsubstring.find("_") != string::npos) {
          // also ignore its value
          getNextPound(stringsupport, subsubstring);
        }
        // update current feature name
        else {
          tmpidx=0;
          tmpname=subsubstring.substr(0,subsubstring.size() - 1);
        }
      }

      featdata->setFeatureMap(features);
    }

    // adding features
    while (!substring.empty()) {
//			TRACE_ERR("Decompounding: " << substring << std::endl);
      getNextPound(substring, subsubstring);

      // no ':' -> feature value that needs to be stored
      if ((loc = subsubstring.find_last_of(":")) != subsubstring.length()-1) {
        featentry.add(ATOFST(subsubstring.c_str()));
      }
      // sparse feature name? store as well
      else if (subsubstring.find("_") != string::npos) {
        std::string name = subsubstring;
        getNextPound(substring, subsubstring);
        featentry.addSparse( name, atof(subsubstring.c_str()) );
        _sparse_flag = true;
      }
    }
    //cerr << "number of sparse features: " << featentry.getSparse().size() << endl;
    featdata->add(featentry,sentence_index);
  }

  inp.close();
}

// TODO
void Data::mergeSparseFeatures() { 
  std::cerr << "ERROR: sparse features can only be trained with pairwise ranked optimizer (PRO), not traditional MERT\n";
  exit(1);
}

// really not the right place...
float sentenceLevelBleuPlusOne( ScoreStats &stats ) {
	float logbleu = 0.0;
	const unsigned int bleu_order = 4;
	for (unsigned int j=0; j<bleu_order; j++) {
		//cerr << (stats.get(2*j)+1) << "/" << (stats.get(2*j+1)+1) << " ";
		logbleu += log(stats.get(2*j)+1) - log(stats.get(2*j+1)+1);
	}
	logbleu /= bleu_order;
	float brevity = 1.0 - (float)stats.get(bleu_order*2)/stats.get(1);
	if (brevity < 0.0) {
		logbleu += brevity;
	}
	//cerr << brevity << " -> " << exp(logbleu) << endl;
	return exp(logbleu);
}

class SampledPair {
private:
	unsigned int translation1;
	unsigned int translation2;
	float scoreDiff;
public:
	SampledPair( unsigned int t1, unsigned int t2, float diff ) {
		if (diff > 0) {
			translation1 = t1;
			translation2 = t2;
			scoreDiff = diff;
		}
		else {
			translation1 = t2;
			translation2 = t1;
			scoreDiff = -diff;
		}			
	}
	float getDiff() { return scoreDiff; }
	unsigned int getTranslation1() { return translation1; }
	unsigned int getTranslation2() { return translation2; }
};
	

void Data::sampleRankedPairs( const std::string &rankedpairfile ) {
	cout << "Sampling ranked pairs." << endl;

	ofstream *outFile = new ofstream();
	outFile->open( rankedpairfile.c_str() );
	ostream *out = outFile;

	const unsigned int n_samplings = 5000;
	const unsigned int n_samples = 50;
	const float min_diff = 0.05;

	// loop over all sentences
  for(unsigned int S=0; S<featdata->size(); S++) {
		unsigned int n_translations = featdata->get(S).size();
		// sample a fixed number of times
		vector< SampledPair* > samples;
		vector< float > scores;
		for(unsigned int i=0; i<n_samplings; i++) {
			unsigned int translation1 = rand() % n_translations;
			float bleu1 = sentenceLevelBleuPlusOne(scoredata->get(S,translation1));

			unsigned int translation2 = rand() % n_translations;
			float bleu2 = sentenceLevelBleuPlusOne(scoredata->get(S,translation2));
			
			if (abs(bleu1-bleu2) < min_diff)
				continue;
			
			samples.push_back( new SampledPair( translation1, translation2, bleu1-bleu2) );
			scores.push_back( 1.0 - abs(bleu1-bleu2) );
		}
		//cerr << "sampled " << samples.size() << " pairs\n";

		float min_diff = -1.0;
		if (samples.size() > n_samples) {
			nth_element(scores.begin(), scores.begin()+(n_samples-1), scores.end());
			min_diff = 0.99999-scores[n_samples-1];
			//cerr << "min_diff = " << min_diff << endl;
		}

		unsigned int collected = 0;
		for(unsigned int i=0; i<samples.size() && collected < n_samples; i++) {
			if (samples[i]->getDiff() >= min_diff) {
				collected++;

				*out << "1";
        outputSample( *out, featdata->get(S,samples[i]->getTranslation1()),
                            featdata->get(S,samples[i]->getTranslation2()) );
        *out << endl;
				*out << "0";
        outputSample( *out, featdata->get(S,samples[i]->getTranslation2()),
                            featdata->get(S,samples[i]->getTranslation1()) );
        *out << endl;
			}
			delete samples[i];
		}
		//cerr << "collected " << collected << endl;
	}
	out->flush();
	outFile->close();
}

void Data::outputSample( ostream &out, const FeatureStats &f1, const FeatureStats &f2 ) 
{
  // difference in score in regular features
	for(unsigned int j=0; j<f1.size(); j++)
		if (abs(f1.get(j)-f2.get(j)) > 0.00001)
			out << " F" << j << " " << (f1.get(j)-f2.get(j));

  if (!hasSparseFeatures())
    return;

  out << " ";

  // sparse features
  const SparseVector &s1 = f1.getSparse();
  const SparseVector &s2 = f2.getSparse();
  SparseVector diff = s1 - s2;
  diff.write(out);
}


void Data::createShards(size_t shard_count, float shard_size, const string& scorerconfig,
      std::vector<Data>& shards) 
{
  assert(shard_count);
  assert(shard_size >=0);
  assert(shard_size <= 1);

  size_t data_size = scoredata->size();
  assert(data_size == featdata->size());

  shard_size *=  data_size;

  for (size_t shard_id = 0; shard_id < shard_count; ++shard_id) {
    vector<size_t> shard_contents;
    if (shard_size == 0) {
      //split into roughly equal size shards
      size_t shard_start = floor(0.5 + shard_id * (float)data_size / shard_count);
      size_t shard_end = floor(0.5 + (shard_id+1) * (float)data_size / shard_count);
      for (size_t i = shard_start; i < shard_end; ++i) {
        shard_contents.push_back(i);
      }
    } else {
      //create shards by randomly sampling
      for (size_t i = 0; i < floor(shard_size+0.5); ++i) {
        shard_contents.push_back(rand() % data_size);
      }
    }
    
    ScorerFactory SF;
    Scorer* scorer = SF.getScorer(score_type, scorerconfig);

    shards.push_back(Data(*scorer));
    shards.back().score_type = score_type;
    shards.back().number_of_scores = number_of_scores;
    shards.back()._sparse_flag = _sparse_flag;
    for (size_t i = 0; i < shard_contents.size(); ++i) {
      shards.back().featdata->add(featdata->get(shard_contents[i]));
      shards.back().scoredata->add(scoredata->get(shard_contents[i]));
    }
    //cerr << endl;
    
  }
}

