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

#include "SafeGetline.h"
#include "SentenceAlignment.h"
#include "tables-core.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "PhraseExtractionOptions.h"

using namespace std;
using namespace MosesTraining;

namespace MosesTraining
{


const long int LINE_MAX_LENGTH = 500000 ;


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
  ExtractTask(size_t id, SentenceAlignment &sentence,PhraseExtractionOptions &initoptions, Moses::OutputFileStream &extractFile, Moses::OutputFileStream &extractFileInv,Moses::OutputFileStream &extractFileOrientation, Moses::OutputFileStream &extractFileContext, Moses::OutputFileStream &extractFileContextInv):
    m_sentence(sentence),
    m_options(initoptions),
    m_extractFile(extractFile),
    m_extractFileInv(extractFileInv),
    m_extractFileOrientation(extractFileOrientation),
    m_extractFileContext(extractFileContext),
    m_extractFileContextInv(extractFileContextInv) {}
  void Run();
private:
  vector< string > m_extractedPhrases;
  vector< string > m_extractedPhrasesInv;
  vector< string > m_extractedPhrasesOri;
  vector< string > m_extractedPhrasesSid;
  vector< string > m_extractedPhrasesContext;
  vector< string > m_extractedPhrasesContextInv;
  void extractBase(SentenceAlignment &);
  void extract(SentenceAlignment &);
  void addPhrase(SentenceAlignment &, int, int, int, int, string &);
  void writePhrasesToFile();
  bool checkPlaceholders (const SentenceAlignment &sentence, int startE, int endE, int startF, int endF);
  bool isPlaceholder(const string &word);

  SentenceAlignment &m_sentence;
  const PhraseExtractionOptions &m_options;
  Moses::OutputFileStream &m_extractFile;
  Moses::OutputFileStream &m_extractFileInv;
  Moses::OutputFileStream &m_extractFileOrientation;
  Moses::OutputFileStream &m_extractFileContext;
  Moses::OutputFileStream &m_extractFileContextInv;
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

  Moses::OutputFileStream extractFile;
  Moses::OutputFileStream extractFileInv;
  Moses::OutputFileStream extractFileOrientation;
  Moses::OutputFileStream extractFileContext;
  Moses::OutputFileStream extractFileContextInv;
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
    } else if (strcmp(argv[i], "--Placeholders") == 0) {
      ++i;
      string str = argv[i];
      options.placeholders = Tokenize(str.c_str(), ",");
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
  if (options.isTranslationFlag()) {
    string fileNameExtractInv = fileNameExtract + ".inv" + (options.isGzOutput()?".gz":"");
    extractFile.Open( (fileNameExtract + (options.isGzOutput()?".gz":"")).c_str());
    extractFileInv.Open(fileNameExtractInv.c_str());
  }
  if (options.isOrientationFlag()) {
    string fileNameExtractOrientation = fileNameExtract + ".o" + (options.isGzOutput()?".gz":"");
    extractFileOrientation.Open(fileNameExtractOrientation.c_str());
  }
  if (options.isFlexScoreFlag()) {
    string fileNameExtractContext = fileNameExtract + ".context"  + (options.isGzOutput()?".gz":"");
    string fileNameExtractContextInv = fileNameExtract + ".context.inv"  + (options.isGzOutput()?".gz":"");
    extractFileContext.Open(fileNameExtractContext.c_str());
    extractFileContextInv.Open(fileNameExtractContextInv.c_str());
  }

  int i = sentenceOffset;

  while(true) {
    i++;
    if (i%10000 == 0) cerr << "." << flush;
    char englishString[LINE_MAX_LENGTH];
    char foreignString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    char weightString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*eFileP), englishString, LINE_MAX_LENGTH, '\n', __FILE__);
    if (eFileP->eof()) break;
    SAFE_GETLINE((*fFileP), foreignString, LINE_MAX_LENGTH, '\n', __FILE__);
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n', __FILE__);
    if (iwFileP) {
      SAFE_GETLINE((*iwFileP), weightString, LINE_MAX_LENGTH, '\n', __FILE__);
    }
    SentenceAlignment sentence;
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (options.isOnlyOutputSpanInfo()) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
    if (sentence.create( englishString, foreignString, alignmentString, weightString, i, false)) {
      if (options.placeholders.size()) {
        sentence.invertAlignment();
      }
      ExtractTask *task = new ExtractTask(i-1, sentence, options, extractFile , extractFileInv, extractFileOrientation, extractFileContext, extractFileContextInv);
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
    if (options.isTranslationFlag()) {
      extractFile.Close();
      extractFileInv.Close();

    }
    if (options.isOrientationFlag()) {
      extractFileOrientation.Close();
    }

    if (options.isFlexScoreFlag()) {
      extractFileContext.Close();
      extractFileContextInv.Close();
    }
  }
}

namespace MosesTraining
{
void ExtractTask::Run()
{
  extract(m_sentence);
  writePhrasesToFile();
  m_extractedPhrases.clear();
  m_extractedPhrasesInv.clear();
  m_extractedPhrasesOri.clear();
  m_extractedPhrasesSid.clear();
  m_extractedPhrasesContext.clear();
  m_extractedPhrasesContextInv.clear();

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

      if(m_options.isWordModel()) {
        wordPrevOrient = getOrientWordModel(sentence, m_options.isWordType(),
                                            connectedLeftTopP, connectedRightTopP,
                                            startF, endF, startE, endE, countF, 0, 1,
                                            &ge, &lt);
        wordNextOrient = getOrientWordModel(sentence, m_options.isWordType(),
                                            connectedLeftTopN, connectedRightTopN,
                                            endF, startF, endE, startE, 0, countF, -1,
                                            &lt, &ge);
      }
      if (m_options.isPhraseModel()) {
        phrasePrevOrient = getOrientPhraseModel(sentence, m_options.isPhraseType(),
                                                connectedLeftTopP, connectedRightTopP,
                                                startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft);
        phraseNextOrient = getOrientPhraseModel(sentence, m_options.isPhraseType(),
                                                connectedLeftTopN, connectedRightTopN,
                                                endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight);
      } else {
        phrasePrevOrient = phraseNextOrient = UNKNOWN;
      }
      if(m_options.isHierModel()) {
        hierPrevOrient = getOrientHierModel(sentence, m_options.isHierType(),
                                            connectedLeftTopP, connectedRightTopP,
                                            startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft, outBottomRight, outBottomLeft, phrasePrevOrient);
        hierNextOrient = getOrientHierModel(sentence, m_options.isHierType(),
                                            connectedLeftTopN, connectedRightTopN,
                                            endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight, outBottomLeft, outBottomRight, phraseNextOrient);
      }

      orientationInfo = ((m_options.isWordModel())? getOrientString(wordPrevOrient, m_options.isWordType()) + " " + getOrientString(wordNextOrient, m_options.isWordType()) : "") + " | " +
                        ((m_options.isPhraseModel())? getOrientString(phrasePrevOrient, m_options.isPhraseType()) + " " + getOrientString(phraseNextOrient, m_options.isPhraseType()) : "") + " | " +
                        ((m_options.isHierModel())? getOrientString(hierPrevOrient, m_options.isHierType()) + " " + getOrientString(hierNextOrient, m_options.isHierType()) : "");

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

void ExtractTask::addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientationInfo)
{
  // source
  //   // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n";
  ostringstream outextractstr;
  ostringstream outextractstrInv;
  ostringstream outextractstrOrientation;

  if (m_options.isOnlyOutputSpanInfo()) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }

  if (m_options.placeholders.size() && !checkPlaceholders(sentence, startE, endE, startF, endF)) {
    return;
  }

  if (m_options.debug) {
      outextractstr << "sentenceID=" << sentence.sentenceID << " ";
      outextractstrInv << "sentenceID=" << sentence.sentenceID << " ";
      outextractstrOrientation << "sentenceID=" << sentence.sentenceID << " ";
  }

  for(int fi=startF; fi<=endF; fi++) {
    if (m_options.isTranslationFlag()) outextractstr << sentence.source[fi] << " ";
    if (m_options.isOrientationFlag()) outextractstrOrientation << sentence.source[fi] << " ";
  }
  if (m_options.isTranslationFlag()) outextractstr << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";

  // target
  for(int ei=startE; ei<=endE; ei++) {
    if (m_options.isTranslationFlag()) outextractstr << sentence.target[ei] << " ";
    if (m_options.isTranslationFlag()) outextractstrInv << sentence.target[ei] << " ";
    if (m_options.isOrientationFlag()) outextractstrOrientation << sentence.target[ei] << " ";
  }
  if (m_options.isTranslationFlag()) outextractstr << "|||";
  if (m_options.isTranslationFlag()) outextractstrInv << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";

  // source (for inverse)

  if (m_options.isTranslationFlag()) {
    for(int fi=startF; fi<=endF; fi++)
      outextractstrInv << sentence.source[fi] << " ";
    outextractstrInv << "|||";
  }

  // alignment
  if (m_options.isTranslationFlag()) {
    for(int ei=startE; ei<=endE; ei++) {
      for(unsigned int i=0; i<sentence.alignedToT[ei].size(); i++) {
        int fi = sentence.alignedToT[ei][i];
        outextractstr << " " << fi-startF << "-" << ei-startE;
        outextractstrInv << " " << ei-startE << "-" << fi-startF;
      }
    }
  }

  if (m_options.isOrientationFlag())
    outextractstrOrientation << orientationInfo;

  if (m_options.isIncludeSentenceIdFlag()) {
    outextractstr << " ||| " << sentence.sentenceID;
  }

  if (m_options.getInstanceWeightsFile().length()) {
    if (m_options.isTranslationFlag()) {
      outextractstr << " ||| " << sentence.weightString;
      outextractstrInv << " ||| " << sentence.weightString;
    }
    if (m_options.isOrientationFlag()) {
      outextractstrOrientation << " ||| " << sentence.weightString;
    }
  }



  // generate two lines for every extracted phrase:
  // once with left, once with right context
  if (m_options.isFlexScoreFlag()) {

    ostringstream outextractstrContext;
    ostringstream outextractstrContextInv;

    for(int fi=startF; fi<=endF; fi++) {
      outextractstrContext << sentence.source[fi] << " ";
    }
    outextractstrContext << "||| ";

    // target
    for(int ei=startE; ei<=endE; ei++) {
      outextractstrContext << sentence.target[ei] << " ";
      outextractstrContextInv << sentence.target[ei] << " ";
    }
    outextractstrContext << "||| ";
    outextractstrContextInv << "||| ";

    for(int fi=startF; fi<=endF; fi++)
      outextractstrContextInv << sentence.source[fi] << " ";

    outextractstrContextInv << "|||";

    string strContext = outextractstrContext.str();
    string strContextInv = outextractstrContextInv.str();

    ostringstream outextractstrContextRight(strContext, ostringstream::app);
    ostringstream outextractstrContextRightInv(strContextInv, ostringstream::app);

    // write context to left
    outextractstrContext << "< ";
    if (startF == 0) outextractstrContext << "<s>";
    else outextractstrContext << sentence.source[startF-1];

    outextractstrContextInv << " < ";
    if (startE == 0) outextractstrContextInv << "<s>";
    else outextractstrContextInv << sentence.target[startE-1];

    // write context to right
    outextractstrContextRight << "> ";
    if (endF+1 == sentence.source.size()) outextractstrContextRight << "<s>";
    else outextractstrContextRight << sentence.source[endF+1];

    outextractstrContextRightInv << " > ";
    if (endE+1 == sentence.target.size()) outextractstrContextRightInv << "<s>";
    else outextractstrContextRightInv << sentence.target[endE+1];

    outextractstrContext << "\n";
    outextractstrContextInv << "\n";
    outextractstrContextRight << "\n";
    outextractstrContextRightInv << "\n";

    m_extractedPhrasesContext.push_back(outextractstrContext.str());
    m_extractedPhrasesContextInv.push_back(outextractstrContextInv.str());
    m_extractedPhrasesContext.push_back(outextractstrContextRight.str());
    m_extractedPhrasesContextInv.push_back(outextractstrContextRightInv.str());
  }

  if (m_options.isTranslationFlag()) outextractstr << "\n";
  if (m_options.isTranslationFlag()) outextractstrInv << "\n";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "\n";


  m_extractedPhrases.push_back(outextractstr.str());
  m_extractedPhrasesInv.push_back(outextractstrInv.str());
  m_extractedPhrasesOri.push_back(outextractstrOrientation.str());
}


void ExtractTask::writePhrasesToFile()
{

  ostringstream outextractFile;
  ostringstream outextractFileInv;
  ostringstream outextractFileOrientation;
  ostringstream outextractFileContext;
  ostringstream outextractFileContextInv;

  for(vector<string>::const_iterator phrase=m_extractedPhrases.begin(); phrase!=m_extractedPhrases.end(); phrase++) {
    outextractFile<<phrase->data();
  }
  for(vector<string>::const_iterator phrase=m_extractedPhrasesInv.begin(); phrase!=m_extractedPhrasesInv.end(); phrase++) {
    outextractFileInv<<phrase->data();
  }
  for(vector<string>::const_iterator phrase=m_extractedPhrasesOri.begin(); phrase!=m_extractedPhrasesOri.end(); phrase++) {
    outextractFileOrientation<<phrase->data();
  }
  for(vector<string>::const_iterator phrase=m_extractedPhrasesContext.begin(); phrase!=m_extractedPhrasesContext.end(); phrase++) {
    outextractFileContext<<phrase->data();
  }
  for(vector<string>::const_iterator phrase=m_extractedPhrasesContextInv.begin(); phrase!=m_extractedPhrasesContextInv.end(); phrase++) {
    outextractFileContextInv<<phrase->data();
  }

  m_extractFile << outextractFile.str();
  m_extractFileInv  << outextractFileInv.str();
  m_extractFileOrientation << outextractFileOrientation.str();
  if (m_options.isFlexScoreFlag()) {
    m_extractFileContext  << outextractFileContext.str();
    m_extractFileContextInv << outextractFileContextInv.str();
  }
}

// if proper conditioning, we need the number of times a source phrase occured

void ExtractTask::extractBase( SentenceAlignment &sentence )
{
  ostringstream outextractFile;
  ostringstream outextractFileInv;

  int countF = sentence.source.size();
  for(int startF=0; startF<countF; startF++) {
    for(int endF=startF;
        (endF<countF && endF<startF+m_options.maxPhraseLength);
        endF++) {
      for(int fi=startF; fi<=endF; fi++) {
        outextractFile << sentence.source[fi] << " ";
      }
      outextractFile << "|||" << endl;
    }
  }

  int countE = sentence.target.size();
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && endE<startE+m_options.maxPhraseLength);
        endE++) {
      for(int ei=startE; ei<=endE; ei++) {
        outextractFileInv << sentence.target[ei] << " ";
      }
      outextractFileInv << "|||" << endl;
    }
  }
  m_extractFile << outextractFile.str();
  m_extractFileInv << outextractFileInv.str();

}


bool ExtractTask::checkPlaceholders (const SentenceAlignment &sentence, int startE, int endE, int startF, int endF)
{
  for (size_t pos = startF; pos <= endF; ++pos) {
    const string &sourceWord = sentence.source[pos];
    if (isPlaceholder(sourceWord)) {
      if (sentence.alignedToS.at(pos).size() != 1) {
        return false;
      } else {
        // check it actually lines up to another placeholder
        int targetPos = sentence.alignedToS.at(pos).at(0);
        const string &otherWord = sentence.target[targetPos];
        if (!isPlaceholder(otherWord)) {
          return false;
        }
      }
    }
  }

  for (size_t pos = startE; pos <= endE; ++pos) {
    const string &targetWord = sentence.target[pos];
    if (isPlaceholder(targetWord)) {
      if (sentence.alignedToT.at(pos).size() != 1) {
        return false;
      } else {
        // check it actually lines up to another placeholder
        int sourcePos = sentence.alignedToT.at(pos).at(0);
        const string &otherWord = sentence.source[sourcePos];
        if (!isPlaceholder(otherWord)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool ExtractTask::isPlaceholder(const string &word)
{
  for (size_t i = 0; i < m_options.placeholders.size(); ++i) {
    const string &placeholder = m_options.placeholders[i];
    if (word == placeholder) {
      return true;
    }
  }
  return false;
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
