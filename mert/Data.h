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
	size_t number_of_scores; //number of scores
		
public:
	Data(Scorer& sc);
	
	~Data(){};
		
	inline void clear() { scoredata->clear(); featdata->clear(); }
	
	ScoreData* getScoreData() { return scoredata; };
	FeatureData* getFeatureData() { return featdata; };
	
	inline size_t NumberOfFeatures() const{ return featdata->NumberOfFeatures(); }
	inline void NumberOfFeatures(size_t v){ featdata->NumberOfFeatures(v); }
	inline std::string Features() const{ return featdata->Features(); }
	inline void Features(const std::string f){ featdata->Features(f); }

	void loadnbest(const std::string &file);

  void load(const std::string &featfile,const std::string &scorefile){
		featdata->load(featfile);
		scoredata->load(scorefile);
  }
	
	void save(const std::string &featfile,const std::string &scorefile, bool bin=false){
		
		if (bin) cerr << "Binary write mode is selected" << endl;
		else cerr << "Binary write mode is NOT selected" << endl;
		
		featdata->save(featfile, bin);
		scoredata->save(scorefile, bin);
	}

	inline bool existsFeatureNames(){ return featdata->existsFeatureNames(); };
	
	inline std::string getFeatureName(size_t idx){ return featdata->getFeatureName(idx); };
	inline size_t getFeatureIndex(const std::string& name){ return featdata->getFeatureIndex(name); };
};


#endif
