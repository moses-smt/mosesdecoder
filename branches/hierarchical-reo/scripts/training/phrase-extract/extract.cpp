// $Id$

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <cstring>

#include <map>
#include <set>
#include <vector>

using namespace std;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
                _IS.getline(_LINE, _SIZE, _DELIM); \
                if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear(); \
                if (_IS.gcount() == _SIZE-1) { \
                  cerr << "Line too long! Buffer overflow. Delete lines >=" \
                    << _SIZE << " chars or raise LINE_MAX_LENGTH in phrase-extract/extract.cpp" \
                    << endl; \
                    exit(1); \
                } \
              }
#define LINE_MAX_LENGTH 60000

// HPhraseVertex represents a point in the alignment matrix
typedef pair <int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a vector of HPhrases
typedef vector < HPhrase > HPhraseVector;

// SentenceVertices represents, from all extracted phrases, all vertices that have the same positioning
// The key of the map is the English index and the value is a set of the foreign ones
typedef map <int, set<int> > HSenteceVertices;

class SentenceAlignment {
	public:
		vector<string> english;
		vector<string> foreign;
		vector<int> alignedCountF;
		vector< vector<int> > alignedToE;

		int create( char[], char[], char[], int );
		//  void clear() { delete(alignment); };
};

void extractBase( SentenceAlignment & );
void extract( SentenceAlignment & );
void addPhrase( SentenceAlignment &, int, int, int, int );
vector<string> tokenize( char [] );
bool isAligned ( SentenceAlignment &, int, int );

// Reordering
void HRextract( SentenceAlignment & );
void HRaddPhrase( SentenceAlignment &, int, int, int, int, string &, string & );
void PBextract( SentenceAlignment & );

enum REO_MODEL_NAME {REO_HIER, REO_PHRASE, REO_WORD};
enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO, REO_LR};

bool allModelsOutputFlag = false;
REO_MODEL_NAME modelName;
REO_MODEL_TYPE modelType;

map < REO_MODEL_NAME, REO_MODEL_TYPE > selectedModels;

ofstream extractFile;
ofstream extractFileInv;
ofstream extractFileOrientation;
int maxPhraseLength;
int phraseCount = 0;
char* fileNameExtract;
bool orientationFlag = false;
bool onlyOutputSpanInfo = false;
bool noFileLimit = false;
bool zipFiles = false;
bool properConditioning = false;

int main(int argc, char* argv[]) 
{
	cerr	<< "PhraseExtract v1.4, written by Philipp Koehn\n"
			<< "phrase extraction from an aligned parallel corpus\n";
	time_t starttime = time(NULL);

	if (argc < 6) {
		cerr << "syntax: phrase-extract en de align extract max-length [orientation | --OnlyOutputSpanInfo | --NoFileLimit | --ProperConditioning | --hierarchical-reo]\n";
		exit(1);
	}
	char* &fileNameE = argv[1];
	char* &fileNameF = argv[2];
	char* &fileNameA = argv[3];
	fileNameExtract = argv[4];
	maxPhraseLength = atoi(argv[5]);

	for(int i=6;i<argc;i++) {
		if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
			onlyOutputSpanInfo = true;
		}
		else if (strcmp(argv[i],"--NoFileLimit") == 0) {
			noFileLimit = true;
		}
		else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
			orientationFlag = true;
			selectedModels.insert(make_pair(REO_WORD, REO_MSD));
		}
		else if(strcmp(argv[i],"--model") == 0){
			char* modelParams = argv[++i];
			char* modelName = strtok(modelParams, "-");
			char* modelType = strtok(modelParams, "-");

			REO_MODEL_NAME intModelName;
			REO_MODEL_TYPE intModelType;

			if(strcmp(modelName, "word") == 0)
				intModelName = REO_WORD;
			else if(strcmp(modelName, "phrase") == 0)
				intModelName = REO_PHRASE;
			else if(strcmp(modelName, "phrase") == 0)
				intModelName = REO_HIER;
			else{
				cerr << "extract: syntax error, unknown reordering model: " << modelName << endl;
				exit(1);
			}

			if(strcmp(modelType, "msd") == 0)
				intModelType = REO_MSD;
			else if(strcmp(modelType, "mslr") == 0)
				intModelType = REO_MSLR;
			else if(strcmp(modelType, "mono") == 0)
				intModelType = REO_MONO;
			else if(strcmp(modelType, "leftright") == 0)
				intModelType = REO_LR;
			else{
				cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
				exit(1);
			}
		}
    }
    else if (strcmp(argv[i],"--ZipFiles") == 0) {
      zipFiles = true;
    }
    else if (strcmp(argv[i],"--ProperConditioning") == 0) {
      properConditioning = true;
    }
    else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }
  ifstream eFile;
  ifstream fFile;
  ifstream aFile;
  eFile.open(fileNameE);
  fFile.open(fileNameF);
  aFile.open(fileNameA);
  istream *eFileP = &eFile;
  istream *fFileP = &fFile;
  istream *aFileP = &aFile;
  
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
      
    if (sentence.create( englishString, foreignString, alignmentString, i )) {
    	if(HRorientationFlag)
    		HRextract(sentence);
    	else if(PBorientationFlag)
    		PBextract(sentence);
    	else
    		extract(sentence);
      if (properConditioning) extractBase(sentence);
    }
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
 
// if proper conditioning, we need the number of times a foreign phrase occured
void extractBase( SentenceAlignment &sentence ) {
  int countF = sentence.foreign.size();
  for(int startF=0;startF<countF;startF++) {
    for(int endF=startF;
        (endF<countF && endF<startF+maxPhraseLength);
        endF++) {
      for(int fi=startF;fi<=endF;fi++) {
	extractFile << sentence.foreign[fi] << " ";
      }
      extractFile << "|||" << endl;
    }
  }

  int countE = sentence.english.size();
  for(int startE=0;startE<countE;startE++) {
    for(int endE=startE;
        (endE<countE && endE<startE+maxPhraseLength);
        endE++) {
      for(int ei=startE;ei<=endE;ei++) {
	extractFileInv << sentence.english[ei] << " ";
      }
      extractFileInv << "|||" << endl;
    }
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
    return;
  } 

  // new file every 1e7 phrases
  if (phraseCount % 10000000 == 0 // new file every 1e7 phrases
      && (!noFileLimit || phraseCount == 0)) { // only new partial file, if file limit

    // close old file
    if (!noFileLimit && phraseCount>0) {
      extractFile.close();
      extractFileInv.close();
      if (orientationFlag) extractFileOrientation.close();
    }
    
    // construct file name
    char part[10];
    if (noFileLimit)
      part[0] = '\0';
    else
      sprintf(part,".part%04d",phraseCount/10000000);  
    string fileNameExtractPart = string(fileNameExtract) + part;
    string fileNameExtractInvPart = string(fileNameExtract) + ".inv" + part;
    string fileNameExtractOrientationPart = string(fileNameExtract) + ".o" + part;

    
    // open files
    extractFile.open(fileNameExtractPart.c_str());
    extractFileInv.open(fileNameExtractInvPart.c_str());
    if (orientationFlag) 
      extractFileOrientation.open(fileNameExtractOrientationPart.c_str());
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
    {
    	if()
      extractFileOrientation << "mono";
    }
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

void HRextract( SentenceAlignment &sentence ) {
  int countE = sentence.english.size();
  int countF = sentence.foreign.size();

  HPhraseVector inboundPhrases;
  HSenteceVertices topLeft;
  HSenteceVertices topRight;
  HSenteceVertices bottomLeft;
  HSenteceVertices bottomRight;
  pair< HSenteceVertices::iterator, bool > ret;
  HSenteceVertices::const_iterator it;
  string orientPrevE, orientNextE;

  // check alignments for english phrase startE...endE
  for(int startE=0;startE<countE;startE++) {
    for(int endE=startE;
	(endE<countE /* && endE<startE+maxPhraseLength */);
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

      if (maxF >= 0 /* && // aligned to any foreign words at all
	  maxF-minF < maxPhraseLength */) { // foreign phrase within limits

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
	       /* startF>maxF-maxPhraseLength && */ // within length limit
	       (startF==minF || sentence.alignedCountF[startF]==0)); // unaligned
	      startF--)
	    // end point of foreign phrase may advance over unaligned
	    for(int endF=maxF;
		(endF<countF &&
		/* endF<startF+maxPhraseLength && */ // within length limit
		 (endF==maxF || sentence.alignedCountF[endF]==0)); // unaligned
		endF++){
//	    	cerr << "Added Phrase:" << endl;
//	    	cerr << startE << " - " << startF << " - " << endE << " - " << endF << endl;
	    	if(endE-startE < maxPhraseLength && endF-startF < maxPhraseLength)
	    		inboundPhrases.push_back(
						HPhrase(
							HPhraseVertex(startF,startE),
							HPhraseVertex(endF,endE)
	    				)
	    			);


	    	set<int> tmpSetSF;
	    	tmpSetSF.insert(startF);
	    	set<int> tmpSetEF;
	    	tmpSetEF.insert(endF);

	    	ret = topLeft.insert( pair<int, set<int> > (startE, tmpSetSF) );
	    	if(ret.second == false){
	    		ret.first->second.insert(startF);
	    	}

	    	ret = topRight.insert( pair<int, set<int> > (startE, tmpSetEF) );
			if(ret.second == false){
				ret.first->second.insert(endF);
			}

	    	ret = bottomLeft.insert( pair<int, set<int> > (endE, tmpSetSF) );
			if(ret.second == false){
				ret.first->second.insert(startF);
			}

	    	ret = bottomRight.insert( pair<int, set<int> > (endE, tmpSetEF) );
			if(ret.second == false){
				ret.first->second.insert(endF);
			}

			tmpSetSF.clear();
			tmpSetEF.clear();

//	    	addPhrase(sentence,startE,endE,startF,endF);
	    }
      }
    }
  }


	  for(int i = 0; i < inboundPhrases.size(); i++){
		  int startF = inboundPhrases[i].first.first;
		  int startE = inboundPhrases[i].first.second;
		  int endF = inboundPhrases[i].second.first;
		  int endE = inboundPhrases[i].second.second;

		  // Previous E
		  if( (startE == 0 && startF == 0) || ((it = bottomRight.find(startE - 1)) != bottomRight.end() && it->second.find(startF-1) != it->second.end()) )
			  orientPrevE = "mono";
		  else if((it = bottomLeft.find(startE - 1)) != bottomLeft.end() && it->second.find(endF + 1) != it->second.end())
			  orientPrevE = "swap";
		  else
			  orientPrevE = "other";

		  // Next E
		  if( (endE == sentence.english.size()-1 && endF == sentence.foreign.size()-1) || ((it = topLeft.find(endE + 1)) != topLeft.end() && it->second.find(endF+1) != it->second.end()))
			  orientNextE = "mono";
		  else if( (it = topRight.find(endE + 1)) != topRight.end() && it->second.find(startF-1) != it->second.end())
			  orientNextE = "swap";
		  else
			  orientNextE = "other";

		  HRaddPhrase(sentence, startE, endE, startF, endF, orientPrevE, orientNextE);
	  }
	  inboundPhrases.clear();
	  topLeft.clear();
	  topRight.clear();
	  bottomLeft.clear();
	  bottomRight.clear();
//	  exit(0);
}

void HRaddPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientPrevE, string &orientNextE) {
  // foreign
  // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n";

  if (onlyOutputSpanInfo) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }

  // new file every 1e7 phrases
  if (phraseCount % 10000000 == 0 // new file every 1e7 phrases
      && (!noFileLimit || phraseCount == 0)) { // only new partial file, if file limit

    // close old file
    if (!noFileLimit && phraseCount>0) {
      extractFile.close();
      extractFileInv.close();
      if (orientationFlag) extractFileOrientation.close();
    }

    // construct file name
    char part[10];
    if (noFileLimit)
      part[0] = '\0';
    else
      sprintf(part,".part%04d",phraseCount/10000000);
    string fileNameExtractPart = string(fileNameExtract) + part;
    string fileNameExtractInvPart = string(fileNameExtract) + ".inv" + part;
    string fileNameExtractOrientationPart = string(fileNameExtract) + ".o" + part;


    // open files
    extractFile.open(fileNameExtractPart.c_str());
    extractFileInv.open(fileNameExtractInvPart.c_str());
    if (orientationFlag)
      extractFileOrientation.open(fileNameExtractOrientationPart.c_str());
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
      extractFileOrientation << orientPrevE;

    // orientation to following E
      extractFileOrientation << " " << orientNextE;
  }

  extractFile << "\n";
  extractFileInv << "\n";
  if (orientationFlag) extractFileOrientation << "\n";
}

void PBextract( SentenceAlignment &sentence ) {
  int countE = sentence.english.size();
  int countF = sentence.foreign.size();

  HPhraseVector inboundPhrases;
  HSenteceVertices topLeft;
  HSenteceVertices topRight;
  HSenteceVertices bottomLeft;
  HSenteceVertices bottomRight;
  pair< HSenteceVertices::iterator, bool > ret;
  HSenteceVertices::const_iterator it;
  string orientPrevE, orientNextE;

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
		endF++){
//	    	cerr << "Added Phrase:" << endl;
//	    	cerr << startE << " - " << startF << " - " << endE << " - " << endF << endl;
	    	if(endE-startE < maxPhraseLength && endF-startF < maxPhraseLength)
	    		inboundPhrases.push_back(
						HPhrase(
							HPhraseVertex(startF,startE),
							HPhraseVertex(endF,endE)
	    				)
	    			);


	    	set<int> tmpSetSF;
	    	tmpSetSF.insert(startF);
	    	set<int> tmpSetEF;
	    	tmpSetEF.insert(endF);

	    	ret = topLeft.insert( pair<int, set<int> > (startE, tmpSetSF) );
	    	if(ret.second == false){
	    		ret.first->second.insert(startF);
	    	}

	    	ret = topRight.insert( pair<int, set<int> > (startE, tmpSetEF) );
			if(ret.second == false){
				ret.first->second.insert(endF);
			}

	    	ret = bottomLeft.insert( pair<int, set<int> > (endE, tmpSetSF) );
			if(ret.second == false){
				ret.first->second.insert(startF);
			}

	    	ret = bottomRight.insert( pair<int, set<int> > (endE, tmpSetEF) );
			if(ret.second == false){
				ret.first->second.insert(endF);
			}

			tmpSetSF.clear();
			tmpSetEF.clear();

//	    	addPhrase(sentence,startE,endE,startF,endF);
	    }
      }
    }
  }


	  for(int i = 0; i < inboundPhrases.size(); i++){
		  int startF = inboundPhrases[i].first.first;
		  int startE = inboundPhrases[i].first.second;
		  int endF = inboundPhrases[i].second.first;
		  int endE = inboundPhrases[i].second.second;

		  // Previous E
		  if( (startE == 0 && startF == 0) || ((it = bottomRight.find(startE - 1)) != bottomRight.end() && it->second.find(startF-1) != it->second.end()) )
			  orientPrevE = "mono";
		  else if((it = bottomLeft.find(startE - 1)) != bottomLeft.end() && it->second.find(endF + 1) != it->second.end())
			  orientPrevE = "swap";
		  else
			  orientPrevE = "other";

		  // Next E
		  if( (endE == sentence.english.size()-1 && endF == sentence.foreign.size()-1) || ((it = topLeft.find(endE + 1)) != topLeft.end() && it->second.find(endF+1) != it->second.end()))
			  orientNextE = "mono";
		  else if( (it = topRight.find(endE + 1)) != topRight.end() && it->second.find(startF-1) != it->second.end())
			  orientNextE = "swap";
		  else
			  orientNextE = "other";

		  HRaddPhrase(sentence, startE, endE, startF, endF, orientPrevE, orientNextE);
	  }
	  inboundPhrases.clear();
	  topLeft.clear();
	  topRight.clear();
	  bottomLeft.clear();
	  bottomRight.clear();
//	  exit(0);
}
