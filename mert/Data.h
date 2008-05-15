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
	vector<int> idxmap_;
	
private:
	char databuf_[BUFSIZ];
	size_t bufLen_;
        Scorer* theScorer;       
        std::string score_type;
	
public:
	Data(Scorer& sc);
	
	~Data(){};
		
	inline void clear() { scoredata->clear(); featdata->clear(); }
	
	void loadnbest(const std::string &file);


        void load(const std::string &featfile,const std::string &scorefile){
		featdata->load(featfile);
//		scoredata->load(scorefile);
		scoredata->load(scorefile);
}
	void save(const std::string &featfile,const std::string &scorefile, bool bin=false){
		featdata->save(featfile, bin);
//		scoredata->save(scorefile, bin);
		scoredata->save(scorefile);
	}
};


#endif
