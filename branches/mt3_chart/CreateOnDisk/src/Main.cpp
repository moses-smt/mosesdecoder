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
	
	assert(argc == 8);
	
	int numSourceFactors		= Moses::Scan<int>(argv[1])
			, numTargetFactors	= Moses::Scan<int>(argv[2])
			, numScores					= Moses::Scan<int>(argv[3])
			, doSort						= Moses::Scan<bool>(argv[4])
			, tableLimit				= Moses::Scan<int>(argv[5]);
	string filePathUnsorted = argv[6]
			,destPath = argv[7];
	
	string filePath = Sort(filePathUnsorted, doSort);

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

std::string Sort(const std::string &filePathInput, bool doSort)
{
	string filePathSorted = filePathInput + ".sorted";

	if (doSort)
	{
		Moses::InputFileStream inStream(filePathInput);

		string filePathNewFormat = filePathInput + ".new-format";
		ofstream newFormatFile;
		newFormatFile.open(filePathNewFormat.c_str(), ios::out | ios::ate | ios::trunc);
		assert(newFormatFile.is_open());

		size_t lineNum = 0;
		char line[100000];
		
		//while(getline(inStream, line))
		while(inStream.getline(line, 100000))
		{
			lineNum++;
			if (lineNum%1000 == 0) cerr << "." << flush;
			if (lineNum%10000 == 0) cerr << ":" << flush;
			if (lineNum%100000 == 0) cerr << lineNum << flush;

			vector<string> sourceToks, targetToks, alignToks, scoreToks, miscToks;
			::AlignType alignments;
			string sourceLHS, targetLHS;
			
			size_t stage = 0;
			/*	0 = source lhs
			 1 = target lhs
			 2 = SPACE
			 3 = source phrase
			 4 = target phrase
			 5 = align
			 6 = scores
			 7 = counts
			 */
			char *tok = strtok (line," ");
			while (tok != NULL)
			{
				//cerr << tok << " ";
				if (0 == strcmp(tok, "|||"))
				{
					++stage;
				}
				else
				{
					switch (stage)
					{
						case 0:
							sourceLHS = tok;
							++stage;
							break;
						case 1:
							targetLHS = tok;
							++stage;
							break;
						case 3:
							sourceToks.push_back(tok);
							break;
						case 4:
							targetToks.push_back(tok);
							break;
						case 5:
						{
							alignToks.push_back(tok);
							
							vector<size_t> alignPoints;
							Moses::Tokenize<size_t>(alignPoints, tok, "-");
							assert(alignPoints.size() == 2);
							alignments.push_back(::AlignPair(alignPoints[0], alignPoints[1]) );	
							break;
						}
						case 6:
							scoreToks.push_back(tok);
							break;
						case 7:
						{
							miscToks.push_back(tok);
							break;
						}
						default:
							assert(false);
							break;
					} // switch
				} // if
				
				tok = strtok (NULL, " ");
			} // while (tok != NULL)
			
			// finished tokenising line. re-arrange
			sourceToks.push_back(sourceLHS);
			targetToks.push_back(targetLHS);
			
			SortAlign(alignments);
			InsertTargetNonTerminals(sourceToks, targetToks, alignments);
			
			std::copy(sourceToks.begin(),sourceToks.end(),
								std::ostream_iterator<string>(newFormatFile," "));
			newFormatFile << " ||| ";

			std::copy(targetToks.begin(),targetToks.end(),
								std::ostream_iterator<string>(newFormatFile," "));
			newFormatFile << " ||| ";
			
			std::copy(alignToks.begin(),alignToks.end(),
								std::ostream_iterator<string>(newFormatFile," "));
			newFormatFile << " ||| ";

			std::copy(scoreToks.begin(),scoreToks.end(),
								std::ostream_iterator<string>(newFormatFile," "));
			newFormatFile << " ||| ";
			
			std::copy(miscToks.begin(),miscToks.end(),
								std::ostream_iterator<string>(newFormatFile," "));

			newFormatFile << endl;
			
			sourceToks.clear();
			targetToks.clear();
			alignToks.clear();
			scoreToks.clear();
			alignments.clear();
			

		} // while(inStream.getline(
		
		newFormatFile.close();
		Moses::PrintUserTime("Written on new file");
		
		string cmd = "export LC_ALL=C && sort -T . " + filePathNewFormat + " >" + filePathSorted ;
		system(cmd.c_str());

		Moses::DeleteFile(filePathNewFormat);
		
		Moses::PrintUserTime("Sorted");
	}
	
	return filePathSorted;
} // Sort()

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
