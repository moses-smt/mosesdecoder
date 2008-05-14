/*
 *  FeatureArray.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureArray.h"
#include "Util.h"


FeatureArray::FeatureArray():
bufLen_(0),idx(0)
{};

void FeatureArray::savetxt(std::ofstream& outFile)
{
        FeatureStats entry;

	outFile << FEATURES_TXT_BEGIN << " " << idx << " " << array_.size() << std::endl;
	for (vector<FeatureStats>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).savetxt(outFile);
	outFile << FEATURES_TXT_END << std::endl;
}

void FeatureArray::savebin(std::ofstream& outFile)
{
        FeatureStats entry;

	TRACE_ERR("binary saving is not yet implemented!" << std::endl);  

/*
NOT YET IMPLEMENTED
*/
	outFile << FEATURES_TXT_BEGIN << " " << idx << " " << array_.size() << std::endl;
	outFile << FEATURES_BIN_END << std::endl;

}

void FeatureArray::savetxt(const std::string &file)
{
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        FeatureStats entry;

	savetxt(outFile);
}

void FeatureArray::loadtxt(ifstream& inFile)
{
        FeatureStats entry;

        int sentence_index;
        int number_of_entries;
	int nextPound;

        std::string substring, stringBuf;
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
                if ((loc = stringBuf.find(FEATURES_END)) != 0){
			TRACE_ERR("ERROR: FeatureArray::loadtxt(): Wrong footer");
			return;
		}
	}
}

void FeatureArray::loadtxt(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	loadtxt(inFile);
	inFile.close();

}

