/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureStats.h"

#define AVAILABLE_ 8;


FeatureStats::FeatureStats()
{
	available_ = AVAILABLE_;
	entries_ = 0;
	array_ = new FeatureStatsType[available_];
};

FeatureStats::~FeatureStats()
{
	delete array_;
};

FeatureStats::FeatureStats(const FeatureStats &stats)
{
	available_ = stats.available();
	entries_ = stats.size();
	array_ = new FeatureStatsType[available_];
	memcpy(array_,stats.getArray(),featbytes_);
};

FeatureStats::FeatureStats(const size_t size)
{
	available_ = size;
	entries_ = size;
	array_ = new FeatureStatsType[available_];
	memset(array_,0,featbytes_);
};


FeatureStats::FeatureStats(std::string &theString)
{
	set(theString);
}

void FeatureStats::expand()
{
	available_*=2;
	featstats_t t_ = new FeatureStatsType[available_];
	memcpy(t_,array_,featbytes_);
	delete array_;
	array_=t_;
}

void FeatureStats::add(FeatureStatsType v)
{
	if (isfull()) expand();
	array_[entries_++]=v;
}

void FeatureStats::set(std::string &theString)
{
  std::string substring, stringBuf;
	reset();
	
	while (!theString.empty()){         
		getNextPound(theString, substring);
		add(ATOFST(substring.c_str()));
	}
}


void FeatureStats::loadbin(std::ifstream& inFile)
{
	inFile.read((char*) array_, featbytes_);
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
//	TRACE_ERR("saving the stats" << std::endl);  
	outFile << *this;
}

void FeatureStats::savebin(std::ofstream& outFile)
{
	outFile.write((char*) array_, featbytes_);
} 

FeatureStats& FeatureStats::operator=(const FeatureStats &stats)
{
	delete array_;
	available_ = stats.available();
	entries_ = stats.size();
	array_ = new FeatureStatsType[available_];
	memcpy(array_,stats.getArray(),featbytes_);
		
	return *this;		
}


/**write the whole object to a stream*/
ostream& operator<<(ostream& o, const FeatureStats& e){
	for (size_t i=0; i< e.size(); i++)
		o << e.get(i) << " ";
	return o;
}
