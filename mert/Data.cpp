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

	inputfilestream inp(file); // matches a stream with a file. Opens the file

	if (!inp.good())
	        throw runtime_error("Unable to open: " + file);

	std::string substring, subsubstring, stringBuf;
	std::string theSentence;
	std::string theFeatures;
	std::string theAlignment;
	std::string::size_type loc;


	while (getline(inp,stringBuf,'\n')){
		if (stringBuf.empty()) continue;

//		TRACE_ERR("stringBuf: " << stringBuf << std::endl); 

		getNextPound(stringBuf, substring, "|||"); //first field
		sentence_index = substring;

		getNextPound(stringBuf, substring, "|||"); //second field
		theSentence = substring;

// adding statistics for error measures
		featentry.reset();
		scoreentry.clear();



		getNextPound(stringBuf, substring, "|||"); //third field
		theFeatures = substring;

		if (stringBuf.length() > 0) {
		  getNextPound(stringBuf, substring, "|||"); //fourth field sentence score
		  if (stringBuf.length() > 0) {
		    	getNextPound(stringBuf, substring, "|||"); //fourth field only there if alignment scorer
		      theAlignment = substring;
			}
		}
		//TODO check alignment exists if scorers need it

		if (!theScorer->useAlignment()) {
   	  theScorer->prepareStats(sentence_index, theSentence, scoreentry);
		} else {
			//an interpolated score would need both sentence and alignment
		  theSentence += "|||";
			theSentence += theAlignment;
			theScorer->prepareStats(sentence_index, theSentence, scoreentry);
		}

		scoredata->add(scoreentry, sentence_index);

		if (!existsFeatureNames()){
			std::string stringsupport=theFeatures;
			// adding feature names
			std::string features="";
			std::string tmpname="";

			size_t tmpidx=0;
			while (!stringsupport.empty()){
				//			TRACE_ERR("Decompounding: " << substring << std::endl); 
				getNextPound(stringsupport, subsubstring);
				
				// string ending with ":" are skipped, because they are the names of the features
				if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
					features+=tmpname+"_"+stringify(tmpidx)+" ";
					tmpidx++;
				}
				else{
					tmpidx=0;
					tmpname=subsubstring.substr(0,subsubstring.size() - 1);
				}
			}
		
			featdata->setFeatureMap(features);
		}
		
// adding features
		while (!theFeatures.empty()){
//			TRACE_ERR("Decompounding: " << theFeatures << std::endl); 
			getNextPound(theFeatures, subsubstring);

// string ending with ":" are skipped, because they are the names of the features
      if ((loc = subsubstring.find(":")) != subsubstring.length()-1){
				featentry.add(ATOFST(subsubstring.c_str()));
			}
		}
		featdata->add(featentry,sentence_index);
	}
	
	inp.close();
}

