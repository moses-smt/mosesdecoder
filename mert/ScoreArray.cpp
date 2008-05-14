/*
 *  ScoreArray.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "ScoreArray.h"
#include "Util.h"


ScoreArray::ScoreArray():
bufLen_(0),idx(0)
{};

void ScoreArray::savetxt(std::ofstream& outFile)
{
        ScoreStats entry;

	outFile << SCORES_BEGIN << " " << idx << " " << array_.size() << std::endl;
	for (vector<ScoreStats>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).savetxt(outFile);
	outFile << SCORES_END << std::endl;
}

void ScoreArray::savetxt(const std::string &file)
{
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        ScoreStats entry;

	savetxt(outFile);
}

void ScoreArray::loadtxt(ifstream& inFile)
{
        ScoreStats entry;

        int sentence_index;
        int number_of_entries;
	int nextPound;

        std::string substring, stringBuf, sentence_code = "";
        std::string::size_type loc;


	TRACE_ERR("starting loadtxt..." << std::endl);
	std::getline(inFile, stringBuf);
	if (stringBuf.empty()){
		TRACE_ERR("ERROR: Empty string" << std::endl);
		return;
	}         

	if (!stringBuf.empty()){         
//		TRACE_ERR("Reading: " << stringBuf << std::endl); 
		nextPound = getNextPound(stringBuf, substring);
		nextPound = getNextPound(stringBuf, substring);
       	        idx = atoi(substring.c_str());
		nextPound = getNextPound(stringBuf, substring);
       	        number_of_entries = atoi(substring.c_str());
//		TRACE_ERR("idx: " << idx " nbest: " << number_of_entries <<  std::endl);
	}

	for (int i=0 ; i < number_of_entries; i++)
	{
                entry.clear();
                std::getline(inFile, stringBuf);
		entry.set(stringBuf);
		add(entry);
	}

	std::getline(inFile, stringBuf);
	if (!stringBuf.empty()){         
//		TRACE_ERR("Reading: " << stringBuf << std::endl); 
                if ((loc = stringBuf.find(SCORES_END)) != 0){
			TRACE_ERR("ERROR: ScoreArray::loadtxt(): Wrong footer");
			return;
		}
	}
}

void ScoreArray::loadtxt(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	loadtxt(inFile);
	inFile.close();

}
