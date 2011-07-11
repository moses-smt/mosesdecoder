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

ScoreArray::ScoreArray(): idx("")
{};

void ScoreArray::savetxt(std::ofstream& outFile, const std::string& sctype)
{
	outFile << SCORES_TXT_BEGIN << " " << idx << " " << array_.size()
         	<< " " << number_of_scores << " " << sctype << std::endl;
	for (scorearray_t::iterator i = array_.begin(); i !=array_.end(); i++){
		i->savetxt(outFile);
		outFile << std::endl;	
	}
	outFile << SCORES_TXT_END << std::endl;
}

void ScoreArray::savebin(std::ofstream& outFile, const std::string& sctype)
{
	outFile << SCORES_BIN_BEGIN << " " << idx << " " << array_.size()
	<< " " << number_of_scores << " " << sctype << std::endl;
	for (scorearray_t::iterator i = array_.begin(); i !=array_.end(); i++)
		i->savebin(outFile);
	
	outFile << SCORES_BIN_END << std::endl;
}

void ScoreArray::save(std::ofstream& inFile, const std::string& sctype, bool bin)
{
	if (size()>0)
		(bin)?savebin(inFile, sctype):savetxt(inFile, sctype);
}

void ScoreArray::save(const std::string &file, const std::string& sctype, bool bin)
{
        std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        save(outFile, sctype, bin);

        outFile.close();
}

void ScoreArray::loadbin(ifstream& inFile, size_t n)
{
	ScoreStats entry(number_of_scores);
	
	for (size_t i=0 ; i < n; i++){
		entry.loadbin(inFile);
		add(entry);
	}	
}

void ScoreArray::loadtxt(ifstream& inFile, size_t n)
{
	ScoreStats entry(number_of_scores);
	
	for (size_t i=0 ; i < n; i++){
		entry.loadtxt(inFile);
		add(entry);
	}	
}

void ScoreArray::load(ifstream& inFile)
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
    if ((loc = stringBuf.find(SCORES_TXT_BEGIN)) == 0){
			binmode=false;
		}else if ((loc = stringBuf.find(SCORES_BIN_BEGIN)) == 0){
			binmode=true;
		}else{
			TRACE_ERR("ERROR: ScoreArray::load(): Wrong header");
			return;
		}
		getNextPound(stringBuf, substring);
		getNextPound(stringBuf, substring);
	  idx = substring;
		getNextPound(stringBuf, substring);
    number_of_entries = atoi(substring.c_str());
		getNextPound(stringBuf, substring);
    number_of_scores = atoi(substring.c_str());
		getNextPound(stringBuf, substring);
	  score_type = substring;
	}
	
	(binmode)?loadbin(inFile, number_of_entries):loadtxt(inFile, number_of_entries);
	
	std::getline(inFile, stringBuf);
	if (!stringBuf.empty()){         
		if ((loc = stringBuf.find(SCORES_TXT_END)) != 0 && (loc = stringBuf.find(SCORES_BIN_END)) != 0){
			TRACE_ERR("ERROR: ScoreArray::load(): Wrong footer");
			return;
		}
	}
}

void ScoreArray::load(const std::string &file)
{
	TRACE_ERR("loading data from " << file << std::endl);   

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	load((ifstream&) inFile);

	inFile.close();
}


void ScoreArray::merge(ScoreArray& e)
{
	//dummy implementation
	for (size_t i=0; i<e.size(); i++)
		add(e.get(i));
}

bool ScoreArray::check_consistency()
{
	size_t sz = NumberOfScores();
	
	if (sz == 0)
		return true;
	
	for (scorearray_t::iterator i=array_.begin(); i!=array_.end(); i++)
		if (i->size()!=sz)
			return false;
	return true;
}


