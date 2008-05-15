/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureStats.h"


FeatureStats::FeatureStats()
{};

 FeatureStats::FeatureStats(const FeatureStats &stats):
array_(stats.array_)
{};

FeatureStats::FeatureStats(const size_t size)
{
	for(int i = 0; i < size; i++)
		array_.push_back(0);
};


FeatureStats::FeatureStats(std::string &theString)
{
	set(theString);
}

void FeatureStats::set(std::string &theString)
{
        std::string substring, stringBuf;
        std::string::size_type loc;
	int nextPound;
	FeatureStatsType sc;
//	TRACE_ERR("Decompounding string: " << theString << std::endl);      
	while (!theString.empty()){         
		nextPound = getNextPound(theString, substring);
		sc = ATOFST(substring.c_str());
		array_.push_back(sc);
	}
}

void FeatureStats::loadtxt(std::ifstream& inFile)
{
        std::string theString;
	std::getline(inFile, theString);
	set(theString);
}

void FeatureStats::loadtxt(const std::string &file)
{
//	TRACE_ERR("loading the stats from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	loadtxt(inFile);
}


void FeatureStats::savetxt(const std::string &file)
{
//	TRACE_ERR("saving the stats into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	savetxt(outFile);
}


void FeatureStats::savetxt(std::ofstream& outFile)
{
	vector<FeatureStatsType>::iterator i = array_.begin();
	outFile << *i;
	i++;
        while (i !=array_.end()){
		outFile << " " << *i;
		i++;
	}
	outFile << std::endl;
}


FeatureStats& FeatureStats::operator=(const FeatureStats &stats)
{
	array_ = stats.array_;
		
	return *this;		
}


