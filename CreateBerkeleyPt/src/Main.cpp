#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include <iostream>
#include <string>
#include "../../BerkeleyPt/src/TargetPhraseCollection.h"
#include "../../BerkeleyPt/src/Phrase.h"
#include "../../BerkeleyPt/src/SourcePhrase.h"
#include "../../BerkeleyPt/src/TargetPhrase.h"
#include "../../BerkeleyPt/src/Vocab.h"
#include "../../BerkeleyPt/src/DbWrapper.h"
#include "../../moses/src/InputFileStream.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/UserMessage.h"

using namespace std;
using namespace MosesBerkeleyPt;

void Save(map<SourcePhrase, TargetPhraseCollection> &tpCollMap, DbWrapper &dbWrapper)
{
	//write out all target phrase colls
	map<SourcePhrase, TargetPhraseCollection>::const_iterator iter;
	for (iter = tpCollMap.begin(); iter != tpCollMap.end(); ++iter)
	{ // could be 1st. tpColl == NULL
		const SourcePhrase &sourcePhrase = iter->first;
		long sourceNodeId = sourcePhrase.Save(dbWrapper.GetSourceDb(), dbWrapper.GetNextSourceNodeId(), dbWrapper.GetSourceWordSize());
		
		const TargetPhraseCollection &tpColl = iter->second;
		dbWrapper.SaveTargetPhraseCollection(sourceNodeId, tpColl);
	}	
	
	// delete all tp coll
	tpCollMap.clear();			
}

int main (int argc, char * const argv[])
{
    // insert code here...
	Moses::ResetUserTime();
	Moses::PrintUserTime("Starting");

	assert(argc == 6);

	int numSourceFactors		= Moses::Scan<int>(argv[1])
			, numTargetFactors	= Moses::Scan<int>(argv[2])
			, numScores					= Moses::Scan<int>(argv[3]);
	string filePath = argv[4]
				,destPath = argv[5];

	Moses::InputFileStream inStream(filePath);

	DbWrapper dbWrapper;
	bool retDb = dbWrapper.BeginSave(destPath, numSourceFactors, numTargetFactors, numScores);
	assert(retDb);
	
	vector<string> tokens;
	string line, prevSourcePhraseStr;
	size_t lineNum = 0;
	map<SourcePhrase, TargetPhraseCollection> tpCollMap;
		// long = sourceNode id
	
	while(getline(inStream, line))
	{
		lineNum++;
    if (lineNum%1000 == 0) cerr << "." << flush;
    if (lineNum%10000 == 0) cerr << ":" << flush;
    if (lineNum%100000 == 0) cerr << "!" << flush;
		
		line = Moses::Trim(line);
		if (line.size() == 0)
			continue;
		
		tokens.clear();
		Moses::TokenizeMultiCharSeparator(tokens, line , "|||" );
		
		assert(tokens.size() == 5 || tokens.size() == 6);
		
		// words
		const string &headWordsStr		= tokens[0]
								,&sourcePhraseStr	= tokens[1]
								,&targetPhraseStr	= tokens[2]
								,&alignStr				= tokens[3]
								,&scoresStr				= tokens[4];
						
		SourcePhrase sourcePhrase;
		sourcePhrase.CreateFromString(sourcePhraseStr, dbWrapper.GetVocab());
		
		TargetPhrase *targetPhrase = new TargetPhrase();
		targetPhrase->CreateFromString(targetPhraseStr, dbWrapper.GetVocab());
		targetPhrase->CreateAlignFromString(alignStr);
		targetPhrase->CreateScoresFromString(scoresStr, numScores);
		targetPhrase->CreateHeadwordsFromString(headWordsStr, dbWrapper.GetVocab());
		
		if (tokens.size() >= 6)
			targetPhrase->CreateCountInfo(tokens[5]);
		
		dbWrapper.SaveTarget(*targetPhrase);
		
		sourcePhrase.SaveTargetNonTerminals(*targetPhrase);
		

		if (prevSourcePhraseStr != sourcePhraseStr)
		{ // different source from last time. 
			prevSourcePhraseStr = sourcePhraseStr;
			Save(tpCollMap, dbWrapper);
		}
		else
		{ // same source as last time. do nothing
		}
		
		// new coll
		TargetPhraseCollection &tpColl = tpCollMap[sourcePhrase];
		tpColl.AddTargetPhrase(targetPhrase);
				
	} // while(getline(inStream, line))

	// save the last coll
	Save(tpCollMap, dbWrapper);
			
	dbWrapper.EndSave();
	
	Moses::PrintUserTime("Finished");
  return 0;
}


