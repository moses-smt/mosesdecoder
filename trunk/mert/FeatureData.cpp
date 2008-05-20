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


FeatureData::FeatureData() {};

void FeatureData::save(std::ofstream& outFile, bool bin)
{
	for (vector<FeatureArray>::iterator i = array_.begin(); i !=array_.end(); i++)
		(*i).save(outFile, bin);
}

void FeatureData::save(const std::string &file, bool bin)
{
	if (file.empty()) return;

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
		

		if (!inFile.good()){
			std::cerr << "ERROR FeatureData::load inFile.good()" << std::endl; 
		}

		entry.clear();
		entry.load(inFile);

		if (entry.size() == 0){
			return;
		}
		add(entry);
		iter++;
	}
}


void FeatureData::load(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);  

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	if (!inFile) {
        	throw runtime_error("Unable to open feature file: " + file);
	}

	load((ifstream&) inFile);

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

