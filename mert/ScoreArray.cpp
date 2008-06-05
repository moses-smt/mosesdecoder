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
	outFile << SCORES_TXT_BEGIN << " " << sctype << " " << idx << " " << array_.size() << std::endl;
	for (scorearray_t::iterator i = array_.begin(); i !=array_.end(); i++){
		i->savetxt(outFile);
		outFile << std::endl;	
	}
	outFile << SCORES_TXT_END << std::endl;
}

void ScoreArray::savebin(std::ofstream& outFile, const std::string& sctype)
{
        TRACE_ERR("binary saving is not yet implemented!" << std::endl);  

/*
NOT YET IMPLEMENTED
*/
        outFile << SCORES_BIN_BEGIN << " " << sctype << " " << idx << " " << array_.size() << std::endl;
        outFile << SCORES_BIN_END << std::endl;

}


void ScoreArray::save(std::ofstream& inFile, const std::string& sctype, bool bin)
{
        (bin)?savebin(inFile, sctype):savetxt(inFile, sctype);
}

void ScoreArray::save(const std::string &file, const std::string& sctype, bool bin)
{
        TRACE_ERR("saving the array into " << file << std::endl);  

        std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

        save(outFile, sctype, bin);

        outFile.close();
}

void ScoreArray::loadtxt(ifstream& inFile)
{
	ScoreStats entry;

  int number_of_entries=0;
	int nextPound;

  std::string substring, stringBuf, sentence_code = "";
  std::string::size_type loc;


	std::getline(inFile, stringBuf);
	if (!inFile.good()){
		return;
	}

	if (!stringBuf.empty()){         
//		TRACE_ERR("Reading: " << stringBuf << std::endl); 
		nextPound = getNextPound(stringBuf, substring);
		nextPound = getNextPound(stringBuf, substring);
	  score_type = substring;
		nextPound = getNextPound(stringBuf, substring);
	  idx = substring;
		nextPound = getNextPound(stringBuf, substring);
    number_of_entries = atoi(substring.c_str());
//		TRACE_ERR("idx: " << idx " nbest: " << number_of_entries <<  std::endl);
		/*PUT HERE A CONSISTENCY CHECK ABOUT THE FORMAT OF THE FILE*/
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
		if ((loc = stringBuf.find(SCORES_TXT_END)) != 0){
			TRACE_ERR("ERROR: ScoreArray::loadtxt(): Wrong footer");
			return;
		}
	}
}

void ScoreArray::loadbin(ifstream& inFile)
{
        TRACE_ERR("binary saving is not yet implemented!" << std::endl);  

/*
NOT YET IMPLEMENTED
*/
}

void ScoreArray::load(ifstream& inFile, bool bin)
{
        (bin)?loadbin(inFile):loadtxt(inFile);
}

void ScoreArray::load(const std::string &file , bool bin)
{
	TRACE_ERR("loading data from " << file << std::endl);   

	inputfilestream inFile(file); // matches a stream with a file. Opens the file

	load((ifstream&) inFile, bin);

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


