// $Id$

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

using namespace std;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();}
#define LINE_MAX_LENGTH 10000

class SentenceAlignment {
 public:
  vector<string> english;
  vector<string> foreign;
  vector<int> alignedCountF;
  vector< vector<int> > alignedToE;

  int create( char[], char[], char[], int );
  //  void clear() { delete(alignment); };
};

void extract( SentenceAlignment & );
void addPhrase( SentenceAlignment &, int, int, int, int );
vector<string> tokenize( char [] );
bool isAligned ( SentenceAlignment &, int, int );

ofstream extractFile;
ofstream extractFileInv;
ofstream extractFileOrientation;
int maxPhraseLength;
int phraseCount = 0;
char* fileNameExtract;
bool orientationFlag;
bool onlyOutputSpanInfo;

int main(int argc, char* argv[]) 
{
  cerr << "PhraseExtract v1.3.0, written by Philipp Koehn\n"
       << "phrase extraction from an aligned parallel corpus\n";
  time_t starttime = time(NULL);

  if (argc != 6 && argc != 7) {
    cerr << "syntax: phrase-extract en de align extract max-length [orientation | --OnlyOutputSpanInfo]\n";
    exit(1);
  }
  char* &fileNameE = argv[1];
  char* &fileNameF = argv[2];
  char* &fileNameA = argv[3];
  fileNameExtract = argv[4];
  maxPhraseLength = atoi(argv[5]);
  onlyOutputSpanInfo = argc == 7 && strcmp(argv[6],"--OnlyOutputSpanInfo") == 0; //az
  if (onlyOutputSpanInfo) cerr << "Only outputting span info in format (starting from 0): SrcBegin SrcEnd TgtBegin TgtEnd\n"; //az
  orientationFlag = (argc == 7 && !onlyOutputSpanInfo);
  if (orientationFlag) cerr << "(also extracting orientation)\n";

  //  string fileNameE = "/data/nlp/koehn/europarl-v2/models/de-en/model/aligned.en";
  //  string fileNameF = "/data/nlp/koehn/europarl-v2/models/de-en/model/aligned.de";
  //  string fileNameA = "/data/nlp/koehn/europarl-v2/models/de-en/model/aligned.grow-diag-final";

  ifstream eFile;
  ifstream fFile;
  ifstream aFile;
  eFile.open(fileNameE);
  fFile.open(fileNameF);
  aFile.open(fileNameA);
  istream *eFileP = &eFile;
  istream *fFileP = &fFile;
  istream *aFileP = &aFile;
  
  // string fileNameExtract = "/data/nlp/koehn/europarl-v2/models/de-en/model/new-extract";

  int i=0;
  while(true) {
    i++;
    if (i%10000 == 0) cerr << "." << flush;
    char englishString[LINE_MAX_LENGTH];
    char foreignString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*eFileP), englishString, LINE_MAX_LENGTH, '\n');
    if (eFileP->eof()) break;
    SAFE_GETLINE((*fFileP), foreignString, LINE_MAX_LENGTH, '\n');
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n');
    SentenceAlignment sentence;
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
      
    if (sentence.create( englishString, foreignString, alignmentString, i ))
      extract(sentence);
    if (onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

  eFile.close();
  fFile.close();
  aFile.close();
  //az: only close if we actually opened it
  if (!onlyOutputSpanInfo) {
    extractFile.close();
    extractFileInv.close();
    if (orientationFlag) extractFileOrientation.close();
  }
}
 
void extract( SentenceAlignment &sentence ) {
  int countE = sentence.english.size();
  int countF = sentence.foreign.size();

  // check alignments for english phrase startE...endE
  for(int startE=0;startE<countE;startE++) {
    for(int endE=startE;
	(endE<countE && endE<startE+maxPhraseLength);
	endE++) {
      
      int minF = 9999;
      int maxF = -1;
      vector< int > usedF = sentence.alignedCountF;
      for(int ei=startE;ei<=endE;ei++) {
	for(int i=0;i<sentence.alignedToE[ei].size();i++) {
	  int fi = sentence.alignedToE[ei][i];
	  // cout << "point (" << fi << ", " << ei << ")\n";
	  if (fi<minF) { minF = fi; }
	  if (fi>maxF) { maxF = fi; }
	  usedF[ fi ]--;
	}
      }
      
      // cout << "f projected ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n"; 

      if (maxF >= 0 && // aligned to any foreign words at all
	  maxF-minF < maxPhraseLength) { // foreign phrase within limits
	
	// check if foreign words are aligned to out of bound english words
	bool out_of_bounds = false;
	for(int fi=minF;fi<=maxF && !out_of_bounds;fi++)
	  if (usedF[fi]>0) {
	    // cout << "ouf of bounds: " << fi << "\n";
	    out_of_bounds = true;
	  }
	
	// cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n"; 
	if (!out_of_bounds)
	  // start point of foreign phrase may retreat over unaligned
	  for(int startF=minF;
	      (startF>=0 &&
	       startF>maxF-maxPhraseLength && // within length limit
	       (startF==minF || sentence.alignedCountF[startF]==0)); // unaligned
	      startF--)
	    // end point of foreign phrase may advance over unaligned
	    for(int endF=maxF;
		(endF<countF && 
		 endF<startF+maxPhraseLength && // within length limit
		 (endF==maxF || sentence.alignedCountF[endF]==0)); // unaligned
		endF++) 
	      addPhrase(sentence,startE,endE,startF,endF);
      }
    }
  }
}

void addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF ) {
  // foreign
  // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n"; 

 if (onlyOutputSpanInfo) {
   cout << startF << " " << endF << " " << startE << " " << endE << endl;
 } else {

  if (phraseCount % 10000000 == 0) {
    if (phraseCount>0) {
      extractFile.close();
      extractFileInv.close();
      if (orientationFlag) extractFileOrientation.close();
    }
    char part[10];
    sprintf(part,".part%04d",phraseCount/10000000);
    string fileNameExtractPart = string(fileNameExtract) + part;
    string fileNameExtractInvPart = string(fileNameExtract) + ".inv" + part;
    string fileNameExtractOrientationPart = string(fileNameExtract) + ".o" + part;
    extractFile.open(fileNameExtractPart.c_str());
    extractFileInv.open(fileNameExtractInvPart.c_str());
    if (orientationFlag) extractFileOrientation.open(fileNameExtractOrientationPart.c_str());
  }
  phraseCount++;

  for(int fi=startF;fi<=endF;fi++) {
    extractFile << sentence.foreign[fi] << " ";
    if (orientationFlag) extractFileOrientation << sentence.foreign[fi] << " ";
  }
  extractFile << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";

  // english
  for(int ei=startE;ei<=endE;ei++) {
    extractFile << sentence.english[ei] << " ";
    extractFileInv << sentence.english[ei] << " ";
    if (orientationFlag) extractFileOrientation << sentence.english[ei] << " ";
  }
  extractFile << "|||";
  extractFileInv << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";

  // foreign (for inverse)
  for(int fi=startF;fi<=endF;fi++)
    extractFileInv << sentence.foreign[fi] << " ";
  extractFileInv << "|||";

  // alignment
  for(int ei=startE;ei<=endE;ei++) 
    for(int i=0;i<sentence.alignedToE[ei].size();i++) {
      int fi = sentence.alignedToE[ei][i];
      extractFile << " " << fi-startF << "-" << ei-startE;
      extractFileInv << " " << ei-startE << "-" << fi-startF;
    }

  if (orientationFlag) {

    // orientation to previous E
    bool connectedLeftTop  = isAligned( sentence, startF-1, startE-1 );
    bool connectedRightTop = isAligned( sentence, endF+1,   startE-1 );
    if      ( connectedLeftTop && !connectedRightTop) 
      extractFileOrientation << "mono";
    else if (!connectedLeftTop &&  connectedRightTop) 
      extractFileOrientation << "swap";
    else 
      extractFileOrientation << "other";
  
    // orientation to following E
    bool connectedLeftBottom  = isAligned( sentence, startF-1, endE+1 );
    bool connectedRightBottom = isAligned( sentence, endF+1,   endE+1 );
    if      ( connectedLeftBottom && !connectedRightBottom) 
      extractFileOrientation << " swap";
    else if (!connectedLeftBottom &&  connectedRightBottom) 
      extractFileOrientation << " mono";
    else 
      extractFileOrientation << " other";
  }

  extractFile << "\n";
  extractFileInv << "\n";
  if (orientationFlag) extractFileOrientation << "\n";
 } // end: if (onlyOutputSpanInfo)
}
  
bool isAligned ( SentenceAlignment &sentence, int fi, int ei ) {
  if (ei == -1 && fi == -1) return true;
  if (ei <= -1 || fi <= -1) return false;
  if (ei == sentence.english.size() && fi == sentence.foreign.size()) return true;
  if (ei >= sentence.english.size() || fi >= sentence.foreign.size()) return false;
  for(int i=0;i<sentence.alignedToE[ei].size();i++) 
    if (sentence.alignedToE[ei][i] == fi) return true;
  return false;
}


int SentenceAlignment::create( char englishString[], char foreignString[], char alignmentString[], int sentenceID ) {
  english = tokenize( englishString );
  foreign = tokenize( foreignString );
  //  alignment = new bool[foreign.size()*english.size()];
  //  alignment = (bool**) calloc(english.size()*foreign.size(),sizeof(bool)); // is this right?
  
  if (english.size() == 0 || foreign.size() == 0) {
    cerr << "no english (" << english.size() << ") or foreign (" << foreign.size() << ") words << end insentence " << sentenceID << endl;
    cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
    return 0;
  }
  // cout << "english.size = " << english.size() << endl;
  // cout << "foreign.size = " << foreign.size() << endl;

  // cout << "xxx\n";
  for(int i=0; i<foreign.size(); i++) {
    // cout << "i" << i << endl;
    alignedCountF.push_back( 0 );
  }
  for(int i=0; i<english.size(); i++) {
    vector< int > dummy;
    alignedToE.push_back( dummy );
  }
  // cout << "\nscanning...\n";

  vector<string> alignmentSequence = tokenize( alignmentString );
  for(int i=0; i<alignmentSequence.size(); i++) {
    int e,f;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &f, &e)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentnce " << sentenceID << endl; 
      cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
      return 0;
    }
      // cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << f << ", " << e << endl;
    if (e >= english.size() || f >= foreign.size()) { 
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << f << ", " << e << ") out of bounds (" << foreign.size() << ", " << english.size() << ")\n";
      cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
      return 0;
    }
    alignedToE[e].push_back( f );
    alignedCountF[f]++;
  }
  return 1;
}

