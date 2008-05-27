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


FeatureArray::FeatureArray(): idx("")
{};

void FeatureArray::savetxt(std::ofstream& outFile)
{

	outFile << FEATURES_TXT_BEGIN << " " << idx << " " << array_.size() << std::endl;
	for (featarray_t::iterator i = array_.begin(); i !=array_.end(); i++){
		i->savetxt(outFile);
		outFile << std::endl;	
	}
	outFile << FEATURES_TXT_END << std::endl;
}

void FeatureArray::savebin(std::ofstream& outFile)
{
	TRACE_ERR("binary saving is not yet implemented!" << std::endl);  

/*
NOT YET IMPLEMENTED
*/
	outFile << FEATURES_TXT_BEGIN << " " << idx << " " << array_.size() << std::endl;
	outFile << FEATURES_BIN_END << std::endl;

}


void FeatureArray::save(std::ofstream& inFile, bool bin)
{
	(bin)?savebin(inFile):savetxt(inFile);
}

void FeatureArray::save(const std::string &file, bool bin)
{
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	save(outFile);

	outFile.close();
}

void FeatureArray::loadtxt(ifstream& inFile)
{
	FeatureStats entry;

  int number_of_entries=0;
	int nextPound;

	std::string substring, stringBuf;
  std::string::size_type loc;


	std::getline(inFile, stringBuf);
	if (!inFile.good()){
		return;
	}

	if (!stringBuf.empty()){         
		TRACE_ERR("Reading: " << stringBuf << std::endl); 
                if ((loc = stringBuf.find(FEATURES_TXT_BEGIN)) != 0){
			TRACE_ERR("ERROR: FeatureArray::loadtxt(): Wrong header");
			return;
		}
		nextPound = getNextPound(stringBuf, substring); 
		nextPound = getNextPound(stringBuf, substring);
    idx = substring;
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
                if ((loc = stringBuf.find(FEATURES_TXT_END)) != 0){
			TRACE_ERR("ERROR: FeatureArray::loadtxt(): Wrong footer");
			return;
		}
	}
}

void FeatureArray::loadbin(ifstream& inFile)
{
	TRACE_ERR("binary saving is not yet implemented!" << std::endl);  

/*
NOT YET IMPLEMENTED
*/
}

void FeatureArray::load(ifstream& inFile, bool bin)
{
	(bin)?loadbin(inFile):loadtxt(inFile);
}

void FeatureArray::load(const std::string &file, bool bin)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	load((ifstream&) inFile, bin);

	inFile.close();

}

void FeatureArray::merge(FeatureArray& e)
{
	//dummy implementation
	for (size_t i=0; i<e.size(); i++)
		add(e.get(i));
}



bool FeatureArray::check_consistency()
{
	size_t sz = NumberOfFeatures();
	
	if (sz == 0)
		return true;
	
	for (featarray_t::iterator i=array_.begin(); i!=array_.end(); i++)
		if (i->size()!=sz)
			return false;
	
	return true;
}

