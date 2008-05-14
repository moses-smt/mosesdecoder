/*
 *  ScoreData.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "ScoreData.h"
#include "Util.h"


ScoreData::ScoreData():
bufLen_(0)
{};

void ScoreData::savetxt(std::ofstream& outFile)
{
        ScoreArray entry;
	for (vector<ScoreArray>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).savetxt(outFile);
}

void ScoreData::savetxt(const std::string &file)
{
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        ScoreStats entry;

	savetxt(outFile);
}

void ScoreData::loadtxt(ifstream& inFile)
{
        ScoreArray entry;

	int iter=0;
	while (!inFile.eof()){
		TRACE_ERR("iter " << iter << " size " << size() << std::endl);
		
		entry.clear();
		entry.loadtxt(inFile);

		if (entry.size() == 0){
			TRACE_ERR("no more data" << std::endl);
			continue;
		}
		entry.savetxt();

		add(entry);
		
		savetxt();
		iter++;
	}
}


void ScoreData::loadtxt(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	loadtxt(inFile);

	inFile.close();
}

void ScoreData::loadnbest(const std::string &file)
{
	TRACE_ERR("loading nbest from " << file << std::endl);  

	ScoreStats entry;
        int sentence_index;
	int nextPound;

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file


	while (!inFile.eof()){

	        std::string substring, subsubstring, stringBuf;
	        std::string theSentence;
        	std::string::size_type loc;

		std::getline(inFile, stringBuf);
		if (stringBuf.empty()) continue;

//		TRACE_ERR("Reading: " << stringBuf << std::endl); 

		nextPound = getNextPound(stringBuf, substring, "|||"); //first field
       	        sentence_index = atoi(substring.c_str());
		nextPound = getNextPound(stringBuf, substring, "|||"); //second field
		theSentence = substring;


		entry.clear();

/* HERE IS THE SECTION TO COMPUTE STATISTICS FOR ERROR MEASURE
 * theSentence contains the translation 
 * append each statistic in the entry object
 * with a command like the following: 
 *
 * entry.add(value);
 *
 */


/* DO NOT MODIFY BELOW */
//		entry.savetxt();
		add(entry,sentence_index);
	}

	inFile.close();
}



void ScoreData::add(ScoreStats e, int sent_idx){
	if (exists(sent_idx)){
		array_.at(sent_idx).add(e);
		ScoreArray a=get(sent_idx);;
	}
	else{
		ScoreArray a;
		a.add(e);
		a.setIndex(sent_idx);
		add(a);
	}
 }
