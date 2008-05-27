/*
 *  Data.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#ifndef DATA_H
#define DATA_H

using namespace std;

#include <limits>
#include <vector>
#include <iostream>

#include "Util.h"
#include "FeatureData.h"
#include "ScoreData.h"

class Scorer;

class Data
{
protected:
	ScoreData* scoredata;
	FeatureData* featdata;
		
private:
  Scorer* theScorer;       
  std::string score_type;
	map<std::string, size_t> featname2idx_; //map from name to index of features
	map<size_t, std::string> idx2featname_; //map from index to name of features
	size_t number_of_features; //number of features 
	size_t number_of_scores; //number of scores
		
public:
	Data(Scorer& sc);
	
	~Data(){};
		
	inline void clear() { scoredata->clear(); featdata->clear(); }
	
	ScoreData* getScoreData() { return scoredata; };
	FeatureData* getFeatureData() { return featdata; };

	void loadnbest(const std::string &file);

  void load(const std::string &featfile,const std::string &scorefile){
		featdata->load(featfile);
		scoredata->load(scorefile);
  }
	
	void save(const std::string &featfile,const std::string &scorefile, bool bin=false){
		featdata->save(featfile, bin);
		scoredata->save(scorefile,bin);
	}

	bool existsFeatureNames(){ return (idx2featname_.size() > 0)?true:false; };
	std::string getFeatureName(size_t idx){ return idx2featname_[idx]; };
  size_t getFeatureIndex(const std::string& name){ return featname2idx_[name]; };
};


#endif
