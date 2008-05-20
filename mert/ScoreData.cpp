/*
 *  ScoreData.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "ScoreData.h"
#include "Scorer.h"
#include "Util.h"


ScoreData::ScoreData(Scorer& ptr):
theScorer(&ptr)
{
	score_type = theScorer->getName();
	TRACE_ERR("score_type:" << score_type << std::endl);
	theScorer->setScoreData(this);//this is not dangerous: we dont use the this pointer in SetScoreData 	
};

void ScoreData::save(std::ofstream& outFile, bool bin)
{
        ScoreArray entry;
	for (vector<ScoreArray>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).save(outFile);
}

void ScoreData::save(const std::string &file, bool bin)
{
	if (file.empty()) return;
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        ScoreStats entry;

	save(outFile, bin);

	outFile.close();
}

void ScoreData::load(ifstream& inFile)
{
        ScoreArray entry;

	int iter=0;
	while (!inFile.eof()){
		TRACE_ERR("iter " << iter << " size " << size() << std::endl);
		
		entry.clear();
		entry.loadtxt(inFile);

		if (entry.size() == 0){
			continue;
		}
		add(entry);
		iter++;
	}
}


void ScoreData::load(const std::string &file)
{
	TRACE_ERR("loading score data from " << file << std::endl); 

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	if (!inFile) {
        	throw runtime_error("Unable to open score file: " + file);
	}

	load((ifstream&) inFile); 

	inFile.close();
}

void ScoreData::add(const ScoreStats& e, int sent_idx){
	if (exists(sent_idx)){
		array_.at(sent_idx).add(e);
	}
	else{
		ScoreArray a;
		a.add(e);
		a.setIndex(sent_idx);
		add(a);
	}
 }
