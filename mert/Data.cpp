/*
 *  Data.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <fstream>
#include "Scorer.h"
#include "Data.h"
#include "Util.h"


Data::Data(Scorer& ptr):
bufLen_(0), theScorer(&ptr)
{
    score_type = (*theScorer).getName();
    featdata=new FeatureData;
    scoredata=new ScoreData(*theScorer);
};

void Data::loadnbest(const std::string &file)
{
	TRACE_ERR("loading nbest from " << file << std::endl);  

	FeatureStats featentry;
	ScoreStats scoreentry;
        int sentence_index;
	int nextPound;

	std::ifstream inFile(file.c_str(), std::ios::in); // matches a stream with a file. Opens the file

    if (!inFile) {
        throw runtime_error("Unable to open: " + file);
    }

	while (!inFile.eof()){

	        std::string substring, subsubstring, stringBuf;
                std::string theSentence;
        	std::string::size_type loc;

		std::getline(inFile, stringBuf);
		if (stringBuf.empty()) continue;

//		TRACE_ERR("Reading: " << stringBuf << std::endl); 

		nextPound = getNextPound(stringBuf, substring, "|||"); //first field
       	        sentence_index = atoi(substring.c_str());

                nextPound = getNextPound(stringBuf, substring, "|||"); //second field
                theSentence = substring;

// adding statistics for error measures
                scoreentry.clear();
                theScorer->prepareStats(sentence_index, theSentence,scoreentry);
                scoredata->add(scoreentry,sentence_index);


		nextPound = getNextPound(stringBuf, substring, "|||"); //third field

// adding features
		featentry.clear();
		scoreentry.clear();
		while (!substring.empty()){
//			TRACE_ERR("Decompounding: " << substring << std::endl); 
			nextPound = getNextPound(substring, subsubstring);
	                if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
				featentry.add(ATOFST(subsubstring.c_str()));
			}
		}
		featdata->add(featentry,sentence_index);
	}

	inFile.close();
}
