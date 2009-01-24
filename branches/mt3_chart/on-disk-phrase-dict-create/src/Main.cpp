// processOnDiskPhraseTable.cpp : Defines the entry point for the console application.
//

#include "../../on-disk-phrase-dict/src/TypeDef.h"
#include "../../on-disk-phrase-dict/src/Phrase.h"
#include "../../on-disk-phrase-dict/src/Vocab.h"
#include "../../on-disk-phrase-dict/src/TargetPhraseCollection.h"
#include "../../on-disk-phrase-dict/src/SourcePhraseCollection.h"
#include "../../moses/src/InputFileStream.h"
#include "../../moses/src/Util.h"

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

using namespace std;
using namespace MosesOnDiskPt;

// moses formatted pt
void CreateAlign(vector< pair<size_t, size_t> > &ret
								,const string &alignSource
								,const string &alignTarget);

// hiero formatted pt
void CreateAlign(vector< pair<size_t, size_t> > &ret
								,const map<size_t, size_t> &alignSource
								,const map<size_t, size_t> &alignTarget);

int main(int argc, char* argv[])
{
	string txtPath(argv[1]);
	size_t numSourceFactors = Moses::Scan<size_t>(string(argv[2]));
  size_t numTargetFactors = Moses::Scan<size_t>(string(argv[3]));

  // save to file
	std::string databaseHome(argv[4]);

	PhraseTableFormat phraseTableFormat = (PhraseTableFormat) Moses::Scan<size_t>(string(argv[5]));
	Vocab vocab;
	TargetPhraseCollection targetPhraseCollection(numTargetFactors);
	SourcePhraseCollection sourcePhraseCollection(numSourceFactors);

	// data from file
	Moses::InputFileStream inFile(txtPath);

	cerr << "Loading" << endl;

	string line;
	size_t lineNum = 0;
	size_t numScores = NOT_FOUND;

	while(getline(inFile, line))
	{
		//if (lineNum % 5000 == 0)
		//	cerr << lineNum << " ";
		//cerr << line << endl;

		line = Moses::Trim(line);
		if (line.size() == 0)
			continue;
		vector<string> tokens = Moses::TokenizeMultiCharSeparator( line , "|||" );

		// words
		size_t ind	= (phraseTableFormat == PhraseTableFormat_Moses) ? 0 : 1;
		const string &sourcePhraseStr	= tokens[ind]
								,&targetPhraseStr	= tokens[ind+1];

		// scores
		ind	= (phraseTableFormat == PhraseTableFormat_Moses) ? 4 : 3;
		const string &scoresStr				= tokens[ind];

		// alignment. (implicit in hiero format, explicit in moses format)
		vector< pair<size_t, size_t> > align;

		Phrase *sourcePhrase, *targetPhrase;

		if (phraseTableFormat == PhraseTableFormat_Hiero)
		{
				// source, target
			map<size_t, size_t> alignSource, alignTarget;
			sourcePhrase	= new Phrase(sourcePhraseStr, numSourceFactors, vocab, alignSource);
			targetPhrase	= new Phrase(targetPhraseStr, numTargetFactors, vocab, alignTarget);

			// create align info
			assert(alignSource.size() == alignTarget.size());
			CreateAlign(align, alignSource, alignTarget);
		}
		else if (phraseTableFormat == PhraseTableFormat_Moses)
		{
			sourcePhrase	= new Phrase(sourcePhraseStr, numSourceFactors, vocab);
			targetPhrase	= new Phrase(targetPhraseStr, numTargetFactors, vocab);

			const string &alignSource = tokens[2]
									,&alignTarget = tokens[3];

			CreateAlign(align, alignSource, alignTarget);
		}
		else
		{
			abort();
		}

		SourcePhraseNode &sourceNode = sourcePhraseCollection.Add(*sourcePhrase);
		const Phrase &targetPhraseRef = targetPhraseCollection.Add(*targetPhrase);

		vector<float> scores = Moses::Tokenize<float>(scoresStr);
		if (numScores == NOT_FOUND)
			numScores = scores.size();
		assert(scores.size() == numScores);

		sourceNode.AddTarget(targetPhraseRef, scores, align);

		delete sourcePhrase;
		delete targetPhrase;

		lineNum++;
	}

	cerr << "Saving" << endl;

	vocab.Save(databaseHome + "/vocab.db");
	targetPhraseCollection.Save(databaseHome + "/target.db");
	sourcePhraseCollection.Save(databaseHome + "/source.db", numScores);

	return 0;
}

void CreateAlign(vector< pair<size_t, size_t> > &ret
								,const string &alignSource
								,const string &alignTarget)
{	// only use alignSource

	vector<string> wordsAlignVec = Moses::Tokenize(alignSource);
	for (size_t sourcePos = 0; sourcePos < wordsAlignVec.size(); ++sourcePos)
	{
		const string &wordsAlign = wordsAlignVec[sourcePos];
		string stripped = wordsAlign.substr(1, wordsAlign.size()-2);
		vector<size_t> targetPosVec = Moses::Tokenize<size_t>(stripped, ",");
		for (size_t ind = 0; ind < targetPosVec.size(); ++ind)
		{
			size_t targetPos = targetPosVec[ind];
			ret.push_back( pair<size_t, size_t>(sourcePos, targetPos));
		}
	}
}

void CreateAlign(vector< pair<size_t, size_t> > &ret
								,const map<size_t, size_t> &alignSource
								,const map<size_t, size_t> &alignTarget)
{
	map<size_t, size_t>::const_iterator iter;
	for (iter = alignSource.begin(); iter != alignSource.end(); ++iter)
	{
		size_t ind = iter->first;
		size_t sourcePos = iter->second;

		map<size_t, size_t>::const_iterator iterTarget;
		iterTarget = alignTarget.find(ind);
		assert(iterTarget != alignTarget.end());

		size_t targetPos = iterTarget->second;

		ret.push_back( pair<size_t, size_t>(sourcePos, targetPos));
	}
}
