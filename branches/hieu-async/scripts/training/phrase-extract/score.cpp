// vim:tabstop=2

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "tables-core.h"

using namespace std;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();}
#define LINE_MAX_LENGTH 10000

class PhraseAlignment {
public:
  int english, foreign;
  vector< vector<int> > alignedToE;
  vector< vector<int> > alignedToF;
  
  void create( char*, int );
  void clear();
  bool equals( PhraseAlignment );
};

class LexicalTable {
public:
  map< WORD_ID, map< WORD_ID, double > > ltable;
  void load( char[] );
};

vector<string> tokenize( char [] );

void processPhrasePairs( vector< PhraseAlignment > & );

ofstream phraseTableFile;

Vocabulary vcbE;
Vocabulary vcbF;
LexicalTable lexTable;
PhraseTable phraseTableE;
PhraseTable phraseTableF;
bool inverseFlag;

int main(int argc, char* argv[]) 
{
  cerr << "PhraseScore v1.2.1, written by Philipp Koehn\n"
       << "phrase scoring methods for extracted phrases\n";
  time_t starttime = time(NULL);

  if (argc != 4 && argc != 5) {
    cerr << "syntax: phrase-score extract lex phrase-table [inverse]\n";
    exit(1);
  }
  char* &fileNameExtract = argv[1];
  char* &fileNameLex = argv[2];
  char* &fileNamePhraseTable = argv[3];
  inverseFlag = false;
  if (argc > 4) {
    inverseFlag = true;
    cerr << "using inverse mode\n";
  }
  //  char[] fileNameExtract& = "/data/nlp/koehn/europarl-v2/models/de-en/model/new-extract.sorted";
  //  string fileNameLex = "/data/nlp/koehn/europarl-v2/models/de-en/model/lex.f2n";
  //  string fileNamePhraseTable = "/data/nlp/koehn/europarl-v2/models/de-en/model/new-phrase-table-half.f2n";

  // lexical translation table
  lexTable.load( fileNameLex );
  
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
  if (phraseTableFile.fail()) {
    cerr << "ERROR: could not open file phrase table file " 
	 << fileNamePhraseTable << endl;
    exit(1);
  }
  
  // loop through all extracted phrase translations
  int lastForeign = -1;
  vector< PhraseAlignment > phrasePairsWithSameF;
  int i=0;
  int fileCount = 0;
  while(true) {
    if (extractFileP.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    char line[LINE_MAX_LENGTH];    
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n');
    //    if (fileCount>0)
    if (extractFileP.eof()) break;
    PhraseAlignment phrasePair;
    phrasePair.create( line, i );
    if (lastForeign >= 0 && lastForeign != phrasePair.foreign) {
      processPhrasePairs( phrasePairsWithSameF );
      for(int j=0;j<phrasePairsWithSameF.size();j++)
	phrasePairsWithSameF[j].clear();
      phrasePairsWithSameF.clear();
      phraseTableE.clear();
      phraseTableF.clear();
      phrasePair.clear(); // process line again, since phrase tables flushed
      phrasePair.create( line, i ); 
    }
    lastForeign = phrasePair.foreign;
    phrasePairsWithSameF.push_back( phrasePair );
  }
  processPhrasePairs( phrasePairsWithSameF );
  phraseTableFile.close();
}

void outputAlignment(const vector<int> &alignmentInfo)
{
	//phraseTableFile << "|";
	if (alignmentInfo.size() > 0)
		phraseTableFile << alignmentInfo[0];
	for (size_t pos = 1 ; pos < alignmentInfo.size() ; ++pos)
	{
		phraseTableFile << "," << alignmentInfo[pos];
	}
	phraseTableFile << " ";
}

void processPhrasePairs( vector< PhraseAlignment > &phrasePair ) {
  map<int, int> countE;
  map<int, int> alignmentE;
  int totalCount = 0;
  int currentCount = 0;
  int maxSameCount = 0;
  int maxSame = -1;
  int old = -1;
  for(int i=0;i<phrasePair.size();i++) {
    if (i>0) {
      if (phrasePair[old].english == phrasePair[i].english) {
	if (! phrasePair[i].equals( phrasePair[old] )) {
	  if (currentCount > maxSameCount) {
	    maxSameCount = currentCount;
	    maxSame = i-1;
	  }
	  currentCount = 0;
	}
      }
      else {
	// wrap up old E
	if (currentCount > maxSameCount) {
	  maxSameCount = currentCount;
	  maxSame = i-1;
	}

	alignmentE[ phrasePair[old].english ] = maxSame;
	//	if (maxSameCount != totalCount)
	//  cout << "max count is " << maxSameCount << "/" << totalCount << endl;
	
	// get ready for new E
	totalCount = 0;
	currentCount = 0;
	maxSameCount = 0;
	maxSame = -1;
      }
    }
    countE[ phrasePair[i].english ]++;
    old = i;
    currentCount++;
    totalCount++;
  }
  
  // wrap up old E
  if (currentCount > maxSameCount) {
    maxSameCount = currentCount;
    maxSame = phrasePair.size()-1;
  }
  alignmentE[ phrasePair[old].english ] = maxSame;
  //  if (maxSameCount != totalCount)
  //    cout << "max count is " << maxSameCount << "/" << totalCount << endl;

  // output table
  typedef map< int, int >::iterator II;
  PHRASE phraseF = phraseTableF.getPhrase( phrasePair[0].foreign );
  for(II i = countE.begin(); i != countE.end(); i++) {
    //    cout << "\tp( " << i->first << " | " << phrasePair[0].foreign << " ; " << phraseF.size() << " ) = ...\n";

    // foreign phrase (unless inverse)
    if (! inverseFlag) {
      for(int j=0;j<phraseF.size();j++)
			{
				phraseTableFile << vcbF.getWord( phraseF[j] );
				phraseTableFile << " ";
			}
      phraseTableFile << "||| ";
		}

    // english phrase
    PHRASE phraseE = phraseTableE.getPhrase( i->first );
		for(int j=0;j<phraseE.size();j++)
		{
      phraseTableFile << vcbE.getWord( phraseE[j] );
			phraseTableFile << " ";
		}
    phraseTableFile << "||| ";

    // foreign phrase (if inverse)
    if (inverseFlag) {
      for(int j=0;j<phraseF.size();j++)
			{
				phraseTableFile << vcbF.getWord( phraseF[j] );
				phraseTableFile << " ";
			}
      phraseTableFile << "||| ";
		}
 

		// output alignment		
		if (! inverseFlag) {
      for(int j=0;j<phraseF.size();j++)
      {
        outputAlignment(phrasePair[0].alignedToF[j]);
      }
      phraseTableFile << "a ||| ";
		}

    for(int j=0;j<phraseE.size();j++)
    {
      outputAlignment(phrasePair[ i->first ].alignedToE[j]);
    }
    phraseTableFile << "b ||| ";

		if (inverseFlag) {
		  for(int j=0;j<phraseF.size();j++)
  		{
    		outputAlignment(phrasePair[0].alignedToF[j]);
  		}
  		phraseTableFile << "c ||| ";
		}

    // phrase translation probability
    phraseTableFile << ((double) i->second / (double) phrasePair.size());

    // lexical translation probability
    double lexScore = 1;
    int null = vcbF.getWordID("NULL");
    PhraseAlignment &current = phrasePair[ alignmentE[ i->first ] ];
    for(int ei=0;ei<phraseE.size();ei++) { // all english words have to be explained
      if (current.alignedToE[ ei ].size() == 0)
	lexScore *= lexTable.ltable[ null ][ phraseE[ ei ] ]; // by NULL if neccessary
      else {
	double thisWordScore = 0;
	for(int j=0;j<current.alignedToE[ ei ].size();j++) {
	  thisWordScore += lexTable.ltable[ phraseF[current.alignedToE[ ei ][ j ] ] ][ phraseE[ ei ] ];
	  //	  cout << "lex" << j << "(" << vcbE.getWord( phraseE[ ei ] ) << "|" << vcbF.getWord( phraseF[current.alignedToE[ ei ][ j ] ] ) << ")=" << lexTable.ltable[ phraseF[current.alignedToE[ ei ][ j ] ] ][ phraseE[ ei ] ] << " ";
	}
	lexScore *= thisWordScore / (double)current.alignedToE[ ei ].size();
      }
      //      cout << " => " << lexScore << endl;
    }
    phraseTableFile << " " << lexScore;

    // model 1 score

    // zens&ney lexical score

    phraseTableFile << endl;
  }
}

void PhraseAlignment::create( char line[], int lineID ) {
  vector< string > token = tokenize( line );
  int item = 1;
  PHRASE phraseF, phraseE;
  for (int j=0; j<token.size(); j++) {
    if (token[j] == "|||") item++;
    else {
      if (item == 1)
	phraseF.push_back( vcbF.storeIfNew( token[j] ) );
      else if (item == 2)
	phraseE.push_back( vcbE.storeIfNew( token[j] ) );
      else if (item == 3) {
	int e,f;
	sscanf(token[j].c_str(), "%d-%d", &f, &e);
	if (e >= phraseE.size() || f >= phraseF.size()) { 
	  cerr << "WARNING: sentence " << lineID << " has alignment point (" << f << ", " << e << ") out of bounds (" << phraseF.size() << ", " << phraseE.size() << ")\n"; }
	else {
	  if (alignedToE.size() == 0) {
	    vector< int > dummy;
	    for(int i=0;i<phraseE.size();i++)
	      alignedToE.push_back( dummy );
	    for(int i=0;i<phraseF.size();i++)
	      alignedToF.push_back( dummy );
	    foreign = phraseTableF.storeIfNew( phraseF );
	    english = phraseTableE.storeIfNew( phraseE );
	  }
	  alignedToE[e].push_back( f );
	  alignedToF[f].push_back( e );
	}
      }
    }
  }
}

void PhraseAlignment::clear() {
  for(int i=0;i<alignedToE.size();i++)
    alignedToE[i].clear();
  for(int i=0;i<alignedToF.size();i++)
    alignedToF[i].clear();
  alignedToE.clear();
  alignedToF.clear();
}

bool PhraseAlignment::equals( PhraseAlignment other ) {
  if (other.english != english) return false;
  if (other.foreign != foreign) return false;
  PHRASE phraseE = phraseTableE.getPhrase( english );
  PHRASE phraseF = phraseTableF.getPhrase( foreign );
  for(int i=0;i<phraseE.size();i++) {
    if (alignedToE[i].size() != other.alignedToE[i].size()) return false;
    for(int j=0; j<alignedToE[i].size(); j++) {
      if (alignedToE[i][j] != other.alignedToE[i][j]) return false;
    }
  }
  for(int i=0;i<phraseF.size();i++) {
    if (alignedToF[i].size() != other.alignedToF[i].size()) return false;
    for(int j=0; j<alignedToF[i].size(); j++) {
      if (alignedToF[i][j] != other.alignedToF[i][j]) return false;
    }
  }
  return true;
}

void LexicalTable::load( char *fileName ) {
  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName);
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];

  int i=0;
  while(true) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n');
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 3) {
      cerr << "line " << i << " in " << fileName << " has wrong number of tokens, skipping:\n" <<
	token.size() << " " << token[0] << " " << line << endl;
      continue;
    }
    
    double prob = atof( token[2].c_str() );
    WORD_ID wordE = vcbE.storeIfNew( token[0] );
    WORD_ID wordF = vcbF.storeIfNew( token[1] );
    ltable[ wordF ][ wordE ] = prob;
  }
  cerr << endl;
}
