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

	inputfilestream inp(file); // matches a stream with a file. Opens the file

	if (!inp.good())
	        throw runtime_error("Unable to open: " + file);

        std::string substring, subsubstring, stringBuf;
	std::string theSentence;
	std::string::size_type loc;

        while (getline(inp,stringBuf,'\n')){
		if (stringBuf.empty()) continue;

		nextPound = getNextPound(stringBuf, substring, "|||"); //first field
       	        sentence_index = atoi(substring.c_str());

                nextPound = getNextPound(stringBuf, substring, "|||"); //second field
                theSentence = substring;

// adding statistics for error measures
		featentry.clear();
		scoreentry.clear();
                theScorer->prepareStats(sentence_index, theSentence, scoreentry);
                scoredata->add(scoreentry, sentence_index);

		nextPound = getNextPound(stringBuf, substring, "|||"); //third field

// adding features
		while (!substring.empty()){
//			TRACE_ERR("Decompounding: " << substring << std::endl); 
			nextPound = getNextPound(substring, subsubstring);

// string ending with ":" are skipped, because they are the names of the features
	                if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
				featentry.add(ATOFST(subsubstring.c_str()));
			}
		}
		featdata->add(featentry,sentence_index);
	}

	inp.close();
}
