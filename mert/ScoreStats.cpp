/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "ScoreStats.h"


ScoreStats::ScoreStats()
{};

 ScoreStats::ScoreStats(const ScoreStats &stats):
array_(stats.array_)
{};

ScoreStats::ScoreStats(const size_t size)
{
	for(unsigned int i = 0; i < size; i++)
		array_.push_back(0);
};


ScoreStats::ScoreStats(std::string &theString)
{
	set(theString);
}

void ScoreStats::set(std::string &theString)
{
    std::string substring, stringBuf;

	int nextPound;
	ScoreStatsType sc;
	while (!theString.empty()){         
        nextPound = getNextPound(theString, substring);
        sc = ATOSST(substring.c_str());
        array_.push_back(sc);
	}
}

void ScoreStats::loadtxt(std::ifstream& inFile)
{
  std::string theString;
	std::getline(inFile, theString);
	set(theString);
}

void ScoreStats::loadtxt(const std::string &file)
{
//	TRACE_ERR("loading the stats from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	loadtxt(inFile);
}


void ScoreStats::savetxt(const std::string &file)
{
//	TRACE_ERR("saving the stats into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	savetxt(outFile);
}


void ScoreStats::savetxt(std::ofstream& outFile)
{
	outFile << *this;
}


ScoreStats& ScoreStats::operator=(const ScoreStats &stats)
{
	array_ = stats.array_;
		
	return *this;		
}


/**write the whole object to a stream*/
ostream& operator<<(ostream& o, const ScoreStats& e){
	for (scorestats_t::const_iterator i = e.array_.begin(); i != e.array_.end(); i++)
		o << *i << " ";
	return o;
}

