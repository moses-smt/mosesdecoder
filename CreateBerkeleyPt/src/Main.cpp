#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

#include <iostream>
#include <string>
#include "../../BerkeleyPt/src/TargetPhraseCollection.h"
#include "../../BerkeleyPt/src/Phrase.h"
#include "../../BerkeleyPt/src/TargetPhrase.h"
#include "../../BerkeleyPt/src/Vocab.h"
#include "../../BerkeleyPt/src/DbWrapper.h"
#include "../../moses/src/InputFileStream.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/UserMessage.h"

using namespace std;
using namespace MosesBerkeleyPt;

int main (int argc, char * const argv[]) {
    // insert code here...
  std::cerr << "Starting\n";
	
	string filePath = "pt.txt";
	
	Moses::InputFileStream inStream(filePath);

	DbWrapper dbWrapper;
	dbWrapper.BeginSave(".", 1, 1, 5);
	
	size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info

	string line;
	size_t lineNum = 0;
	size_t numScores = NOT_FOUND;
	long sourceNodeIdOld = 0;
	TargetPhraseCollection *tpColl = NULL;

	while(getline(inStream, line))
	{
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
		targetPhrase->CreateScoresFromString(scoresStr);
		targetPhrase->CreateHeadwordsFromString(headWordsStr, dbWrapper.GetVocab());

		long sourceNodeId = dbWrapper.SaveSource(sourcePhrase, *targetPhrase);
		dbWrapper.SaveTarget(*targetPhrase);

		if (sourceNodeIdOld != sourceNodeId)
		{ // different source from last time. write out target phrase coll
			if (tpColl)
			{ // could be 1st. tpColl == NULL
				dbWrapper.SaveTargetPhraseCollection(sourceNodeIdOld, *tpColl);
			}

			// new coll
			delete tpColl;
			tpColl = new TargetPhraseCollection();
			tpColl->AddTargetPhrase(targetPhrase);

			sourceNodeIdOld = sourceNodeId;
		}
		else
		{ // same source as last time. do nothing
			tpColl->AddTargetPhrase(targetPhrase);
		}

	}
	
	delete tpColl;

	dbWrapper.EndSave();
	
	std::cerr << "Finished\n";
  return 0;
}


