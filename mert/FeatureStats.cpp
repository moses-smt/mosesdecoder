/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureStats.h"


FeatureStats::FeatureStats():
bufLen_(0)
{};

 FeatureStats::FeatureStats(const FeatureStats &stats):
array_(stats.array_),
bufLen_(0)
{};

FeatureStats::FeatureStats(const size_t size):
bufLen_(0)
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

	outFile << array_.at(0);
	vector<FeatureStatsType>::iterator i = array_.begin();
	outFile << " " << *i;
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
	bufLen_ = 0;
		
	return *this;		
}

void FeatureStats::setBuffer(char* buffer, size_t sz)
{
	memcpy(databuf_, (char *)buffer, sz);
		
	// Now pack the data into a single contiguous memory location for storage.
	bufLen_ = 0;
	
	unpackVector(databuf_, bufLen_, array_);
}

/*
 * Marshalls this classes data members into a single
 * contiguous memory location for the purpose of storing
 * the data in a database.
 */
char *FeatureStats::getBuffer()
{
	// Zero out the buffer
	memset(databuf_, 0, BUFSIZ);
		
	// Now pack the data into a single contiguous memory location for storage.
	bufLen_ = 0;

	packVector(databuf_, bufLen_, array_);
	return databuf_;
}

int FeatureStats::pack(char *buffer, size_t &bufferlen)
{
	getBuffer();
	size_t size = packVariable(buffer, bufferlen, bufLen_);
	memcpy(buffer + bufferlen, databuf_, bufLen_);
	bufferlen += bufLen_;

	return size + bufLen_;
}

int FeatureStats::unpack(char *buffer, size_t &bufferlen)
{
	size_t size = unpackVariable(buffer, bufferlen, bufLen_);
	memcpy(databuf_, buffer + bufferlen, bufLen_);
	bufferlen += bufLen_;
	setBuffer(databuf_, bufLen_);
	
	return size + bufLen_;
}


