#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
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
	dbWrapper.BeginSave(destPath, numSourceFactors, numTargetFactors, numScores);
	
	string line, prevSourcePhraseStr;
	size_t lineNum = 0;
	map<long, TargetPhraseCollection> tpCollMap;
		// long = sourceNode id
	
	while(getline(inStream, line))
	{
    if (++lineNum % 10000 == 0) 
			cerr << lineNum << " " << flush;

		line = Moses::Trim(line);
		if (line.size() == 0)
			continue;
		vector<string> tokens = Moses::TokenizeMultiCharSeparator( line , "|||" );
		
		// words
		const string &headWordsStr		= tokens[0]
								,&sourcePhraseStr	= tokens[1]
								,&targetPhraseStr	= tokens[2]
								,&alignStr				= tokens[3]
								,&scoresStr				= tokens[4];
						
		Phrase sourcePhrase;
		sourcePhrase.CreateFromString(sourcePhraseStr, dbWrapper.GetVocab());
		
		TargetPhrase *targetPhrase = new TargetPhrase();
		targetPhrase->CreateFromString(targetPhraseStr, dbWrapper.GetVocab());
		targetPhrase->CreateAlignFromString(alignStr);
		targetPhrase->CreateScoresFromString(scoresStr, numScores);
		targetPhrase->CreateHeadwordsFromString(headWordsStr, dbWrapper.GetVocab());

		long sourceNodeId = dbWrapper.SaveSource(sourcePhrase, *targetPhrase);
		dbWrapper.SaveTarget(*targetPhrase);

		if (prevSourcePhraseStr != sourcePhraseStr)
		{ // different source from last time. 
			prevSourcePhraseStr = sourcePhraseStr;
			
			//write out all target phrase colls
			map<long, TargetPhraseCollection>::const_iterator iter;
			for (iter = tpCollMap.begin(); iter != tpCollMap.end(); ++iter)
			{ // could be 1st. tpColl == NULL
				long sourceNodeIdColl = iter->first;
				const TargetPhraseCollection &tpColl = iter->second;
				dbWrapper.SaveTargetPhraseCollection(sourceNodeIdColl, tpColl);
			}

			// delete all tp coll
			tpCollMap.clear();			
		}
		else
		{ // same source as last time. do nothing
		}

		// new coll
		TargetPhraseCollection &tpColl = tpCollMap[sourceNodeId];
		tpColl.AddTargetPhrase(targetPhrase);
		
	}

	// save the last coll
	//write out all target phrase colls
	map<long, TargetPhraseCollection>::const_iterator iter;
	for (iter = tpCollMap.begin(); iter != tpCollMap.end(); ++iter)
	{ // could be 1st. tpColl == NULL
		long sourceNodeIdColl = iter->first;
		const TargetPhraseCollection &tpColl = iter->second;
		dbWrapper.SaveTargetPhraseCollection(sourceNodeIdColl, tpColl);
	}
	
	// delete all tp coll
	tpCollMap.clear();			
	
	dbWrapper.EndSave();
	
	Moses::PrintUserTime("Finished");
  return 0;
}


