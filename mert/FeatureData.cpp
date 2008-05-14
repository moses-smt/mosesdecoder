/*
 *  FeatureData.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "FeatureData.h"
#include "Util.h"


FeatureData::FeatureData():
bufLen_(0)
{};

void FeatureData::save(std::ofstream& outFile, bool bin)
{
	for (vector<FeatureArray>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).save(outFile, bin);
}

void FeatureData::save(const std::string &file, bool bin)
{
	TRACE_ERR("saving the array into " << file << std::endl);  

	std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	save(outFile, bin);

	outFile.close();
}

void FeatureData::load(ifstream& inFile)
{
        FeatureArray entry;

	int iter=0;
	while (!inFile.eof()){
		TRACE_ERR("iter " << iter << " size " << size() << std::endl);
		
		entry.clear();
		entry.load(inFile);

		if (entry.size() == 0){
			TRACE_ERR("no more data" << std::endl);
			continue;
		}
		add(entry);
		iter++;
	}
}


void FeatureData::load(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

	load(inFile);

	inFile.close();
}

void FeatureData::loadnbest(const std::string &file)
{
	TRACE_ERR("loading nbest from " << file << std::endl);  

	FeatureStats entry;
        int sentence_index;
	int nextPound;

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file


	while (!inFile.eof()){

	        std::string substring, subsubstring, stringBuf;
        	std::string::size_type loc;

		std::getline(inFile, stringBuf);
		if (stringBuf.empty()) continue;

//		TRACE_ERR("Reading: " << stringBuf << std::endl); 

		nextPound = getNextPound(stringBuf, substring, "|||"); //first field
       	        sentence_index = atoi(substring.c_str());
		nextPound = getNextPound(stringBuf, substring, "|||"); //second field
		nextPound = getNextPound(stringBuf, substring, "|||"); //third field

		entry.clear();
		while (!substring.empty()){
//			TRACE_ERR("Decompounding: " << substring << std::endl); 
			nextPound = getNextPound(substring, subsubstring);
	                if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
				entry.add(ATOFST(subsubstring.c_str()));
			}
		}
//		entry.save();
		add(entry,sentence_index);
	}

	inFile.close();
}

void FeatureData::add(FeatureArray& e){
	if (e.getIndex() < size()){ // array at poistion e.getIndex() already exists
		//enlarge array at position e.getIndex()
		array_.at(e.getIndex()).merge(e);
		setIndex();
	}
	else{
		array_.push_back(e);
	}
}

void FeatureData::add(FeatureStats e, int sent_idx){
	if (exists(sent_idx)){
//		TRACE_ERR("Inserting in array " << sent_idx << std::endl); 
		array_.at(sent_idx).add(e);
		FeatureArray a=get(sent_idx);;
//		TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl); 
	}
	else{
//		TRACE_ERR("Creating a new entry in the array" << std::endl); 
		FeatureArray a;
		a.add(e);
		a.setIndex(sent_idx);
		add(a);
//		TRACE_ERR("size: " << size() << " -> " << a.size() << std::endl); 
	}
 }