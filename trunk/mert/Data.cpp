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
theScorer(&ptr)
{
	score_type = (*theScorer).getName();
	TRACE_ERR("Data::score_type " << score_type << std::endl);  
	
	TRACE_ERR("Data::Scorer type from Scorer: " << theScorer->getName() << endl);
  featdata=new FeatureData;
  scoredata=new ScoreData(*theScorer);
};

void Data::loadnbest(const std::string &file)
{
	TRACE_ERR("loading nbest from " << file << std::endl);  

	FeatureStats featentry;
	ScoreStats scoreentry;
	std::string sentence_index;
	int nextPound;

	inputfilestream inp(file); // matches a stream with a file. Opens the file

	if (!inp.good())
	        throw runtime_error("Unable to open: " + file);

	std::string substring, subsubstring, stringBuf;
	std::string theSentence;
	std::string::size_type loc;

	while (getline(inp,stringBuf,'\n')){
		if (stringBuf.empty()) continue;

//		TRACE_ERR("stringBuf: " << stringBuf << std::endl); 

		nextPound = getNextPound(stringBuf, substring, "|||"); //first field
		sentence_index = substring;

		nextPound = getNextPound(stringBuf, substring, "|||"); //second field
		theSentence = substring;

// adding statistics for error measures
		featentry.reset();
		scoreentry.clear();

		theScorer->prepareStats(sentence_index, theSentence, scoreentry);
		scoredata->add(scoreentry, sentence_index);
				
		nextPound = getNextPound(stringBuf, substring, "|||"); //third field

		if (!existsFeatureNames()){
		std::string stringsupport=substring;
		// adding feature names
		std::string tmpname="";
		size_t tmpidx=0;
		while (!stringsupport.empty()){
//			TRACE_ERR("Decompounding: " << substring << std::endl); 
			nextPound = getNextPound(stringsupport, subsubstring);
			
			// string ending with ":" are skipped, because they are the names of the features
			if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
				featname2idx_[tmpname+"_"+stringify(tmpidx)]=idx2featname_.size();
				idx2featname_[idx2featname_.size()]=tmpname+"_"+stringify(tmpidx);
				tmpidx++;
			}
			else{
				tmpidx=0;
				tmpname=subsubstring.substr(0,subsubstring.size() - 1);
			}
		}
		std::string features="";
		for (size_t i=0; i<idx2featname_.size(); i++)
			features+=idx2featname_[i]+" ";

		NumberOfFeatures(idx2featname_.size());
		Features(features);
		TRACE_ERR("number_of_features: " << NumberOfFeatures() << std::endl); 
		TRACE_ERR("features: " << Features() << std::endl); 
		}
		
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
