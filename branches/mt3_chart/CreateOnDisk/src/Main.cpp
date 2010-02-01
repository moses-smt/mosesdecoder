#undef _GLIBCXX_DEBUG
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include "../../moses/src/InputFileStream.h"
#include "../../moses/src/Util.h"
#include "../../moses/src/UserMessage.h"
#include "../../OnDiskPt/src/OnDiskWrapper.h"
#include "../../OnDiskPt/src/SourcePhrase.h"
#include "../../OnDiskPt/src/TargetPhrase.h"
#include "../../OnDiskPt/src/TargetPhraseCollection.h"
#include "../../OnDiskPt/src/Word.h"
#include "../../OnDiskPt/src/Vocab.h"
#include "Main.h"

using namespace std;
using namespace OnDiskPt;

int main (int argc, char * const argv[])
{
	// insert code here...
	Moses::ResetUserTime();
	Moses::PrintUserTime("Starting");
	
	assert(argc == 7);
	
	int numSourceFactors		= Moses::Scan<int>(argv[1])
			, numTargetFactors	= Moses::Scan<int>(argv[2])
			, numScores					= Moses::Scan<int>(argv[3])
			, tableLimit				= Moses::Scan<int>(argv[4]);
	string filePath = argv[5]
				,destPath = argv[6];
	
	Moses::InputFileStream inStream(filePath);
	
	OnDiskWrapper onDiskWrapper;
	bool retDb = onDiskWrapper.BeginSave(destPath, numSourceFactors, numTargetFactors, numScores);
	assert(retDb);
	
	PhraseNode &rootNode = onDiskWrapper.GetRootSourceNode();
	size_t lineNum = 0;
	char line[100000];

	//while(getline(inStream, line))
	while(inStream.getline(line, 100000))
	{
		lineNum++;
    if (lineNum%1000 == 0) cerr << "." << flush;
    if (lineNum%10000 == 0) cerr << ":" << flush;
    if (lineNum%100000 == 0) cerr << lineNum << flush;
		//cerr << lineNum << " " << line << endl;
		
		std::vector<float> misc;
		string sourceStr;
		SourcePhrase sourcePhrase;
		TargetPhrase *targetPhrase = new TargetPhrase(numScores);
		Tokenize(sourcePhrase, *targetPhrase, line, onDiskWrapper, sourceStr, numScores, misc);
		assert(misc.size() == onDiskWrapper.GetNumCounts());
		
		rootNode.AddTargetPhrase(sourcePhrase, targetPhrase, onDiskWrapper, tableLimit, misc);	
	}
	
	rootNode.Save(onDiskWrapper, 0, tableLimit);
	onDiskWrapper.EndSave();
	Moses::DeleteFile(filePath);

	Moses::PrintUserTime("Finished");
  
	//pause();
	return 0;	
	
} // main()

bool Flush(const OnDiskPt::SourcePhrase *prevSourcePhrase, const OnDiskPt::SourcePhrase *currSourcePhrase)
{
	if (prevSourcePhrase == NULL)
		return false;
	
	assert(currSourcePhrase);
	bool ret = (*currSourcePhrase > *prevSourcePhrase);
	//cerr << *prevSourcePhrase << endl << *currSourcePhrase << " " << ret << endl << endl;

	return ret;
}

void Tokenize(SourcePhrase &sourcePhrase, TargetPhrase &targetPhrase, char *line, OnDiskWrapper &onDiskWrapper, string &sourceStr, int numScores, vector<float> &misc)
{
	size_t scoreInd = 0;
	
	// MAIN LOOP
	size_t stage = 0;
	/*	0 = source phrase
	 1 = target phrase
	 2 = align
	 3 = scores
	 */
	char *tok = strtok (line," ");
	while (tok != NULL)
	{
		if (0 == strcmp(tok, "|||"))
		{
			++stage;
		}
		else
		{
			switch (stage)
			{
				case 0:
				{
					sourceStr += string(tok) + " ";

					Word *word = new Word();
					word->CreateFromString(tok, onDiskWrapper.GetVocab());
					sourcePhrase.AddWord(word);
					break;
				}
				case 1:
				{
					Word *word = new Word();
					word->CreateFromString(tok, onDiskWrapper.GetVocab());
					targetPhrase.AddWord(word);
					break;
				}
				case 2:
				{
					targetPhrase.Create1AlignFromString(tok);
					break;
				}
				case 3:
				{
					float score = Moses::Scan<float>(tok);
					targetPhrase.SetScore(score, scoreInd);
					++scoreInd;
					break;
				}
				case 4:
				{ // count info. Only store the 1st one
					if (misc.size() == 0)
					{
						float val = Moses::Scan<float>(tok);
						misc.push_back(val);
					}
					break;
				}
				default:
					assert(false);
					break;
			}
		}
		
		tok = strtok (NULL, " ");
	} // while (tok != NULL)
	
	assert(scoreInd == numScores);
	targetPhrase.SortAlign();
	
} // Tokenize()

void InsertTargetNonTerminals(std::vector<std::string> &sourceToks, const std::vector<std::string> &targetToks, const ::AlignType &alignments)
{
	for (int ind = alignments.size() - 1; ind >= 0; --ind)
	{
		const ::AlignPair &alignPair = alignments[ind];
		size_t sourcePos = alignPair.first
					,targetPos = alignPair.second;
		
		const string &target = targetToks[targetPos];
		sourceToks.insert(sourceToks.begin() + sourcePos + 1, target);
		
	}
}

class AlignOrderer
	{
	public:	
		bool operator()(const ::AlignPair &a, const ::AlignPair &b) const
		{
			return a.first < b.first;
		}
	};

void SortAlign(::AlignType &alignments)
{
	std::sort(alignments.begin(), alignments.end(), AlignOrderer());
}
