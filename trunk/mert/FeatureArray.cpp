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
	outFile << FEATURES_TXT_BEGIN << " " << idx << " " << array_.size()
	        << " " << number_of_features << " " << features << std::endl;
	for (featarray_t::iterator i = array_.begin(); i !=array_.end(); i++){
		i->savetxt(outFile);
		outFile << std::endl;	
	}
	outFile << FEATURES_TXT_END << std::endl;
}

void FeatureArray::savebin(std::ofstream& outFile)
{
	outFile << FEATURES_BIN_BEGIN << " " << idx << " " << array_.size()
	        << " " << number_of_features << " " << features << std::endl;
  for (featarray_t::iterator i = array_.begin(); i !=array_.end(); i++)
		i->savebin(outFile);

	outFile << FEATURES_BIN_END << std::endl;
}


void FeatureArray::save(std::ofstream& inFile, bool bin)
{
	if (size()>0)
		(bin)?savebin(inFile):savetxt(inFile);
}

void FeatureArray::save(const std::string &file, bool bin)
{

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	save(outFile);

	outFile.close();
}

void FeatureArray::loadbin(ifstream& inFile, size_t n)
{
	FeatureStats entry(number_of_features);

	for (size_t i=0 ; i < n; i++){
		entry.loadbin(inFile);
		add(entry);
	}	
}

void FeatureArray::loadtxt(ifstream& inFile, size_t n)
{
	FeatureStats entry(number_of_features);
	
	for (size_t i=0 ; i < n; i++){
		entry.loadtxt(inFile);
		add(entry);
	}	
}

void FeatureArray::load(ifstream& inFile)
{
  size_t number_of_entries=0;
	bool binmode=false;
	
	std::string substring, stringBuf;
  std::string::size_type loc;

	std::getline(inFile, stringBuf);
	if (!inFile.good()){
		return;
	}

	if (!stringBuf.empty()){         
    if ((loc = stringBuf.find(FEATURES_TXT_BEGIN)) == 0){
			binmode=false;
		}else if ((loc = stringBuf.find(FEATURES_BIN_BEGIN)) == 0){
			binmode=true;
		}else{
			TRACE_ERR("ERROR: FeatureArray::load(): Wrong header");
			return;
		}
		getNextPound(stringBuf, substring); 
		getNextPound(stringBuf, substring);
    idx = substring;
		getNextPound(stringBuf, substring);
    number_of_entries = atoi(substring.c_str());
		getNextPound(stringBuf, substring);
    number_of_features = atoi(substring.c_str());
		features = stringBuf;
	}

	(binmode)?loadbin(inFile, number_of_entries):loadtxt(inFile, number_of_entries);

	std::getline(inFile, stringBuf);
	if (!stringBuf.empty()){         
		if ((loc = stringBuf.find(FEATURES_TXT_END)) != 0 && (loc = stringBuf.find(FEATURES_BIN_END)) != 0){
			TRACE_ERR("ERROR: FeatureArray::load(): Wrong footer");
			return;
		}
	}
}

void FeatureArray::load(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	load((ifstream&) inFile);

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

