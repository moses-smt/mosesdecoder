/*
 * extract.cpp
 *	Modified by: Rohit Gupta CDAC, Mumbai, India
 *	on July 15, 2012 to implement parallel processing
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
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <limits>

#include "SentenceAlignment.h"
#include "tables-core.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "PhraseExtractionOptions.h"

using namespace std;
using namespace MosesTraining;

namespace MosesTraining
{

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

bool isAligned (SentenceAlignment &, int, int);

int sentenceOffset = 0;

std::vector<std::string> Tokenize(const std::string& str,
                                  const std::string& delimiters = " \t");

bool flexScoreFlag = false;

}

namespace MosesTraining
{

class ExtractTask
{
public:
  ExtractTask(size_t id, SentenceAlignment &sentence,PhraseExtractionOptions &initoptions, Moses::OutputFileStream &extractFileOrientation)
    :m_sentence(sentence),
    m_options(initoptions),
    m_extractFileOrientation(extractFileOrientation)
	{}
  void Run();
private:
  void extract(SentenceAlignment &);
  void addPhrase(SentenceAlignment &, int, int, int, int, string &);
  void writePhrasesToFile();

  SentenceAlignment &m_sentence;
  const PhraseExtractionOptions &m_options;
  Moses::OutputFileStream &m_extractFileOrientation;
};
}

int main(int argc, char* argv[])
{
  cerr	<< "PhraseExtract v1.4, written by Philipp Koehn\n"
        << "phrase extraction from an aligned parallel corpus\n";

  if (argc < 6) {
    cerr << "syntax: extract en de align extract max-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ] ";
    cerr<<"| --OnlyOutputSpanInfo | --NoTTable | --GZOutput | --IncludeSentenceId | --SentenceOffset n | --InstanceWeights filename ]\n";
    exit(1);
  }

  Moses::OutputFileStream extractFileOrientation;
  const char* const &fileNameE = argv[1];
  const char* const &fileNameF = argv[2];
  const char* const &fileNameA = argv[3];
  const string fileNameExtract = string(argv[4]);
  PhraseExtractionOptions options(atoi(argv[5]));

  for(int i=6; i<argc; i++) {
    if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
      options.initOnlyOutputSpanInfo(true);
    } else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
      options.initOrientationFlag(true);
    } else if (strcmp(argv[i],"--FlexibilityScore") == 0) {
      options.initFlexScoreFlag(true);
    } else if (strcmp(argv[i],"--NoTTable") == 0) {
      options.initTranslationFlag(false);
    } else if (strcmp(argv[i], "--IncludeSentenceId") == 0) {
      options.initIncludeSentenceIdFlag(true);
    } else if (strcmp(argv[i], "--SentenceOffset") == 0) {
      if (i+1 >= argc || argv[i+1][0] < '0' || argv[i+1][0] > '9') {
        cerr << "extract: syntax error, used switch --SentenceOffset without a number" << endl;
        exit(1);
      }
      sentenceOffset = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--GZOutput") == 0) {
      options.initGzOutput(true);
    } else if (strcmp(argv[i], "--InstanceWeights") == 0) {
      if (i+1 >= argc) {
        cerr << "extract: syntax error, used switch --InstanceWeights without file name" << endl;
        exit(1);
      }
      options.initInstanceWeightsFile(argv[++i]);
    } else if (strcmp(argv[i], "--Debug") == 0) {
    	options.debug = true;
    } else if (strcmp(argv[i], "--MinPhraseLength") == 0) {
    	options.minPhraseLength = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--Separator") == 0) {
    	options.separator = argv[++i];
    } else if(strcmp(argv[i],"--model") == 0) {
      if (i+1 >= argc) {
        cerr << "extract: syntax error, no model's information provided to the option --model " << endl;
        exit(1);
      }
      char*  modelParams = argv[++i];
      char*  modelName = strtok(modelParams, "-");
      char*  modelType = strtok(NULL, "-");

      // REO_MODEL_TYPE intModelType;

      if(strcmp(modelName, "wbe") == 0) {
        options.initWordModel(true);
        if(strcmp(modelType, "msd") == 0)
          options.initWordType(REO_MSD);
        else if(strcmp(modelType, "mslr") == 0)
          options.initWordType(REO_MSLR);
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          options.initWordType(REO_MONO);
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else if(strcmp(modelName, "phrase") == 0) {
        options.initPhraseModel(true);
        if(strcmp(modelType, "msd") == 0)
          options.initPhraseType(REO_MSD);
        else if(strcmp(modelType, "mslr") == 0)
          options.initPhraseType(REO_MSLR);
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          options.initPhraseType(REO_MONO);
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else if(strcmp(modelName, "hier") == 0) {
        options.initHierModel(true);
        if(strcmp(modelType, "msd") == 0)
          options.initHierType(REO_MSD);
        else if(strcmp(modelType, "mslr") == 0)
          options.initHierType(REO_MSLR);
        else if(strcmp(modelType, "mono") == 0 || strcmp(modelType, "monotonicity") == 0)
          options.initHierType(REO_MONO);
        else {
          cerr << "extract: syntax error, unknown reordering model type: " << modelType << endl;
          exit(1);
        }
      } else {
        cerr << "extract: syntax error, unknown reordering model: " << modelName << endl;
        exit(1);
      }

      options.initAllModelsOutputFlag(true);
    } else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }

  // default reordering model if no model selected
  // allows for the old syntax to be used
  if(options.isOrientationFlag() && !options.isAllModelsOutputFlag()) {
    options.initWordModel(true);
    options.initWordType(REO_MSD);
  }

  // open input files
  Moses::InputFileStream eFile(fileNameE);
  Moses::InputFileStream fFile(fileNameF);
  Moses::InputFileStream aFile(fileNameA);

  istream *eFileP = &eFile;
  istream *fFileP = &fFile;
  istream *aFileP = &aFile;

  istream *iwFileP = NULL;
  auto_ptr<Moses::InputFileStream> instanceWeightsFile;
  if (options.getInstanceWeightsFile().length()) {
    instanceWeightsFile.reset(new Moses::InputFileStream(options.getInstanceWeightsFile()));
    iwFileP = instanceWeightsFile.get();
  }

  // open output files
  if (options.isOrientationFlag()) {
    string fileNameExtractOrientation = fileNameExtract + ".o" + (options.isGzOutput()?".gz":"");
    extractFileOrientation.Open(fileNameExtractOrientation.c_str());
  }

  int i = sentenceOffset;

  string englishString, foreignString, alignmentString, weightString;

  while(getline(*eFileP, englishString)) {
    i++;

    getline(*eFileP, englishString);
    getline(*fFileP, foreignString);
    getline(*aFileP, alignmentString);
    if (iwFileP) {
      getline(*iwFileP, weightString);
    }

    if (i%10000 == 0) cerr << "." << flush;

    SentenceAlignment sentence;
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (options.isOnlyOutputSpanInfo()) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
    if (sentence.create( englishString.c_str(), foreignString.c_str(), alignmentString.c_str(), weightString.c_str(), i, false)) {
      ExtractTask *task = new ExtractTask(i-1, sentence, options, extractFileOrientation);
      task->Run();
      delete task;

    }
    if (options.isOnlyOutputSpanInfo()) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

  eFile.Close();
  fFile.Close();
  aFile.Close();

  //az: only close if we actually opened it
  if (!options.isOnlyOutputSpanInfo()) {
    if (options.isOrientationFlag()) {
      extractFileOrientation.Close();
    }
  }
}

namespace MosesTraining
{
void ExtractTask::Run()
{
  extract(m_sentence);
}

void ExtractTask::extract(SentenceAlignment &sentence)
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

  bool relaxLimit = m_options.isHierModel();
  bool buildExtraStructure = m_options.isPhraseModel() || m_options.isHierModel();

  // check alignments for target phrase startE...endE
  // loop over extracted phrases which are compatible with the word-alignments
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && (relaxLimit || endE<startE+m_options.maxPhraseLength));
        endE++) {

      int minF = std::numeric_limits<int>::max();
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
          (relaxLimit || maxF-minF < m_options.maxPhraseLength)) { // source phrase within limits

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
               (relaxLimit || startF>maxF-m_options.maxPhraseLength) && // within length limit
               (startF==minF || sentence.alignedCountS[startF]==0)); // unaligned
              startF--)
            // end point of source phrase may advance over unaligned
            for(int endF=maxF;
                (endF<countF &&
                 (relaxLimit || endF<startF+m_options.maxPhraseLength) && // within length limit
                 (endF - startF + 1 > m_options.minPhraseLength) && // within length limit
                 (endF==maxF || sentence.alignedCountS[endF]==0)); // unaligned
                endF++) { // at this point we have extracted a phrase
              if(buildExtraStructure) { // phrase || hier
                if(endE-startE < m_options.maxPhraseLength && endF-startF < m_options.maxPhraseLength) { // within limit
                  inboundPhrases.push_back(HPhrase(HPhraseVertex(startF,startE),
                                                   HPhraseVertex(endF,endE)));
                  insertPhraseVertices(inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                                       startF, startE, endF, endE);
                } else
                  insertPhraseVertices(outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                                       startF, startE, endF, endE);
              } else {
                string orientationInfo = "";
                if(m_options.isWordModel()) {
                  REO_POS wordPrevOrient, wordNextOrient;
                  bool connectedLeftTopP  = isAligned( sentence, startF-1, startE-1 );
                  bool connectedRightTopP = isAligned( sentence, endF+1,   startE-1 );
                  bool connectedLeftTopN  = isAligned( sentence, endF+1, endE+1 );
                  bool connectedRightTopN = isAligned( sentence, startF-1,   endE+1 );
                  wordPrevOrient = getOrientWordModel(sentence, m_options.isWordType(), connectedLeftTopP, connectedRightTopP, startF, endF, startE, endE, countF, 0, 1, &ge, &lt);
                  wordNextOrient = getOrientWordModel(sentence, m_options.isWordType(), connectedLeftTopN, connectedRightTopN, endF, startF, endE, startE, 0, countF, -1, &lt, &ge);
                  orientationInfo += getOrientString(wordPrevOrient, m_options.isWordType()) + " " + getOrientString(wordNextOrient, m_options.isWordType());
                  if(m_options.isAllModelsOutputFlag())
                    " | | ";
                }
                addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
              }
            }
        }
      }
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
    if(connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() &&
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
    if((connectedRightTop = (it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() &&
                            it->second.find(indexF) != it->second.end()) ||
        (connectedRightTop = (it = outBottomLeft.find(startE - unit)) != outBottomLeft.end() &&
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

int getClass(const std::string &str)
{
	size_t pos = str.find("swap");
	if (pos == str.npos) {
		return 0;
	}
	else if (pos == 0) {
		return 1;
	}
	else {
		return 2;
	}
}

void ExtractTask::addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientationInfo)
{
  if (m_options.isOnlyOutputSpanInfo()) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }

  const string &sep = m_options.separator;

  m_extractFileOrientation << sentence.sentenceID << " " << sep << " ";
  m_extractFileOrientation << getClass(orientationInfo) << " " << sep << " ";

  // position
  m_extractFileOrientation << startF << " " << endF << " " << sep << " ";

  // start
  m_extractFileOrientation << "<s> ";
  for(int fi=0; fi<startF; fi++) {
	  m_extractFileOrientation << sentence.source[fi] << " ";
  }
  m_extractFileOrientation << sep << " ";

  // middle
  for(int fi=startF; fi<=endF; fi++) {
	  m_extractFileOrientation << sentence.source[fi] << " ";
  }
  m_extractFileOrientation << sep << " ";

  // end
  for(int fi=endF+1; fi<sentence.source.size(); fi++) {
	  m_extractFileOrientation << sentence.source[fi] << " ";
  }
  m_extractFileOrientation << "</s> ";


  // target
  /*
  for(int ei=startE; ei<=endE; ei++) {
	  m_extractFileOrientation << sentence.target[ei] << " ";
  }
  */
  m_extractFileOrientation << endl;
}


/** tokenise input string to vector of string. each element has been separated by a character in the delimiters argument.
		The separator can only be 1 character long. The default delimiters are space or tab
*/
std::vector<std::string> Tokenize(const std::string& str,
                                  const std::string& delimiters)
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}

}
