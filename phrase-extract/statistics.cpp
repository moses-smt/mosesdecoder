// $Id$
// vim:tabstop=2

#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "AlignmentPhrase.h"
#include "SafeGetline.h"
#include "tables-core.h"
#include "InputFileStream.h"

using namespace std;
using namespace MosesTraining;

#define LINE_MAX_LENGTH 10000

namespace MosesTraining
{

class PhraseAlignment
{
public:
  int english, foreign;
  vector< vector<size_t> > alignedToE;
  vector< vector<size_t> > alignedToF;

  bool create( char*, int );
  void clear();
  bool equals( const PhraseAlignment& );
};

class LexicalTable
{
public:
  map< WORD_ID, map< WORD_ID, double > > ltable;
  void load( const string &);
};

}

void processPhrasePairs( vector< PhraseAlignment > & );

ofstream phraseTableFile;

Vocabulary vcbE;
Vocabulary vcbF;
LexicalTable lexTable;
PhraseTable phraseTableE;
PhraseTable phraseTableF;
bool inverseFlag;
int phrasePairBase = 0; // only used for "proper" conditioning

int main(int argc, char* argv[])
{
  cerr << "PhraseStatistics v1.1 written by Nicola Bertoldi\n"
       << "modifying PhraseScore v1.4 written by Philipp Koehn\n"
       << "It computes statistics for extracted phrase pairs\n"
       << "if (direct):\n"
       << "src_phrase ||| trg_phrase || freq(src_phrase, trg_phrase) freq(src_phrase) length(src_phrase) length(trg_phrase)\n"
       << "if (inverse)\n"
       << "src_phrase ||| trg_phrase || freq(src_phrase, trg_phrase) freq(trg_phrase) length(src_phrase) length(trg_phrase)\n";
  time_t starttime = time(NULL);

  if (argc != 4 && argc != 5) {
    cerr << "syntax: statistics extract lex phrase-table [inverse]\n";
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

  // lexical translation table
  lexTable.load( fileNameLex );

  // sorted phrase extraction file
  Moses::InputFileStream extractFile(fileNameExtract);

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
    SAFE_GETLINE((extractFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    //    if (fileCount>0)
    if (extractFileP.eof())
      break;
    PhraseAlignment phrasePair;
    bool isPhrasePair = phrasePair.create( line, i );
    if (lastForeign >= 0 && lastForeign != phrasePair.foreign) {
      processPhrasePairs( phrasePairsWithSameF );
      for(size_t j=0; j<phrasePairsWithSameF.size(); j++)
        phrasePairsWithSameF[j].clear();
      phrasePairsWithSameF.clear();
      phraseTableE.clear();
      phraseTableF.clear();
      phrasePair.clear(); // process line again, since phrase tables flushed
      phrasePair.create( line, i );
      phrasePairBase = 0;
    }
    lastForeign = phrasePair.foreign;
    if (isPhrasePair)
      phrasePairsWithSameF.push_back( phrasePair );
    else
      phrasePairBase++;
  }
  processPhrasePairs( phrasePairsWithSameF );
  phraseTableFile.close();
}

void processPhrasePairs( vector< PhraseAlignment > &phrasePair )
{
  if (phrasePair.size() == 0) return;
  map<int, int> countE;
  map<int, int> alignmentE;
  int totalCount = 0;
  int currentCount = 0;
  int maxSameCount = 0;
  int maxSame = -1;
  int old = -1;
  for(size_t i=0; i<phrasePair.size(); i++) {
    if (i>0) {
      if (phrasePair[old].english == phrasePair[i].english) {
        if (! phrasePair[i].equals( phrasePair[old] )) {
          if (currentCount > maxSameCount) {
            maxSameCount = currentCount;
            maxSame = i-1;
          }
          currentCount = 0;
        }
      } else {
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
  size_t index = 0;
  for(II i = countE.begin(); i != countE.end(); i++) {
    //cout << "\tp( " << i->first << " | " << phrasePair[0].foreign << " ; " << phraseF.size() << " ) = ...\n";
    //cerr << index << endl;

    // foreign phrase (unless inverse)
    if (! inverseFlag) {
      for(size_t j=0; j<phraseF.size(); j++) {
        phraseTableFile << vcbF.getWord( phraseF[j] );
        phraseTableFile << " ";
      }
      phraseTableFile << "||| ";
    }

    // english phrase
    PHRASE phraseE = phraseTableE.getPhrase( i->first );
    for(size_t j=0; j<phraseE.size(); j++) {
      phraseTableFile << vcbE.getWord( phraseE[j] );
      phraseTableFile << " ";
    }
    phraseTableFile << "||| ";

    // foreign phrase (if inverse)
    if (inverseFlag) {
      for(size_t j=0; j<phraseF.size(); j++) {
        phraseTableFile << vcbF.getWord( phraseF[j] );
        phraseTableFile << " ";
      }
      phraseTableFile << "||| ";
    }

    // phrase pair frequency
    phraseTableFile << i->second;

    //source phrase pair frequency
    phraseTableFile << " " << phrasePair.size();

    // source phrase length
    phraseTableFile	<< " " << phraseF.size();

    // target phrase length
    phraseTableFile	<< " " << phraseE.size();

    phraseTableFile << endl;

    index += i->second;
  }
}

bool PhraseAlignment::create( char line[], int lineID )
{
  vector< string > token = tokenize( line );
  int item = 1;
  PHRASE phraseF, phraseE;
  for (size_t j=0; j<token.size(); j++) {
    if (token[j] == "|||") item++;
    else {
      if (item == 1)
        phraseF.push_back( vcbF.storeIfNew( token[j] ) );
      else if (item == 2)
        phraseE.push_back( vcbE.storeIfNew( token[j] ) );
      else if (item == 3) {
        int e,f;
        sscanf(token[j].c_str(), "%d-%d", &f, &e);
        if ((size_t)e >= phraseE.size() || (size_t)f >= phraseF.size()) {
          cerr << "WARNING: sentence " << lineID << " has alignment point (" << f << ", " << e << ") out of bounds (" << phraseF.size() << ", " << phraseE.size() << ")\n";
        } else {
          if (alignedToE.size() == 0) {
            vector< size_t > dummy;
            for(size_t i=0; i<phraseE.size(); i++)
              alignedToE.push_back( dummy );
            for(size_t i=0; i<phraseF.size(); i++)
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
  return (item>2); // real phrase pair, not just foreign phrase
}

void PhraseAlignment::clear()
{
  for(size_t i=0; i<alignedToE.size(); i++)
    alignedToE[i].clear();
  for(size_t i=0; i<alignedToF.size(); i++)
    alignedToF[i].clear();
  alignedToE.clear();
  alignedToF.clear();
}

bool PhraseAlignment::equals( const PhraseAlignment& other )
{
  if (this == &other) return true;
  if (other.english != english) return false;
  if (other.foreign != foreign) return false;
  PHRASE phraseE = phraseTableE.getPhrase( english );
  PHRASE phraseF = phraseTableF.getPhrase( foreign );
  for(size_t i=0; i<phraseE.size(); i++) {
    if (alignedToE[i].size() != other.alignedToE[i].size()) return false;
    for(size_t j=0; j<alignedToE[i].size(); j++) {
      if (alignedToE[i][j] != other.alignedToE[i][j]) return false;
    }
  }
  for(size_t i=0; i<phraseF.size(); i++) {
    if (alignedToF[i].size() != other.alignedToF[i].size()) return false;
    for(size_t j=0; j<alignedToF[i].size(); j++) {
      if (alignedToF[i][j] != other.alignedToF[i][j]) return false;
    }
  }
  return true;
}

void LexicalTable::load( const string &filePath )
{
  cerr << "Loading lexical translation table from " << filePath;
  ifstream inFile;
  inFile.open(filePath.c_str());
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
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 3) {
      cerr << "line " << i << " in " << filePath << " has wrong number of tokens, skipping:\n" <<
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
