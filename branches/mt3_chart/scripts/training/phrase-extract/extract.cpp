// $Id$
// vim:tabstop=2

/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <cstdio>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <sstream>
#include "extract.h"

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

using namespace std;

int main(int argc, char* argv[]) 
{
  cerr << "Extract v2.0, written by Philipp Koehn\n"
       << "rule extraction from an aligned parallel corpus\n";
  time_t starttime = time(NULL);

	if (argc < 5) {
		cerr << "syntax: extract corpus.target corpus.source corpus.align extract "
		     << " [ --Hierarchical | --Orientation"
				 << " | --GlueGrammar FILE | --UnknownWordLabel FILE"
				 << " | --OnlyDirect"
		     << " | --MaxSpan[" << maxSpan << "]"
				 << " | --MinHoleTarget[" << minHoleTarget << "]"
				 << " | --MinHoleSource[" << minHoleSource << "]"
				 << " | --MinWords[" << minWords << "]"
				 << " | --MaxSymbolsTarget[" << maxSymbolsTarget << "]"
				 << " | --MaxSymbolsSource[" << maxSymbolsSource << "]"
				 << " | --MaxNonTerm[" << maxNonTerm << "]"
		     << " | --SourceSyntax | --TargetSyntax" 
		     << " | --AllowOnlyUnalignedWords | --DisallowNonTermConsecTarget |--NonTermConsecSource |  --NoNonTermFirstWord | --NoFractionalCounting ]\n";
		exit(1);
	}
  char* &fileNameT = argv[1];
  char* &fileNameS = argv[2];
  char* &fileNameA = argv[3];
	string fileNameGlueGrammar;
 	string fileNameUnknownWordLabel;
	string fileNameExtract = string(argv[4]);

	int optionInd = 5;

  for(int i=optionInd;i<argc;i++) {
		// maximum span length
		if (strcmp(argv[i],"--MaxSpan") == 0) {
			maxSpan = atoi(argv[++i]);
			if (maxSpan < 1) {
				cerr << "extract error: --maxSpan should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--MinHoleTarget") == 0) {
			minHoleTarget = atoi(argv[++i]);
			if (minHoleTarget < 1) {
				cerr << "extract error: --minHoleTarget should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--MinHoleSource") == 0) {
			minHoleSource = atoi(argv[++i]);
			if (minHoleSource < 1) {
				cerr << "extract error: --minHoleSource should be at least 1" << endl;
				exit(1);
			}
		}
		// maximum number of words in hierarchical phrase
		else if (strcmp(argv[i],"--MaxSymbolsTarget") == 0) {
			maxSymbolsTarget = atoi(argv[++i]);
			if (maxSymbolsTarget < 1) {
				cerr << "extract error: --MaxSymbolsTarget should be at least 1" << endl;
				exit(1);
			}
		}
		// maximum number of words in hierarchical phrase
		else if (strcmp(argv[i],"--MaxSymbolsSource") == 0) {
			maxSymbolsSource = atoi(argv[++i]);
			if (maxSymbolsSource < 1) {
				cerr << "extract error: --MaxSymbolsSource should be at least 1" << endl;
				exit(1);
			}
		}
		// minimum number of words in hierarchical phrase
		else if (strcmp(argv[i],"--MinWords") == 0) {
			minWords = atoi(argv[++i]);
			if (minWords < 0) {
				cerr << "extract error: --MinWords should be at least 0" << endl;
				exit(1);
			}
		}
		// maximum number of non-terminals
		else if (strcmp(argv[i],"--MaxNonTerm") == 0) {
			maxNonTerm = atoi(argv[++i]);
			if (maxNonTerm < 1) {
				cerr << "extract error: --MaxNonTerm should be at least 1" << endl;
				exit(1);
			}
		}		
		// allow consecutive non-terminals (X Y | X Y)
    else if (strcmp(argv[i],"--TargetSyntax") == 0) {
      targetSyntax = true;
    }
    else if (strcmp(argv[i],"--SourceSyntax") == 0) {
      sourceSyntax = true;
    }
    else if (strcmp(argv[i],"--AllowOnlyUnalignedWords") == 0) {
      requireAlignedWord = false;
    }
    else if (strcmp(argv[i],"--DisallowNonTermConsecTarget") == 0) {
      nonTermConsecTarget = false;
    }
    else if (strcmp(argv[i],"--NonTermConsecSource") == 0) {
      nonTermConsecSource = true;
    }
    else if (strcmp(argv[i],"--NoNonTermFirstWord") == 0) {
      nonTermFirstWord = false;
    }
    else if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
      onlyOutputSpanInfo = true;
    }
		// do not create many part00xx files!
    else if (strcmp(argv[i],"--NoFileLimit") == 0) {
      // now default
    }
		else if (strcmp(argv[i],"--OnlyDirect") == 0) {
      onlyDirectFlag = true;
    }
    else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
      orientationFlag = true;
    }
		else if (strcmp(argv[i],"--GlueGrammar") == 0) {
			glueGrammarFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --GlueGrammar requires a file name" << endl;
				exit(0);
			}
			fileNameGlueGrammar = string(argv[i]);
			cerr << "creating glue grammar in '" << fileNameGlueGrammar << "'" << endl;
    }
		else if (strcmp(argv[i],"--UnknownWordLabel") == 0) {
			unknownWordLabelFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --UnknownWordLabel requires a file name" << endl;
				exit(0);
			}
			fileNameUnknownWordLabel = string(argv[i]);
			cerr << "creating unknown word labels in '" << fileNameUnknownWordLabel << "'" << endl;
		}
    else if (strcmp(argv[i],"--Hierarchical") == 0) {
			cerr << "extracting hierarchical rules" << endl;
      hierarchicalFlag = true;
    }
		// TODO: this should be a useful option
    //else if (strcmp(argv[i],"--ZipFiles") == 0) {
    //  zipFiles = true;
    //}
		// if an source phrase is paired with two target phrases, then count(t|s) = 0.5
    else if (strcmp(argv[i],"--NoFractionalCounting") == 0) {
      fractionalCounting = false;
    }
    else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

	// open input files
  ifstream tFile;
  ifstream sFile;
  ifstream aFile;
  tFile.open(fileNameT);
  sFile.open(fileNameS);
  aFile.open(fileNameA);
  istream *tFileP = &tFile;
  istream *sFileP = &sFile;
  istream *aFileP = &aFile;

	// open output files
  string fileNameExtractInv = fileNameExtract + ".inv";
  string fileNameExtractOrientation = fileNameExtract + ".o";
  extractFile.open(fileNameExtract.c_str());
	if (!onlyDirectFlag)
		extractFileInv.open(fileNameExtractInv.c_str());
  if (orientationFlag)
    extractFileOrientation.open(fileNameExtractOrientation.c_str());
  
	// loop through all sentence pairs
  int i=0;
  while(true) {
    i++;
    if (i%1000 == 0) cerr << "." << flush;
    if (i%10000 == 0) cerr << ":" << flush;
    if (i%100000 == 0) cerr << "!" << flush;
    char targetString[LINE_MAX_LENGTH];
    char sourceString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*tFileP), targetString, LINE_MAX_LENGTH, '\n');
    if (tFileP->eof()) break;
    SAFE_GETLINE((*sFileP), sourceString, LINE_MAX_LENGTH, '\n');
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n');
    SentenceAlignment sentence;
    // cout << "read in: " << targetString << " & " << sourceString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << sourceString << endl;
      cout << "LOG: TGT: " << targetString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
      
    if (sentence.create( targetString, sourceString, alignmentString, i )) 
		{
			if (unknownWordLabelFlag)
			{
				collectWordLabelCounts(sentence);
			}
      extractRules(sentence);
			consolidateRules();
			writeRulesToFile();
			extractedRules.clear();
    }
    if (onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

  tFile.close();
  sFile.close();
  aFile.close();
  // only close if we actually opened it
  if (!onlyOutputSpanInfo) {
    extractFile.close();
		if (!onlyDirectFlag) extractFileInv.close();
    if (orientationFlag) extractFileOrientation.close();
  }

	if (glueGrammarFlag)
		writeGlueGrammar(fileNameGlueGrammar);

	if (unknownWordLabelFlag)
		writeUnknownWordLabel(fileNameUnknownWordLabel);
}
 
void extractRules( SentenceAlignment &sentence ) {
	int countT = sentence.target.size();
	int countS = sentence.source.size();

	// phrase repository for creating hiero phrases
	RuleExist ruleExist(countT);

	// check alignments for target phrase startT...endT
	for(int lengthT=1;
			lengthT <= maxSpan && lengthT <= countT;
			lengthT++) {
		for(int startT=0; startT < countT-(lengthT-1); startT++) {
			
			// that's nice to have
			int endT = startT + lengthT - 1;

			// if there is target side syntax, there has to be a node
			if (targetSyntax && !sentence.targetTree.HasNode(startT,endT))
				continue;
			
			// find find aligned source words
			// first: find minimum and maximum source word
			int minS = 9999;
			int maxS = -1;
			vector< int > usedS = sentence.alignedCountS;
			for(int ti=startT;ti<=endT;ti++) {
				for(int i=0;i<sentence.alignedToT[ti].size();i++) {
					int si = sentence.alignedToT[ti][i];
					// cerr << "point (" << si << ", " << ti << ")\n";
					if (si<minS) { minS = si; }
					if (si>maxS) { maxS = si; }
					usedS[ si ]--;
				}
			}

			// unaligned phrases are not allowed
			if( maxS == -1 )
				continue;

			// source phrase has to be within limits
			if( maxS-minS >= maxSpan )
				continue;
			
			// check if source words are aligned to out of bound target words
			bool out_of_bounds = false;
			for(int si=minS;si<=maxS && !out_of_bounds;si++)
				if (usedS[si]>0) {
					out_of_bounds = true;
				}
				
			// if out of bound, you gotta go
			if (out_of_bounds)
				continue;

			// done with all the checks, lets go over all consistent phrase pairs
			// start point of source phrase may retreat over unaligned
			for(int startS=minS;
					(startS>=0 &&
					 startS>maxS-maxSpan && // within length limit
					 (startS==minS || sentence.alignedCountS[startS]==0)); // unaligned
					startS--)
			{
				// end point of source phrase may advance over unaligned
				for(int endS=maxS;
						(endS<countS && endS<startS+maxSpan && // within length limit
						 (endS==maxS || sentence.alignedCountS[endS]==0)); // unaligned
						endS++) 
				{
					// if there is source side syntax, there has to be a node
					if (sourceSyntax && !sentence.sourceTree.HasNode(startS,endS))
						continue;

					// TODO: loop over all source and target syntax labels

					// if within length limits, add as fully-lexical phrase pair
					if (endT-startT < maxSymbolsTarget && endS-startS < maxSymbolsSource)
					{
						addRule(sentence,startT,endT,startS,endS, ruleExist);
					}

					// take note that this is a valid phrase alignment
					ruleExist.Add(startT, endT, startS, endS);

					// extract hierarchical rules
					if (hierarchicalFlag)
					{
						// are rules not allowed to start non-terminals?
						int initStartT = nonTermFirstWord ? startT : startT + 1;
					
						HoleCollection holeColl; // empty hole collection
						addHieroRule(sentence, startT, endT, startS, endS, 
												 ruleExist, holeColl, 0, initStartT, 
												 endT-startT+1, endS-startS+1);
					}
				}
			}
		}
	}
}

string printTargetHieroPhrase(SentenceAlignment &sentence
															, int startT, int endT, int startS, int endS 
															, WordIndex &indexT, HoleCollection &holeColl, LabelIndex &labelIndex)
{
	HoleList::iterator iterHoleList = holeColl.GetTargetHoles().begin();
	assert(iterHoleList != holeColl.GetTargetHoles().end());

	string out = "";
	int outPos = 0;
	int holeCount = 0;
  for(int currPos = startT; currPos <= endT; currPos++) 
	{
		bool isHole = false;
		if (iterHoleList != holeColl.GetTargetHoles().end())
		{
			const Hole &hole = *iterHoleList;
			isHole = hole.GetStart() == currPos;
		}

		if (isHole)
		{
			Hole &hole = *iterHoleList;
			int labelI = labelIndex[ 2+holeCount ];
			string label = targetSyntax ? 
				sentence.targetTree.GetNodes(currPos,hole.GetEnd())[ labelI ]->GetLabel() : "X";
			out += " [" + label + "]";

			currPos = hole.GetEnd();
			hole.SetPos(outPos);
			++iterHoleList;
			holeCount++;
		}
		else
		{
			indexT[currPos] = outPos;
			out += " " + sentence.target[currPos];
		}

		outPos++;
	}

	assert(iterHoleList == holeColl.GetTargetHoles().end());
	return out.substr(1);
}


string printSourceHieroPhrase( SentenceAlignment &sentence
	, int startT, int endT, int startS, int endS 
	, WordIndex &indexS, HoleCollection &holeColl, LabelIndex &labelIndex)
{
	vector<Hole*>::iterator iterHoleList = holeColl.GetSortedSourceHoles().begin();
	assert(iterHoleList != holeColl.GetSortedSourceHoles().end());

	string out = "";
	int outPos = 0;
	int holeCount = 0;
	int holeTotal = holeColl.GetTargetHoles().size();
  for(int currPos = startS; currPos <= endS; currPos++) 
	{
		bool isHole = false;
		if (iterHoleList != holeColl.GetSortedSourceHoles().end())
		{
			const Hole &hole = **iterHoleList;
			isHole = hole.GetStart() == currPos;
		}

		if (isHole)
		{
			Hole &hole = **iterHoleList;
			//HoleList::const_iterator iterSourceHoles;
			//int sequenceNumber;
			//int i=1;
			//for (iterSourceHoles = holeColl.GetSourceHoles().begin(); iterSourceHoles != holeColl.GetSourceHoles().end(); ++iterSourceHoles)
			//{
			//	if (iterSourceHoles->GetStart() == currPos)
			//	{ sequenceNumber = i; }
			//	++i;
			//}
			int labelI = labelIndex[ 2+holeCount+holeTotal ];
			string label = sourceSyntax ? 
				sentence.sourceTree.GetNodes(currPos,hole.GetEnd())[ labelI ]->GetLabel() : "X";
			out += " [" + label + "]";
			
			currPos = hole.GetEnd();
			hole.SetPos(outPos);		
			++iterHoleList;
			++holeCount;
		}
		else
		{
			indexS[currPos] = outPos;
			out += " " + sentence.source[currPos];
		}

		outPos++;
	}

	assert(iterHoleList == holeColl.GetSortedSourceHoles().end());
	return out.substr(1);
}

void printHieroAlignment(SentenceAlignment &sentence
												 , int startT, int endT, int startS, int endS 
												 , const WordIndex &indexS, const WordIndex &indexT, HoleCollection &holeColl, ExtractedRule &rule)
{
	// print alignment of words
	for(int ti=startT;ti<=endT;ti++) {
		if (indexT.find(ti) != indexT.end()) { // does word still exist?
			for(int i=0;i<sentence.alignedToT[ti].size();i++) {
				int si = sentence.alignedToT[ti][i];
				rule.alignment      += " " + IntToString(indexS.find(si)->second) + "-" + IntToString(indexT.find(ti)->second);
				if (!onlyDirectFlag)
					rule.alignmentInv += " " + IntToString(indexT.find(ti)->second) + "-" + IntToString(indexS.find(si)->second);
			}
		}
	}
  
	// print alignment of non terminals
	HoleList::const_iterator iterSource, iterTarget;
	iterTarget = holeColl.GetTargetHoles().begin();
	for (iterSource = holeColl.GetSourceHoles().begin(); iterSource != holeColl.GetSourceHoles().end(); ++iterSource)
	{
		rule.alignment      += " " + IntToString(iterSource->GetPos()) + "-" + IntToString(iterTarget->GetPos());
		if (!onlyDirectFlag)
			rule.alignmentInv += " " + IntToString(iterTarget->GetPos()) + "-" + IntToString(iterSource->GetPos());
		++iterTarget;
	}

	assert(iterTarget == holeColl.GetTargetHoles().end());
	rule.alignment = rule.alignment.substr(1);
	if (!onlyDirectFlag)
		rule.alignmentInv = rule.alignmentInv.substr(1);
}

void printHieroPhrase( SentenceAlignment &sentence, int startT, int endT, int startS, int endS 
											 , HoleCollection &holeColl, LabelIndex &labelIndex)
{
	phraseCount++;
	WordIndex indexS, indexT; // to keep track of word positions in rule

	ExtractedRule rule( startT, endT, startS, endS );

	// phrase labels
	string targetLabel = targetSyntax ? 
		sentence.targetTree.GetNodes(startT,endT)[ labelIndex[0] ]->GetLabel() : "X";
	//string sourceLabel = sourceSyntax ?
	//	sentence.sourceTree.GetNodes(startS,endS)[ labelIndex[1] ]->GetLabel() : "X";
	string sourceLabel = "X";

	// target
	rule.target = "[" + targetLabel + "] " + 
		printTargetHieroPhrase(sentence, startT, endT, startS, endS, indexT, holeColl, labelIndex);
	
	// source
	// holeColl.SortSourceHoles();
	rule.source = "[" + sourceLabel + "] " +
		printSourceHieroPhrase(sentence, startT, endT, startS, endS, indexS, holeColl, labelIndex);

	// alignment
	printHieroAlignment(sentence, startT, endT, startS, endS, indexS, indexT, holeColl, rule);

//	cerr << "\tLABEL RULE " << rule.source << " ||| " << rule.target << endl;

	addRuleToCollection( rule );
}

void printAllHieroPhrases( SentenceAlignment &sentence
													 , int startT, int endT, int startS, int endS 
													 , HoleCollection &holeColl)
{
	LabelIndex labelIndex,labelCount;

	// number of target head labels
	labelCount.push_back( targetSyntax ?
												sentence.targetTree.GetNodes(startT,endT).size() : 1 );
	labelIndex.push_back(0);

	// number of source head labels
	labelCount.push_back( sourceSyntax ?
												sentence.sourceTree.GetNodes(startS,endS).size() : 1 );
	labelIndex.push_back(0);

	// number of target hole labels
	for( HoleList::const_iterator hole = holeColl.GetTargetHoles().begin();
			 hole != holeColl.GetTargetHoles().end(); hole++ )
	{
		labelCount.push_back( targetSyntax ?
													sentence.targetTree.GetNodes(hole->GetStart(),hole->GetEnd()).size() : 1 );
		labelIndex.push_back(0);
	}

	// number of source hole labels
	holeColl.SortSourceHoles();
	for( vector<Hole*>::iterator i = holeColl.GetSortedSourceHoles().begin();
			 i != holeColl.GetSortedSourceHoles().end(); i++ )
	{
		const Hole &hole = **i;
		labelCount.push_back( sourceSyntax ?
													sentence.sourceTree.GetNodes(hole.GetStart(),hole.GetEnd()).size() : 1 );
		labelIndex.push_back(0);
	}
//	cerr << "LABEL COUNT: ";
//	for(int i=0;i<labelCount.size();i++) 
//	{
//		cerr << " " << labelCount[i];
//	}
//	cerr << endl;

	// loop through the holes
	bool done = false;
	while(!done)
	{
//		cerr << "\tLABEL INDEX: ";
//		for(int i=0;i<labelIndex.size();i++) 
//		{
//			cerr << " " << labelIndex[i];
//		}
//		cerr << endl;
		printHieroPhrase( sentence, startT, endT, startS, endS, holeColl, labelIndex );
		for(int i=0; i<labelIndex.size(); i++)
		{
			labelIndex[i]++;
			if(labelIndex[i] == labelCount[i])
			{
				labelIndex[i] = 0;
				if (i == labelIndex.size()-1)
					done = true;			
			}
			else
			{
				break;
			}
		}
	}
}


// this function is called recursively
// it pokes a new hole into the phrase pair, and then calls itself for more holes
void addHieroRule( SentenceAlignment &sentence
									 , int startT, int endT, int startS, int endS 
									 , RuleExist &ruleExist, const HoleCollection &holeColl
									 , int numHoles, int initStartT, int wordCountT, int wordCountS)
{
	// cerr << "phrase " << startT << "-" << endT << "=>" << startS << "-" << endS << " (" << numHoles << ")" << endl;
	// done, if already the maximum number of non-terminals in phrase pair
	if (numHoles >= maxNonTerm)
		return;

	// find a hole...
	for (int startHoleT = initStartT; startHoleT <= endT; ++startHoleT)
	{
		//cerr << "phrase " << startT << "-" << endT << "=>" << startS << "-" << endS << " (" << numHoles << ")" << endl;
		for (int endHoleT = startHoleT+(minHoleTarget-1); endHoleT <= endT; ++endHoleT)
		{
			//cerr << "considering from hole " << startHoleT << "-" << endHoleT << " from phrase " << startT << "-" << endT << "=>" << startS << "-" << endS << " (" << numHoles << ")" << endl;
			// if last non-terminal, enforce word count limit
			if (numHoles == maxNonTerm-1 && wordCountT - (endHoleT-startT+1) + (numHoles+1) > maxSymbolsTarget)
				continue;

			// always enforce min word count limit
			if (wordCountT - (endHoleT-startHoleT+1) < minWords)
				continue;

			// except the whole span
			if (startHoleT == startT && endHoleT == endT)
				continue;

			// does a phrase cover this target span? 
			// if it does, then there should be a list of mapped source phrases
			// (multiple possible due to unaligned words)
			const HoleList &sourceHoles = ruleExist.GetSourceHoles(startHoleT, endHoleT);

			// loop over sub phrase pairs
			HoleList::const_iterator iterSourceHoles;
			for (iterSourceHoles = sourceHoles.begin(); iterSourceHoles != sourceHoles.end(); ++iterSourceHoles)
			{
				const Hole &sourceHole = *iterSourceHoles;

				// cerr << "source" << sourceHole.GetStart() << "-" << sourceHole.GetEnd() << endl; 

				// enforce minimum hole size
				if (sourceHole.GetEnd()-sourceHole.GetStart()+1 < minHoleSource)
					continue;

				// if last non-terminal, enforce word count limit
				if (numHoles == maxNonTerm-1 && 
						wordCountS - (sourceHole.GetEnd()-sourceHole.GetStart()+1) + (numHoles+1) > maxSymbolsSource)
					continue;

				// enforce min word count limit
				if (wordCountS - (sourceHole.GetEnd()-sourceHole.GetStart()+1) < minWords)
					continue;
				
				// hole must be subphrase of the source phrase
				// (may be violated if subphrase contains additional unaligned source word)
				if (startS > sourceHole.GetStart() || endS <  sourceHole.GetEnd())
					continue;

				// make sure target side does not overlap with another hole
				if (holeColl.OverlapSource(sourceHole))
					continue;

				// if consecutive non-terminals are not allowed, also check for source
				if (!nonTermConsecSource && holeColl.ConsecSource(sourceHole) )
					continue;

				// require that at least one aligned word is left
				if (requireAlignedWord)
				{
					HoleList::const_iterator iterHoleList = holeColl.GetTargetHoles().begin();
					bool foundAlignedWord = false;
					// loop through all word positions
					for(int pos = startT; pos <= endT && !foundAlignedWord; pos++) 
					{
						// new hole? moving on...
						if (pos == startHoleT)
						{
							pos = endHoleT;
						}
						// covered by hole? moving on...
						else if (iterHoleList != holeColl.GetTargetHoles().end() && iterHoleList->GetStart() == pos)
						{
							pos = iterHoleList->GetEnd();
							++iterHoleList;
						}
						// covered by word? check if it is aligned
						else
						{
							if (sentence.alignedToT[pos].size() > 0)
							  foundAlignedWord = true;
						}
					}
					if (!foundAlignedWord)
						continue;
				}

				// update list of holes in this phrase pair
				HoleCollection copyHoleColl(holeColl);
				copyHoleColl.Add(startHoleT, endHoleT, sourceHole.GetStart(), sourceHole.GetEnd());

				// now some checks that disallow this phrase pair, but not further recursion
				bool allowablePhrase = true;

				// maximum words count violation?
				if (wordCountS - (sourceHole.GetEnd()-sourceHole.GetStart()+1) + (numHoles+1) > maxSymbolsSource)
					allowablePhrase = false;

				if (wordCountT - (endHoleT-startHoleT+1) + (numHoles+1) > maxSymbolsTarget)
					allowablePhrase = false;
				
				// passed all checks...
				// cerr << "adding hole " << startHoleT << "," << endHoleT << "," << sourceHole.GetStart() << "," << sourceHole.GetEnd() << endl;
				if (allowablePhrase)
					printAllHieroPhrases(sentence, startT, endT, startS, endS, copyHoleColl);

				// recursively search for next hole
				int nextInitStartT = nonTermConsecTarget ? endHoleT + 1 : endHoleT + 2;
				addHieroRule(sentence, startT, endT, startS, endS 
										 , ruleExist, copyHoleColl, numHoles + 1, nextInitStartT
										 , wordCountT - (endHoleT-startHoleT+1)
										 , wordCountS - (sourceHole.GetEnd()-sourceHole.GetStart()+1));
			}
		}
	}
}

void addRule( SentenceAlignment &sentence, int startT, int endT, int startS, int endS 
							, RuleExist &ruleExist) 
{
  // source
  //cerr << "adding ( " << startS << "-" << endS << ", " << startT << "-" << endT << ")\n"; 

  if (onlyOutputSpanInfo) {
    cout << startS << " " << endS << " " << startT << " " << endT << endl;
    return;
  }
  phraseCount++;

	ExtractedRule rule(startT, endT, startS, endS);
	
	// phrase labels
	string targetLabel,sourceLabel;
	if (hierarchicalFlag) {
	  //sourceLabel = sourceSyntax ? 
	  //	sentence.sourceTree.GetNodes(startS,endS)[0]->GetLabel() : "X";
	        sourceLabel = "X";
		targetLabel = targetSyntax ?
			sentence.targetTree.GetNodes(startT,endT)[0]->GetLabel() : "X";
	}

	// source
	if (hierarchicalFlag)
		rule.source = " [" + sourceLabel + "]";
  for(int si=startS;si<=endS;si++)
		rule.source += " " + sentence.source[si];
	rule.source = rule.source.substr(1);

  // target
	if (hierarchicalFlag)
		rule.target = " [" + targetLabel + "]";
  for(int ti=startT;ti<=endT;ti++) 
		rule.target += " " + sentence.target[ti];
	rule.target = rule.target.substr(1);

  // alignment
  for(int ti=startT;ti<=endT;ti++) 
	{
    for(int i=0;i<sentence.alignedToT[ti].size();i++) 
		{
      int si = sentence.alignedToT[ti][i];
			rule.alignment += " " + IntToString(si-startS) + "-" + IntToString(ti-startT);
			if (!onlyDirectFlag)
				rule.alignmentInv += " " + IntToString(ti-startT) + "-" + IntToString(si-startS);
    }
	}
	rule.alignment = rule.alignment.substr(1);
	if (!onlyDirectFlag)
		rule.alignmentInv = rule.alignmentInv.substr(1);

	// orientation
  if (orientationFlag) {
    // orientation to previous E
    bool connectedLeftTop  = isAligned( sentence, startS-1, startT-1 );
    bool connectedRightTop = isAligned( sentence, endS+1,   startT-1 );
    if      ( connectedLeftTop && !connectedRightTop) 
			rule.orientation = "mono";
    else if (!connectedLeftTop &&  connectedRightTop) 
			rule.orientation = "swap";
    else 
			rule.orientation = "other";
  
    // orientation to following E
    bool connectedLeftBottom  = isAligned( sentence, startS-1, endT+1 );
    bool connectedRightBottom = isAligned( sentence, endS+1,   endT+1 );
    if      ( connectedLeftBottom && !connectedRightBottom) 
			rule.orientationForward = "swap";
    else if (!connectedLeftBottom &&  connectedRightBottom) 
			rule.orientationForward = "mono";
    else 
			rule.orientationForward = "other";
  }
	addRuleToCollection( rule );
}

bool isAligned ( SentenceAlignment &sentence, int si, int ti ) {
  if (ti == -1 && si == -1) return true;
  if (ti <= -1 || si <= -1) return false;
  if (ti == sentence.target.size() && si == sentence.source.size()) return true;
  if (ti >= sentence.target.size() || si >= sentence.source.size()) return false;
  for(int i=0;i<sentence.alignedToT[ti].size();i++) 
    if (sentence.alignedToT[ti][i] == si) return true;
  return false;
}

int SentenceAlignment::create( char targetString[], char sourceString[], char alignmentString[], int sentenceID ) {
  // tokenizing English (and potentially extract syntax spans)
  if (targetSyntax) {
      string targetStringCPP = string(targetString);
      ProcessAndStripXMLTags( targetStringCPP, targetTree, targetLabelCollection , targetTopLabelCollection );
      target = tokenize( targetStringCPP.c_str() );
      // cerr << "E: " << targetStringCPP << endl;
  }
  else {
      target = tokenize( targetString );
  }

  // tokenizing source (and potentially extract syntax spans)
  if (sourceSyntax) {
      string sourceStringCPP = string(sourceString);
      ProcessAndStripXMLTags( sourceStringCPP, sourceTree, sourceLabelCollection , sourceTopLabelCollection );
      source = tokenize( sourceStringCPP.c_str() );
      // cerr << "F: " << sourceStringCPP << endl;
  }
  else {
      source = tokenize( sourceString );
  }

  // check if sentences are empty
  if (target.size() == 0 || source.size() == 0) {
    cerr << "no target (" << target.size() << ") or source (" << source.size() << ") words << end insentence " << sentenceID << endl;
    cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
    return 0;
  }

  // prepare data structures for alignments
  for(int i=0; i<source.size(); i++) {
    alignedCountS.push_back( 0 );
  }
  for(int i=0; i<target.size(); i++) {
    vector< int > dummy;
    alignedToT.push_back( dummy );
  }

  // reading in alignments
  vector<string> alignmentSequence = tokenize( alignmentString );
  for(int i=0; i<alignmentSequence.size(); i++) {
    int s,t;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &s, &t)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentence " << sentenceID << endl; 
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return 0;
    }
      // cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << s << ", " << t << endl;
    if (t >= target.size() || s >= source.size()) { 
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << s << ", " << t << ") out of bounds (" << source.size() << ", " << target.size() << ")\n";
      cerr << "T: " << targetString << endl << "S: " << sourceString << endl;
      return 0;
    }
    alignedToT[t].push_back( s );
    alignedCountS[s]++;
  }
  return 1;
}

void addRuleToCollection( ExtractedRule &newRule ) {
	//cerr << "add ( " << newRule.source << " , " << newRule.target << " )\n";
	 
	// no double-counting of identical rules from overlapping spans
	if (!duplicateRules)
	{
		vector<ExtractedRule>::const_iterator rule;
		for(rule = extractedRules.begin(); rule != extractedRules.end(); rule++ )
		{
			if (rule->source.compare( newRule.source ) == 0 &&
					rule->target.compare( newRule.target ) == 0 &&
					!(rule->endT < newRule.startT || rule->startT > newRule.endT)) // overlapping
			{
				return;
			}
		}
	}
	extractedRules.push_back( newRule );
}

void consolidateRules() 
{
	typedef vector<ExtractedRule>::iterator R;
	map<int, map<int, map<int, map<int,int> > > > spanCount;

	// compute number of rules per span
	if (fractionalCounting) 
	{
		for(R rule = extractedRules.begin(); rule != extractedRules.end(); rule++ )
		{
			spanCount[ rule->startT ][ rule->endT ][ rule->startS ][ rule->endS ]++;
		}
	}

	// compute fractional counts
	for(R rule = extractedRules.begin(); rule != extractedRules.end(); rule++ )
	{
		rule->count =    1.0/(float) (fractionalCounting ? spanCount[ rule->startT ][ rule->endT ][ rule->startS ][ rule->endS ] : 1.0 );
	}

	// consolidate counts
	for(R rule = extractedRules.begin(); rule != extractedRules.end(); rule++ )
	{
		if (rule->count == 0)
			continue;
		for(R r2 = rule+1; r2 != extractedRules.end(); r2++ )
		{
			if (rule->source.compare( r2->source ) == 0 &&
					rule->target.compare( r2->target ) == 0 &&
					rule->alignment.compare( r2->alignment ) == 0)
			{
				rule->count += r2->count;
				r2->count = 0;
			}
		}
	}
}

void writeRulesToFile() {
	vector<ExtractedRule>::const_iterator rule;
	for(rule = extractedRules.begin(); rule != extractedRules.end(); rule++ )
	{
		if (rule->count == 0)
			continue;

		extractFile << rule->source << " ||| " 
								<< rule->target << " ||| " 
								<< rule->alignment << " ||| " 
								<< rule->count << "\n";

		if (!onlyDirectFlag)
			extractFileInv << rule->target << " ||| " 
										 << rule->source << " ||| " 
										 << rule->alignmentInv << " ||| " 
										 << rule->count << "\n";
		
		if (orientationFlag)
			extractFileOrientation << rule->source << " ||| " 
														 << rule->target << " ||| " 
														 << rule->orientation << " " << rule->orientationForward << "\n";
	}
}

void writeGlueGrammar( string fileName )
{
	ofstream grammarFile;
	grammarFile.open(fileName.c_str());
	if (!targetSyntax)
	{
		grammarFile << "[X] [S] ||| <s> ||| <s> |||  ||| 1" << endl
		            << "[X] [S] ||| [X] </s> ||| [S] </s> ||| 0-0 ||| 1" << endl
		            << "[X] [S] ||| [X] [X] ||| [S] [X] ||| 0-0 1-1 ||| 2.718" << endl;
	}
	else
	{
		// chose a top label that is not already a label
		string topLabel = "QQQQQQ";
		for( int i=1;i<=topLabel.length(); i++)
		{
			if(targetLabelCollection.find( topLabel.substr(0,i) ) == targetLabelCollection.end() )
			{
				topLabel = topLabel.substr(0,i);
				break;
			}
		}
		// basic rules
		grammarFile << "[X] [" << topLabel << "] ||| <s> ||| <s> |||  ||| 1" << endl
								<< "[X] [" << topLabel << "] ||| [X] </s> ||| [" << topLabel << "] </s> ||| 0-0 ||| 1" << endl;

		// top rules
		for( map<string,int>::const_iterator i =  targetTopLabelCollection.begin();
				 i !=  targetTopLabelCollection.end(); i++ )
		{
			grammarFile << "[X] [" << topLabel << "] ||| <s> [X] </s> ||| <s> [" << i->first << "] </s> ||| 1-1 ||| 1" << endl;
		}

		// glue rules
		for( set<string>::const_iterator i =  targetLabelCollection.begin();
				 i !=  targetLabelCollection.end(); i++ )
		{
			grammarFile << "[X] [" << topLabel << "] ||| [X] [X] ||| [" << topLabel << "] [" << *i << "] ||| 0-0 1-1 ||| 2.718" << endl;
		}
		grammarFile << "[X] [" << topLabel << "] ||| [X] [X] ||| [" << topLabel << "] [X] ||| 0-0 1-1 ||| 2.718" << endl; // glue rule for unknown word... 
	}
	grammarFile.close();
}

// collect counts for labels for each word
// ( labels of singleton words are used to estimate
//   distribution oflabels for unknown words )

map<string,int> wordCount;
map<string,string> wordLabel;
void collectWordLabelCounts( SentenceAlignment &sentence ) 
{
	int countT = sentence.target.size();
	for(int ti=0; ti < countT; ti++) 
	{
		string &word = sentence.target[ ti ];
		const vector< SyntaxNode* >& labels = sentence.targetTree.GetNodes(ti,ti);
		if (labels.size() > 0)
		{
			wordCount[ word ]++;
			wordLabel[ word ] = labels[0]->GetLabel();
		}
	}
}

void writeUnknownWordLabel( string fileName )
{
	ofstream outFile;
	outFile.open(fileName.c_str());
	typedef map<string,int>::const_iterator I;

	map<string,int> count;
	int total = 0;
	for(I word = wordCount.begin(); word != wordCount.end(); word++)
	{
		// only consider singletons
		if (word->second == 1)
		{
		  count[ wordLabel[ word->first ] ]++;
			total++;
		}
	}

	for(I pos = count.begin(); pos != count.end(); pos++)
	{
		double ratio = ((double) pos->second / (double) total);
		if (ratio > 0.03)
			outFile << pos->first << " " << ratio << endl;
	}

	outFile.close();
}
