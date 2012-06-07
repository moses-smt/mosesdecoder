/*
 * extract.cpp
 *
 *      Modified by: Nadi Tomeh - LIMSI/CNRS
 *      Machine Translation Marathon 2010, Dublin
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <cstring>

#include <map>
#include <set>
#include <vector>

#include "SafeGetline.h"
#include "SentenceAlignment.h"
#include "tables-core.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"

using namespace std;

#define LINE_MAX_LENGTH 500000

// HPhraseVertex represents a point in the alignment matrix
typedef pair <int, int> HPhraseVertex;

// Phrase represents a bi-phrase; each bi-phrase is defined by two points in the alignment matrix:
// bottom-left and top-right
typedef pair<HPhraseVertex, HPhraseVertex> HPhrase;

// HPhraseVector is a vector of HPhrases
typedef vector < HPhrase > HPhraseVector;

// SentenceVertices represents, from all extracted phrases, all vertices that have the same positioning
// The key of the map is the English index and the value is a set of the source ones
typedef map <int, set<int> > HSentenceVertices;

enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};

REO_POS getOrientWordModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                           int, int, int, int, int, int, int,
                           bool (*)(int, int), bool (*)(int, int));
REO_POS getOrientPhraseModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                             int, int, int, int, int, int, int,
                             bool (*)(int, int), bool (*)(int, int),
                             const HSentenceVertices &, const HSentenceVertices &);
REO_POS getOrientHierModel(SentenceAlignment &, REO_MODEL_TYPE, bool, bool,
                           int, int, int, int, int, int, int,
                           bool (*)(int, int), bool (*)(int, int),
                           const HSentenceVertices &, const HSentenceVertices &,
                           const HSentenceVertices &, const HSentenceVertices &,
                           REO_POS);

void insertVertex(HSentenceVertices &, int, int);
void insertPhraseVertices(HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &,
                          int, int, int, int);
string getOrientString(REO_POS, REO_MODEL_TYPE);

bool ge(int, int);
bool le(int, int);
bool lt(int, int);

void extractBase(SentenceAlignment &);
void extract(SentenceAlignment &);
void addPhrase(SentenceAlignment &, int, int, int, int, string &);
bool isAligned (SentenceAlignment &, int, int);

bool allModelsOutputFlag = false;

bool wordModel = false;
REO_MODEL_TYPE wordType = REO_MSD;
bool phraseModel = false;
REO_MODEL_TYPE phraseType = REO_MSD;
bool hierModel = false;
REO_MODEL_TYPE hierType = REO_MSD;


Moses::OutputFileStream extractFile;
Moses::OutputFileStream extractFileInv;
Moses::OutputFileStream extractFileOrientation;
Moses::OutputFileStream extractFileSentenceId;
int maxPhraseLength;
bool orientationFlag = false;
bool translationFlag = true;
bool sentenceIdFlag = false; //create extract file with sentence id
bool onlyOutputSpanInfo = false;
bool gzOutput = false;

int main(int argc, char* argv[])
{
  cerr	<< "PhraseExtract v1.4, written by Philipp Koehn\n"
        << "phrase extraction from an aligned parallel corpus\n";

  if (argc < 6) {
    cerr << "syntax: extract en de align extract max-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ] | --OnlyOutputSpanInfo | --NoTTable | --SentenceId]\n";
    exit(1);
  }
  char* &fileNameE = argv[1];
  char* &fileNameF = argv[2];
  char* &fileNameA = argv[3];
  string fileNameExtract = string(argv[4]);
  maxPhraseLength = atoi(argv[5]);

  for(int i=6; i<argc; i++) {
    if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
      onlyOutputSpanInfo = true;
    } else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
      orientationFlag = true;
    } else if (strcmp(argv[i],"--NoTTable") == 0) {
      translationFlag = false;
    } else if (strcmp(argv[i], "--SentenceId") == 0) {
      sentenceIdFlag = true;  
    } else if (strcmp(argv[i], "--GZOutput") == 0) {
      gzOutput = true;  
    } else if(strcmp(argv[i],"--model") == 0) {
      if (i+1 >= argc) {
        cerr << "extract: syntax error, no model's information provided to the option --model " << endl;
        exit(1);
      }
      char* modelParams = argv[++i];
      char* modelName = strtok(modelParams, "-");
      char* modelType = strtok(NULL, "-");

      REO_MODEL_TYPE intModelType;

      if(strcmp(modelName, "wbe") == 0) {
        wordModel = true;
        if(strcmp(modelType, "msd") == 0)
          wordType = REO_MSD;
        else if(strcmp(modelType, "mslr") == 0)
          wordType = REO_MSLR;
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          wordType = REO_MONO;
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else if(strcmp(modelName, "phrase") == 0) {
        phraseModel = true;
        if(strcmp(modelType, "msd") == 0)
          phraseType = REO_MSD;
        else if(strcmp(modelType, "mslr") == 0)
          phraseType = REO_MSLR;
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          phraseType = REO_MONO;
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else if(strcmp(modelName, "hier") == 0) {
        hierModel = true;
        if(strcmp(modelType, "msd") == 0)
          hierType = REO_MSD;
        else if(strcmp(modelType, "mslr") == 0)
          hierType = REO_MSLR;
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          hierType = REO_MONO;
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else {
        cerr << "extract: syntax error, unknown reordering model: " << modelName << endl;
        exit(1);
      }

      allModelsOutputFlag = true;
    } else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

  // default reordering model if no model selected
  // allows for the old syntax to be used
  if(orientationFlag && !allModelsOutputFlag) {
    wordModel = true;
    wordType = REO_MSD;
  }

  // open input files
  Moses::InputFileStream eFile(fileNameE);
  Moses::InputFileStream fFile(fileNameF);
  Moses::InputFileStream aFile(fileNameA);

  istream *eFileP = &eFile;
  istream *fFileP = &fFile;
  istream *aFileP = &aFile;

  // open output files
  if (translationFlag) {
    string fileNameExtractInv = fileNameExtract + ".inv" + (gzOutput?".gz":"");
    extractFile.Open( (fileNameExtract + (gzOutput?".gz":"")).c_str());
    extractFileInv.Open(fileNameExtractInv.c_str());
  }
  if (orientationFlag) {
    string fileNameExtractOrientation = fileNameExtract + ".o" + (gzOutput?".gz":"");
    extractFileOrientation.Open(fileNameExtractOrientation.c_str());
  }

  if (sentenceIdFlag) {
    string fileNameExtractSentenceId = fileNameExtract + ".sid" + (gzOutput?".gz":"");
    extractFileSentenceId.Open(fileNameExtractSentenceId.c_str());
  }

  int i=0;
  while(true) {
    i++;
    if (i%10000 == 0) cerr << "." << flush;
    char englishString[LINE_MAX_LENGTH];
    char foreignString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*eFileP), englishString, LINE_MAX_LENGTH, '\n', __FILE__);
    if (eFileP->eof()) break;
    SAFE_GETLINE((*fFileP), foreignString, LINE_MAX_LENGTH, '\n', __FILE__);
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n', __FILE__);
    SentenceAlignment sentence;
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }

    if (sentence.create( englishString, foreignString, alignmentString, i)) {
      extract(sentence);
    }
    if (onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }
  eFile.Close();
  fFile.Close();
  aFile.Close();
  //az: only close if we actually opened it
  if (!onlyOutputSpanInfo) {
    if (translationFlag) {
      extractFile.Close();
      extractFileInv.Close();
    }
    if (orientationFlag) extractFileOrientation.Close();
    if (sentenceIdFlag) {
      extractFileSentenceId.Close();
    }
  }
}

void extract(SentenceAlignment &sentence)
{
  int countE = sentence.target.size();
  int countF = sentence.source.size();

  HPhraseVector inboundPhrases;

  HSentenceVertices inTopLeft;
  HSentenceVertices inTopRight;
  HSentenceVertices inBottomLeft;
  HSentenceVertices inBottomRight;

  HSentenceVertices outTopLeft;
  HSentenceVertices outTopRight;
  HSentenceVertices outBottomLeft;
  HSentenceVertices outBottomRight;

  HSentenceVertices::const_iterator it;

  bool relaxLimit = hierModel;
  bool buildExtraStructure = phraseModel || hierModel;

  // check alignments for target phrase startE...endE
  // loop over extracted phrases which are compatible with the word-alignments
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && (relaxLimit || endE<startE+maxPhraseLength));
        endE++) {

      int minF = 9999;
      int maxF = -1;
      vector< int > usedF = sentence.alignedCountS;
      for(int ei=startE; ei<=endE; ei++) {
        for(size_t i=0; i<sentence.alignedToT[ei].size(); i++) {
          int fi = sentence.alignedToT[ei][i];
          if (fi<minF) {
            minF = fi;
          }
          if (fi>maxF) {
            maxF = fi;
          }
          usedF[ fi ]--;
        }
      }

      if (maxF >= 0 && // aligned to any source words at all
          (relaxLimit || maxF-minF < maxPhraseLength)) { // source phrase within limits

        // check if source words are aligned to out of bound target words
        bool out_of_bounds = false;
        for(int fi=minF; fi<=maxF && !out_of_bounds; fi++)
          if (usedF[fi]>0) {
            // cout << "ouf of bounds: " << fi << "\n";
            out_of_bounds = true;
          }

        // cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n";
        if (!out_of_bounds) {
          // start point of source phrase may retreat over unaligned
          for(int startF=minF;
              (startF>=0 &&
               (relaxLimit || startF>maxF-maxPhraseLength) && // within length limit
               (startF==minF || sentence.alignedCountS[startF]==0)); // unaligned
              startF--)
            // end point of source phrase may advance over unaligned
            for(int endF=maxF;
                (endF<countF &&
                 (relaxLimit || endF<startF+maxPhraseLength) && // within length limit
                 (endF==maxF || sentence.alignedCountS[endF]==0)); // unaligned
                endF++) { // at this point we have extracted a phrase
              if(buildExtraStructure) { // phrase || hier
                if(endE-startE < maxPhraseLength && endF-startF < maxPhraseLength) { // within limit
                  inboundPhrases.push_back(HPhrase(HPhraseVertex(startF,startE),
                                                   HPhraseVertex(endF,endE)));
                  insertPhraseVertices(inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                                       startF, startE, endF, endE);
                } else
                  insertPhraseVertices(outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                                       startF, startE, endF, endE);
              } else {
                string orientationInfo = "";
                if(wordModel) {
                  REO_POS wordPrevOrient, wordNextOrient;
                  bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
                  bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
                  bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
                  bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );
                  wordPrevOrient = getOrientWordModel(sentence, wordType, connectedLeftTopP, connectedRightTopP, startF, endF, startE, endE, countF, 0, 1, &ge, &lt);
                  wordNextOrient = getOrientWordModel(sentence, wordType, connectedLeftTopN, connectedRightTopN, endF, startF, endE, startE, 0, countF, -1, &lt, &ge);
                  orientationInfo += getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType);
                  if(allModelsOutputFlag)
                    " | | ";
                }
                addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
              }
            }
        }
      }
    }
  }

  if(buildExtraStructure) { // phrase || hier
    string orientationInfo = "";
    REO_POS wordPrevOrient, wordNextOrient, phrasePrevOrient, phraseNextOrient, hierPrevOrient, hierNextOrient;

    for(size_t i = 0; i < inboundPhrases.size(); i++) {
      int startF = inboundPhrases[i].first.first;
      int startE = inboundPhrases[i].first.second;
      int endF = inboundPhrases[i].second.first;
      int endE = inboundPhrases[i].second.second;

      bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
      bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
      bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
      bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );

      if(wordModel) {
        wordPrevOrient = getOrientWordModel(sentence, wordType,
                                            connectedLeftTopP, connectedRightTopP,
                                            startF, endF, startE, endE, countF, 0, 1,
                                            &ge, &lt);
        wordNextOrient = getOrientWordModel(sentence, wordType,
                                            connectedLeftTopN, connectedRightTopN,
                                            endF, startF, endE, startE, 0, countF, -1,
                                            &lt, &ge);
      }
      if (phraseModel) {
        phrasePrevOrient = getOrientPhraseModel(sentence, phraseType,
                                                connectedLeftTopP, connectedRightTopP,
                                                startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft);
        phraseNextOrient = getOrientPhraseModel(sentence, phraseType,
                                                connectedLeftTopN, connectedRightTopN,
                                                endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight);
      } else {
        phrasePrevOrient = phraseNextOrient = UNKNOWN;
      }
      if(hierModel) {
        hierPrevOrient = getOrientHierModel(sentence, hierType,
                                            connectedLeftTopP, connectedRightTopP,
                                            startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft, outBottomRight, outBottomLeft, phrasePrevOrient);
        hierNextOrient = getOrientHierModel(sentence, hierType,
                                            connectedLeftTopN, connectedRightTopN,
                                            endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight, outBottomLeft, outBottomRight, phraseNextOrient);
      }

      orientationInfo = ((wordModel)? getOrientString(wordPrevOrient, wordType) + " " + getOrientString(wordNextOrient, wordType) : "") + " | " +
                        ((phraseModel)? getOrientString(phrasePrevOrient, phraseType) + " " + getOrientString(phraseNextOrient, phraseType) : "") + " | " +
                        ((hierModel)? getOrientString(hierPrevOrient, hierType) + " " + getOrientString(hierNextOrient, hierType) : "");

      addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
    }
  }
}

REO_POS getOrientWordModel(SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                           bool connectedLeftTop, bool connectedRightTop,
                           int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                           bool (*ge)(int, int), bool (*lt)(int, int) )
{

  if( connectedLeftTop && !connectedRightTop)
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if (!connectedLeftTop &&  connectedRightTop)
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    connectedLeftTop = isAligned(sentence, indexF, startE-unit);
  for(int indexF=endF+2*unit; (*lt)(indexF,countF) && !connectedRightTop; indexF=indexF+unit)
    connectedRightTop = isAligned(sentence, indexF, startE-unit);
  if(connectedLeftTop && !connectedRightTop)
    return DRIGHT;
  else if(!connectedLeftTop && connectedRightTop)
    return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientPhraseModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                              bool connectedLeftTop, bool connectedRightTop,
                              int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                              bool (*ge)(int, int), bool (*lt)(int, int),
                              const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft)
{

  HSentenceVertices::const_iterator it;

  if((connectedLeftTop && !connectedRightTop) ||
      //(startE == 0 && startF == 0) ||
      //(startE == sentence.target.size()-1 && startF == sentence.source.size()-1) ||
      ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if((!connectedLeftTop &&  connectedRightTop) ||
      ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() && it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit)
    if(connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
                          it->second.find(indexF) != it->second.end())
      return DRIGHT;
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit)
    if(connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end() &&
                           it->second.find(indexF) != it->second.end())
      return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientHierModel (SentenceAlignment & sentence, REO_MODEL_TYPE modelType,
                            bool connectedLeftTop, bool connectedRightTop,
                            int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                            bool (*ge)(int, int), bool (*lt)(int, int),
                            const HSentenceVertices & inBottomRight, const HSentenceVertices & inBottomLeft,
                            const HSentenceVertices & outBottomRight, const HSentenceVertices & outBottomLeft,
                            REO_POS phraseOrient)
{

  HSentenceVertices::const_iterator it;

  if(phraseOrient == LEFT ||
      (connectedLeftTop && !connectedRightTop) ||
      //    (startE == 0 && startF == 0) ||
      //(startE == sentence.target.size()-1 && startF == sentence.source.size()-1) ||
      ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()) ||
      ((it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()))
    return LEFT;
  if(modelType == REO_MONO)
    return UNKNOWN;
  if(phraseOrient == RIGHT ||
      (!connectedLeftTop &&  connectedRightTop) ||
      ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() &&
       it->second.find(endF + unit) != it->second.end()) ||
      ((it = outBottomLeft.find(startE - unit)) != outBottomLeft.end() &&
       it->second.find(endF + unit) != it->second.end()))
    return RIGHT;
  if(modelType == REO_MSD)
    return UNKNOWN;
  if(phraseOrient != UNKNOWN)
    return phraseOrient;
  connectedLeftTop = false;
  for(int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit) {
    if((connectedLeftTop = (it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
                           it->second.find(indexF) != it->second.end()) ||
        (connectedLeftTop = (it = outBottomRight.find(startE - unit)) != outBottomRight.end() &&
                            it->second.find(indexF) != it->second.end()))
      return DRIGHT;
  }
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit) {
    if((connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomRight.end() &&
                            it->second.find(indexF) != it->second.end()) ||
        (connectedRightTop = (it = outBottomLeft.find(startE - unit)) != outBottomRight.end() &&
                             it->second.find(indexF) != it->second.end()))
      return DLEFT;
  }
  return UNKNOWN;
}

bool isAligned ( SentenceAlignment &sentence, int fi, int ei )
{
  if (ei == -1 && fi == -1)
    return true;
  if (ei <= -1 || fi <= -1)
    return false;
  if ((size_t)ei == sentence.target.size() && (size_t)fi == sentence.source.size())
    return true;
  if ((size_t)ei >= sentence.target.size() || (size_t)fi >= sentence.source.size())
    return false;
  for(size_t i=0; i<sentence.alignedToT[ei].size(); i++)
    if (sentence.alignedToT[ei][i] == fi)
      return true;
  return false;
}

bool ge(int first, int second)
{
  return first >= second;
}

bool le(int first, int second)
{
  return first <= second;
}

bool lt(int first, int second)
{
  return first < second;
}

void insertVertex( HSentenceVertices & corners, int x, int y )
{
  set<int> tmp;
  tmp.insert(x);
  pair< HSentenceVertices::iterator, bool > ret = corners.insert( pair<int, set<int> > (y, tmp) );
  if(ret.second == false) {
    ret.first->second.insert(x);
  }
}

void insertPhraseVertices(
  HSentenceVertices & topLeft,
  HSentenceVertices & topRight,
  HSentenceVertices & bottomLeft,
  HSentenceVertices & bottomRight,
  int startF, int startE, int endF, int endE)
{

  insertVertex(topLeft, startF, startE);
  insertVertex(topRight, endF, startE);
  insertVertex(bottomLeft, startF, endE);
  insertVertex(bottomRight, endF, endE);
}

string getOrientString(REO_POS orient, REO_MODEL_TYPE modelType)
{
  switch(orient) {
  case LEFT:
    return "mono";
    break;
  case RIGHT:
    return "swap";
    break;
  case DRIGHT:
    return "dright";
    break;
  case DLEFT:
    return "dleft";
    break;
  case UNKNOWN:
    switch(modelType) {
    case REO_MONO:
      return "nomono";
      break;
    case REO_MSD:
      return "other";
      break;
    case REO_MSLR:
      return "dright";
      break;
    }
    break;
  }
  return "";
}

void addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientationInfo)
{
  // source
  // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n";

  if (onlyOutputSpanInfo) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }

  for(int fi=startF; fi<=endF; fi++) {
    if (translationFlag) extractFile << sentence.source[fi] << " ";
    if (orientationFlag) extractFileOrientation << sentence.source[fi] << " ";
    if (sentenceIdFlag) extractFileSentenceId << sentence.source[fi] << " ";
  }
  if (translationFlag) extractFile << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";
  if (sentenceIdFlag) extractFileSentenceId << "||| ";

  // target
  for(int ei=startE; ei<=endE; ei++) {
    if (translationFlag) extractFile << sentence.target[ei] << " ";
    if (translationFlag) extractFileInv << sentence.target[ei] << " ";
    if (orientationFlag) extractFileOrientation << sentence.target[ei] << " ";
    if (sentenceIdFlag) extractFileSentenceId << sentence.target[ei] << " ";
  }
  if (translationFlag) extractFile << "|||";
  if (translationFlag) extractFileInv << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";
  if (sentenceIdFlag) extractFileSentenceId << "||| ";

  // source (for inverse)
  if (translationFlag) {
    for(int fi=startF; fi<=endF; fi++)
      extractFileInv << sentence.source[fi] << " ";
    extractFileInv << "|||";
  }

  // alignment
  if (translationFlag) {
    for(int ei=startE; ei<=endE; ei++) {
      for(size_t i=0; i<sentence.alignedToT[ei].size(); i++) {
        int fi = sentence.alignedToT[ei][i];
        extractFile << " " << fi-startF << "-" << ei-startE;
        extractFileInv << " " << ei-startE << "-" << fi-startF;
      }
    }
  }

  if (orientationFlag)
    extractFileOrientation << orientationInfo;

  if (sentenceIdFlag) {
    extractFileSentenceId << sentence.sentenceID;
  }

  if (translationFlag) extractFile << "\n";
  if (translationFlag) extractFileInv << "\n";
  if (orientationFlag) extractFileOrientation << "\n";
  if (sentenceIdFlag) extractFileSentenceId << "\n";
}

// if proper conditioning, we need the number of times a source phrase occured
void extractBase( SentenceAlignment &sentence )
{
  int countF = sentence.source.size();
  for(int startF=0; startF<countF; startF++) {
    for(int endF=startF;
        (endF<countF && endF<startF+maxPhraseLength);
        endF++) {
      for(int fi=startF; fi<=endF; fi++) {
        extractFile << sentence.source[fi] << " ";
      }
      extractFile << "|||" << endl;
    }
  }

  int countE = sentence.target.size();
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && endE<startE+maxPhraseLength);
        endE++) {
      for(int ei=startE; ei<=endE; ei++) {
        extractFileInv << sentence.target[ei] << " ";
      }
      extractFileInv << "|||" << endl;
    }
  }
}
