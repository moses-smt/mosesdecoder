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

#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <set>

#include "SafeGetline.h"
#include "tables-core.h"
#include "PhraseAlignment.h"
#include "score.h"

using namespace std;

#define LINE_MAX_LENGTH 100000

Vocabulary vcbT;
Vocabulary vcbS;

class LexicalTable 
{
public:
  map< WORD_ID, map< WORD_ID, double > > ltable;
  void load( char[] );
  double permissiveLookup( WORD_ID wordS, WORD_ID wordT ) 
	{
		// cout << endl << vcbS.getWord( wordS ) << "-" << vcbT.getWord( wordT ) << ":";
		if (ltable.find( wordS ) == ltable.end()) return 1.0;
		if (ltable[ wordS ].find( wordT ) == ltable[ wordS ].end()) return 1.0;
		// cout << ltable[ wordS ][ wordT ];
		return ltable[ wordS ][ wordT ];
  }
};

vector<string> tokenize( const char [] );

void computeCountOfCounts( char* fileNameExtract, int maxLines );
void processPhrasePairs( vector< PhraseAlignment > & );
PhraseAlignment* findBestAlignment( vector< PhraseAlignment* > & );
void outputPhrasePair( vector< PhraseAlignment * > &, float );
double computeLexicalTranslation( PHRASE &, PHRASE &, PhraseAlignment * );

ofstream phraseTableFile;

LexicalTable lexTable;
PhraseTable phraseTableT;
PhraseTable phraseTableS;
bool inverseFlag = false;
bool hierarchicalFlag = false;
bool wordAlignmentFlag = false;
bool onlyDirectFlag = false;
bool goodTuringFlag = false;
#define GT_MAX 10
bool logProbFlag = false;
int negLogProb = 1;
bool lexFlag = true;
int countOfCounts[GT_MAX+1];
float discountFactor[GT_MAX+1];
int maxLinesGTDiscount = -1;

int main(int argc, char* argv[]) 
{
	cerr << "Score v2.0 written by Philipp Koehn\n"
	     << "scoring methods for extracted rules\n";

	if (argc < 4) {
		cerr << "syntax: score extract lex phrase-table [--Inverse] [--Hierarchical] [--OnlyDirect] [--LogProb] [--NegLogProb] [--NoLex] [--GoodTuring] [--WordAlignment file]\n";
		exit(1);
	}
	char* fileNameExtract = argv[1];
	char* fileNameLex = argv[2];
	char* fileNamePhraseTable = argv[3];

	for(int i=4;i<argc;i++) {
		if (strcmp(argv[i],"inverse") == 0 || strcmp(argv[i],"--Inverse") == 0) {
			inverseFlag = true;
			cerr << "using inverse mode\n";
		}
		else if (strcmp(argv[i],"--Hierarchical") == 0) {
			hierarchicalFlag = true;
			cerr << "processing hierarchical rules\n";
		}
		else if (strcmp(argv[i],"--OnlyDirect") == 0) {
			onlyDirectFlag = true;
			cerr << "outputing in correct phrase table format (no merging with inverse)\n";
		}
		else if (strcmp(argv[i],"--WordAlignment") == 0) {
			wordAlignmentFlag = true;
			cerr << "outputing word alignment" << endl;
		}
		else if (strcmp(argv[i],"--NoLex") == 0) {
			lexFlag = false;
			cerr << "not computing lexical translation score\n";
		}
		else if (strcmp(argv[i],"--GoodTuring") == 0) {
			goodTuringFlag = true;
			cerr << "using Good Turing discounting\n";
		}
		else if (strcmp(argv[i],"--LogProb") == 0) {
			logProbFlag = true;
			cerr << "using log-probabilities\n";
		}
		else if (strcmp(argv[i],"--NegLogProb") == 0) {
			logProbFlag = true;
			negLogProb = -1;
			cerr << "using negative log-probabilities\n";
		}
		else if (strcmp(argv[i],"--MaxLinesGTDiscount") == 0) {
			++i;
			maxLinesGTDiscount = atoi(argv[i]);
			cerr << "maxLinesGTDiscount=" << maxLinesGTDiscount << endl;
		}
		else {
			cerr << "ERROR: unknown option " << argv[i] << endl;
			exit(1);
		}
	}

	// lexical translation table
	if (lexFlag)
		lexTable.load( fileNameLex );
  
	// compute count of counts for Good Turing discounting
	if (goodTuringFlag)
		computeCountOfCounts( fileNameExtract, maxLinesGTDiscount );

	// sorted phrase extraction file
	ifstream extractFile;
	extractFile.open(fileNameExtract);
	if (extractFile.fail()) {
		cerr << "ERROR: could not open extract file " << fileNameExtract << endl;
		exit(1);
	}
	istream &extractFileP = extractFile;

	// output file: phrase translation table
	phraseTableFile.open(fileNamePhraseTable);
	if (phraseTableFile.fail()) 
	{
		cerr << "ERROR: could not open file phrase table file " 
		     << fileNamePhraseTable << endl;
		exit(1);
	}
  
  // loop through all extracted phrase translations
  int lastSource = -1;
  vector< PhraseAlignment > phrasePairsWithSameF;
  int i=0;
	char line[LINE_MAX_LENGTH],lastLine[LINE_MAX_LENGTH];
	lastLine[0] = '\0';
	PhraseAlignment *lastPhrasePair = NULL;
  while(true) {
    if (extractFileP.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (extractFileP.eof())	break;
				
		// identical to last line? just add count
		if (lastSource > 0 && strcmp(line,lastLine) == 0)
		{
			lastPhrasePair->addToCount( line );
			continue;			
		}
		strcpy( lastLine, line );

		// create new phrase pair
		PhraseAlignment phrasePair;
		phrasePair.create( line, i );
		
		// only differs in count? just add count
		if (lastPhrasePair != NULL && lastPhrasePair->equals( phrasePair ))
		{
			lastPhrasePair->count += phrasePair.count;
			phrasePair.clear();
			continue;
		}
		
		// if new source phrase, process last batch
		if (lastSource >= 0 && lastSource != phrasePair.GetSource()) {
			processPhrasePairs( phrasePairsWithSameF );
			for(int j=0;j<phrasePairsWithSameF.size();j++)
				phrasePairsWithSameF[j].clear();
			phrasePairsWithSameF.clear();
			phraseTableT.clear();
			phraseTableS.clear();
			// process line again, since phrase tables flushed
			phrasePair.clear();
			phrasePair.create( line, i ); 
		}
		
		// add phrase pairs to list, it's now the last one
		lastSource = phrasePair.GetSource();
		phrasePairsWithSameF.push_back( phrasePair );
		lastPhrasePair = &phrasePairsWithSameF[phrasePairsWithSameF.size()-1];
	}
	processPhrasePairs( phrasePairsWithSameF );
	phraseTableFile.close();
}

void computeCountOfCounts( char* fileNameExtract, int maxLines )
{
	cerr << "computing counts of counts";
	for(int i=1;i<=GT_MAX;i++) countOfCounts[i] = 0;

	ifstream extractFile;
	extractFile.open( fileNameExtract );
	if (extractFile.fail()) {
		cerr << "ERROR: could not open extract file " << fileNameExtract << endl;
		exit(1);
	}
	istream &extractFileP = extractFile;

	// loop through all extracted phrase translations
	int lineNum = 0;
	char line[LINE_MAX_LENGTH],lastLine[LINE_MAX_LENGTH];
	lastLine[0] = '\0';
	PhraseAlignment *lastPhrasePair = NULL;
	while(true) {
		if (extractFileP.eof()) break;
		if (maxLines > 0 && lineNum >= maxLines) break;
		if (++lineNum % 100000 == 0) cerr << "." << flush;
		SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
		if (extractFileP.eof())	break;
		
		// identical to last line? just add count
		if (strcmp(line,lastLine) == 0)
		{
			lastPhrasePair->addToCount( line );
			continue;			
		}
		strcpy( lastLine, line );

		// create new phrase pair
		PhraseAlignment *phrasePair = new PhraseAlignment();
		phrasePair->create( line, lineNum );
		
		if (lineNum == 1)
		{
			lastPhrasePair = phrasePair;
			continue;
		}

		// only differs in count? just add count
		if (lastPhrasePair->match( *phrasePair ))
		{
			lastPhrasePair->count += phrasePair->count;
			phrasePair->clear();
			delete(phrasePair);
			continue;
		}

		// periodically house cleaning
		if (phrasePair->GetSource() != lastPhrasePair->GetSource())
		{
			phraseTableT.clear(); // these would get too big
			phraseTableS.clear(); // these would get too big
			// process line again, since phrase tables flushed
			phrasePair->clear();
			phrasePair->create( line, lineNum ); 
		}

		int count = lastPhrasePair->count + 0.99999;
		if(count <= GT_MAX)
			countOfCounts[ count ]++;
		lastPhrasePair->clear();
		delete( lastPhrasePair );
		lastPhrasePair = phrasePair;
	}
	
	delete lastPhrasePair;

	discountFactor[0] = 0.01; // floor
	cerr << "\n";
	for(int i=1;i<GT_MAX;i++)
	{
		discountFactor[i] = ((float)i+1)/(float)i*(((float)countOfCounts[i+1]+0.1) / ((float)countOfCounts[i]+0.1));
		cerr << "count " << i << ": " << countOfCounts[ i ] << ", discount factor: " << discountFactor[i];
		// some smoothing...
		if (discountFactor[i]>1) 
			discountFactor[i] = 1;
		if (discountFactor[i]<discountFactor[i-1])
			discountFactor[i] = discountFactor[i-1];
		cerr << " -> " << discountFactor[i]*i << endl;
	}
}
	
void processPhrasePairs( vector< PhraseAlignment > &phrasePair ) {
  if (phrasePair.size() == 0) return;

	// group phrase pairs based on alignments that matter
	// (i.e. that re-arrange non-terminals)
	vector< vector< PhraseAlignment * > > phrasePairGroup;
	float totalSource = 0;
	
	// loop through phrase pairs
	for(size_t i=0; i<phrasePair.size(); i++)
	{
		// add to total count
		totalSource += phrasePair[i].count;

		bool matched = false;
		// check for matches
		for(size_t g=0; g<phrasePairGroup.size(); g++)
		{
			vector< PhraseAlignment* > &group = phrasePairGroup[g];
			// matched? place into same group
			if ( group[0]->match( phrasePair[i] ))
			{
				group.push_back( &phrasePair[i] );
				matched = true;
			}
		}
		// not matched? create new group
		if (! matched) 
		{
			vector< PhraseAlignment* > newGroup;
			newGroup.push_back( &phrasePair[i] );
			phrasePairGroup.push_back( newGroup );
		}
	}
	
	for(size_t g=0; g<phrasePairGroup.size(); g++)
	{
		vector< PhraseAlignment* > &group = phrasePairGroup[g];
		outputPhrasePair( group, totalSource );
	}
}

PhraseAlignment* findBestAlignment( vector< PhraseAlignment* > &phrasePair ) 
{
	float bestAlignmentCount = -1;
	PhraseAlignment* bestAlignment;

	for(int i=0;i<phrasePair.size();i++) 
	{
		if (phrasePair[i]->count > bestAlignmentCount)
		{
			bestAlignmentCount = phrasePair[i]->count;
			bestAlignment = phrasePair[i];
		}
	}
	
	return bestAlignment;
}

void outputPhrasePair( vector< PhraseAlignment* > &phrasePair, float totalCount ) 
{
  if (phrasePair.size() == 0) return;

	PhraseAlignment *bestAlignment = findBestAlignment( phrasePair );

	// compute count
	float count = 0;
	for(size_t i=0;i<phrasePair.size();i++)
	{
		count += phrasePair[i]->count;
	}

	PHRASE phraseS = phraseTableS.getPhrase( phrasePair[0]->GetSource() );
	PHRASE phraseT = phraseTableT.getPhrase( phrasePair[0]->GetTarget() );

	// labels (if hierarchical)

	// source phrase (unless inverse)
	if (! inverseFlag) 
	{
		for(int j=0;j<phraseS.size();j++)
		{
			phraseTableFile << vcbS.getWord( phraseS[j] );
			phraseTableFile << " ";
		}
		phraseTableFile << "||| ";
	}
	
	// target phrase
	for(int j=0;j<phraseT.size();j++)
	{
		phraseTableFile << vcbT.getWord( phraseT[j] );
		phraseTableFile << " ";
	}
	phraseTableFile << "||| ";
	
	// source phrase (if inverse)
	if (inverseFlag) 
	{
		for(int j=0;j<phraseS.size();j++)
		{
			phraseTableFile << vcbS.getWord( phraseS[j] );
			phraseTableFile << " ";
		}
		phraseTableFile << "||| ";
	}

	// phrase translation probability
	if (goodTuringFlag && count<GT_MAX)
		count *= discountFactor[(int)(count+0.99999)];
	double condScore = count / totalCount;	
	phraseTableFile << ( logProbFlag ? negLogProb*log(condScore) : condScore );
	
	// lexical translation probability
	if (lexFlag)
	{
		double lexScore = computeLexicalTranslation( phraseS, phraseT, bestAlignment);
		phraseTableFile << " " << ( logProbFlag ? negLogProb*log(lexScore) : lexScore );
	}
	
	phraseTableFile << " ||| ";

	// alignment info for non-terminals
	if (! inverseFlag)
	{
		if (hierarchicalFlag) 
		{ // always output alignment if hiero style, but only for non-terms
			assert(phraseT.size() == bestAlignment->alignedToT.size() + 1);
			for(int j = 0; j < phraseT.size() - 1; j++)
			{
				if (isNonTerminal(vcbT.getWord( phraseT[j] )))
				{
					if (bestAlignment->alignedToT[ j ].size() != 1)
					{
						cerr << "Error: unequal numbers of non-terminals. Make sure the text does not contain words in square brackets (like [xxx])." << endl;
						phraseTableFile.flush();
						assert(bestAlignment->alignedToT[ j ].size() == 1);
					}
					int sourcePos = *(bestAlignment->alignedToT[ j ].begin());
					phraseTableFile << sourcePos << "-" << j << " ";
				}
			}
		}
		else if (wordAlignmentFlag)
		{ // alignment info in pb model
			for(int j=0;j<bestAlignment->alignedToT.size();j++)
			{
				const set< size_t > &aligned = bestAlignment->alignedToT[j];
				for (set< size_t >::const_iterator p(aligned.begin()); p != aligned.end(); ++p)
				{
					phraseTableFile << *p << "-" << j << " ";
				}
			}
		}
	}

	phraseTableFile << " ||| " << totalCount;
	phraseTableFile << endl;
}

double computeLexicalTranslation( PHRASE &phraseS, PHRASE &phraseT, PhraseAlignment *alignment ) {
	// lexical translation probability
	double lexScore = 1.0;
	int null = vcbS.getWordID("NULL");
	// all target words have to be explained
	for(int ti=0;ti<alignment->alignedToT.size();ti++) { 
    const set< size_t > & srcIndices = alignment->alignedToT[ ti ];
		if (srcIndices.empty())
		{ // explain unaligned word by NULL
			lexScore *= lexTable.permissiveLookup( null, phraseT[ ti ] ); 
		}
		else 
		{ // go through all the aligned words to compute average
			double thisWordScore = 0;
      for (set< size_t >::const_iterator p(srcIndices.begin()); p != srcIndices.end(); ++p) {
				thisWordScore += lexTable.permissiveLookup( phraseS[ *p ], phraseT[ ti ] );
			}
			lexScore *= thisWordScore / (double)srcIndices.size();
		}
	}
	return lexScore;
}

void LexicalTable::load( char *fileName ) 
{
  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName);
  if (inFile.fail()) 
	{
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];

  int i=0;
  while(true) 
	{
    i++;
    if (i%100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 3) 
		{
      cerr << "line " << i << " in " << fileName 
			     << " has wrong number of tokens, skipping:\n"
			     << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }
    
    double prob = atof( token[2].c_str() );
    WORD_ID wordT = vcbT.storeIfNew( token[0] );
    WORD_ID wordS = vcbS.storeIfNew( token[1] );
    ltable[ wordS ][ wordT ] = prob;
  }
  cerr << endl;
}

