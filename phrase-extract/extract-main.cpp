#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <limits>

#include "tables-core.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "PhraseExtractionOptions.h"
#include "SentenceAlignmentWithSyntax.h"
#include "SyntaxNode.h"
#include "moses/Util.h"

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

REO_POS getOrientWordModel(SentenceAlignmentWithSyntax &, REO_MODEL_TYPE, bool, bool,
                           int, int, int, int, int, int, int,
                           bool (*)(int, int), bool (*)(int, int));
REO_POS getOrientPhraseModel(SentenceAlignmentWithSyntax &, REO_MODEL_TYPE, bool, bool,
                             int, int, int, int, int, int, int,
                             bool (*)(int, int), bool (*)(int, int),
                             const HSentenceVertices &, const HSentenceVertices &);
REO_POS getOrientHierModel(SentenceAlignmentWithSyntax &, REO_MODEL_TYPE, bool, bool,
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

bool isAligned (SentenceAlignmentWithSyntax &, int, int);

int sentenceOffset = 0;


class ExtractTask
{
public:
  ExtractTask(
    size_t id, SentenceAlignmentWithSyntax &sentence,
    PhraseExtractionOptions &initoptions,
    Moses::OutputFileStream &extractFile,
    Moses::OutputFileStream &extractFileInv,
    Moses::OutputFileStream &extractFileOrientation,
    Moses::OutputFileStream &extractFileContext,
    Moses::OutputFileStream &extractFileContextInv):
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
  void extractBase();
  void extract();
  void addPhrase(int, int, int, int, const std::string &);
  void writePhrasesToFile();
  bool checkPlaceholders(int startE, int endE, int startF, int endF) const;
  bool isPlaceholder(const string &word) const;
  bool checkTargetConstituentBoundaries(int startE, int endE, int startF, int endF,
                                        ostringstream &outextractstrPhraseProperties) const;
  void getOrientationInfo(int startE, int endE, int startF, int endF,
                          const HSentenceVertices& inTopLeft,
                          const HSentenceVertices& inTopRight,
                          const HSentenceVertices& inBottomLeft,
                          const HSentenceVertices& inBottomRight,
                          const HSentenceVertices& outTopLeft,
                          const HSentenceVertices& outTopRight,
                          const HSentenceVertices& outBottomLeft,
                          const HSentenceVertices& outBottomRight,
                          std::string &orientationInfo) const;

  SentenceAlignmentWithSyntax &m_sentence;
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
  cerr	<< "PhraseExtract v1.5, written by Philipp Koehn et al." << std::endl
        << "phrase extraction from an aligned parallel corpus" << std::endl;

  if (argc < 6) {
    cerr << "syntax: extract en de align extract max-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ] ";
    cerr << "| --OnlyOutputSpanInfo | --NoTTable | --GZOutput | --IncludeSentenceId | --SentenceOffset n | --InstanceWeights filename ";
    cerr << "| --TargetConstituentConstrained | --TargetConstituentBoundaries ]" << std::endl;
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
    } else if (strcmp(argv[i],"--TargetConstituentConstrained") == 0) {
      options.initTargetConstituentConstrainedFlag(true);
    } else if (strcmp(argv[i],"--TargetConstituentBoundaries") == 0) {
      options.initTargetConstituentBoundariesFlag(true);
    } else if (strcmp(argv[i],"--FlexibilityScore") == 0) {
      options.initFlexScoreFlag(true);
    } else if (strcmp(argv[i],"--SingleWordHeuristic") == 0) {
      options.initSingleWordHeuristicFlag(true);
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
      Moses::Tokenize(options.placeholders, str.c_str(), ",");
    } else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'" << std::endl;
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

  // stats on labels for glue grammar and unknown word label probabilities
  set< string > targetLabelCollection, sourceLabelCollection;
  map< string, int > targetTopLabelCollection, sourceTopLabelCollection;
  const bool targetSyntax = true;

  int i = sentenceOffset;

  string englishString, foreignString, alignmentString, weightString;

  while (getline(*eFileP, englishString)) {
    // Print progress dots to stderr.
    i++;
    if (i%10000 == 0) cerr << "." << flush;

    getline(*fFileP, foreignString);
    getline(*aFileP, alignmentString);
    if (iwFileP) {
      getline(*iwFileP, weightString);
    }

    SentenceAlignmentWithSyntax sentence
    (targetLabelCollection, sourceLabelCollection,
     targetTopLabelCollection, sourceTopLabelCollection,
     targetSyntax, false);
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (options.isOnlyOutputSpanInfo()) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
    if (sentence.create( englishString.c_str(),
                         foreignString.c_str(),
                         alignmentString.c_str(),
                         weightString.c_str(),
                         i, false)) {
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

  // We've been printing progress dots to stderr.  End the line.
  cerr << endl;
}

namespace MosesTraining
{
void ExtractTask::Run()
{
  extract();
  writePhrasesToFile();
  m_extractedPhrases.clear();
  m_extractedPhrasesInv.clear();
  m_extractedPhrasesOri.clear();
  m_extractedPhrasesSid.clear();
  m_extractedPhrasesContext.clear();
  m_extractedPhrasesContextInv.clear();

}

void ExtractTask::extract()
{
  int countE = m_sentence.target.size();
  int countF = m_sentence.source.size();

  HPhraseVector inboundPhrases;

  HSentenceVertices inTopLeft;
  HSentenceVertices inTopRight;
  HSentenceVertices inBottomLeft;
  HSentenceVertices inBottomRight;

  HSentenceVertices outTopLeft;
  HSentenceVertices outTopRight;
  HSentenceVertices outBottomLeft;
  HSentenceVertices outBottomRight;

  bool relaxLimit = m_options.isHierModel();

  // check alignments for target phrase startE...endE
  // loop over extracted phrases which are compatible with the word-alignments
  for (int startE=0; startE<countE; startE++) {
    for (int endE=startE;
         (endE<countE && (relaxLimit || endE<startE+m_options.maxPhraseLength));
         endE++) {

      int minF = std::numeric_limits<int>::max();
      int maxF = -1;
      vector< int > usedF = m_sentence.alignedCountS;
      for (int ei=startE; ei<=endE; ei++) {
        for (size_t i=0; i<m_sentence.alignedToT[ei].size(); i++) {
          int fi = m_sentence.alignedToT[ei][i];
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
        for (int fi=minF; fi<=maxF && !out_of_bounds; fi++)
          if (usedF[fi]>0) {
            // cout << "ouf of bounds: " << fi << std::endl;
            out_of_bounds = true;
          }

        // cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")" << std::endl;
        if (!out_of_bounds) {
          // start point of source phrase may retreat over unaligned
          for (int startF=minF;
               (startF>=0 &&
                (relaxLimit || startF>maxF-m_options.maxPhraseLength) && // within length limit
                (startF==minF || m_sentence.alignedCountS[startF]==0)); // unaligned
               startF--) {
            // end point of source phrase may advance over unaligned
            for (int endF=maxF;
                 (endF<countF &&
                  (relaxLimit || endF<startF+m_options.maxPhraseLength) && // within length limit
                  (endF==maxF || m_sentence.alignedCountS[endF]==0)); // unaligned
                 endF++) { // at this point we have extracted a phrase

              if(endE-startE < m_options.maxPhraseLength && endF-startF < m_options.maxPhraseLength) { // within limit
                inboundPhrases.push_back(HPhrase(HPhraseVertex(startF,startE),
                                                 HPhraseVertex(endF,endE)));
                insertPhraseVertices(inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                                     startF, startE, endF, endE);
              } else {
                insertPhraseVertices(outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                                     startF, startE, endF, endE);
              }
            }
          }
        }
      }
    }
  }

  std::string orientationInfo = "";

  for (size_t i = 0; i < inboundPhrases.size(); i++) {

    int startF = inboundPhrases[i].first.first;
    int startE = inboundPhrases[i].first.second;
    int endF = inboundPhrases[i].second.first;
    int endE = inboundPhrases[i].second.second;

    getOrientationInfo(startE, endE, startF, endF,
                       inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                       outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                       orientationInfo);

    addPhrase(startE, endE, startF, endF, orientationInfo);
  }

  if (m_options.isSingleWordHeuristicFlag()) {
    // add single word phrases that are not consistent with the word alignment
    m_sentence.invertAlignment();
    for (int ei=0; ei<countE; ei++) {
      for (size_t i=0; i<m_sentence.alignedToT[ei].size(); i++) {
        int fi = m_sentence.alignedToT[ei][i];
        if ((m_sentence.alignedToT[ei].size() > 1) || (m_sentence.alignedToS[fi].size() > 1)) {

          if (m_options.isOrientationFlag()) {
            getOrientationInfo(ei, ei, fi, fi,
                               inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                               outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                               orientationInfo);
          }

          addPhrase(ei, ei, fi, fi, orientationInfo);
        }
      }
    }
  }
}

void ExtractTask::getOrientationInfo(int startE, int endE, int startF, int endF,
                                     const HSentenceVertices& inTopLeft,
                                     const HSentenceVertices& inTopRight,
                                     const HSentenceVertices& inBottomLeft,
                                     const HSentenceVertices& inBottomRight,
                                     const HSentenceVertices& outTopLeft,
                                     const HSentenceVertices& outTopRight,
                                     const HSentenceVertices& outBottomLeft,
                                     const HSentenceVertices& outBottomRight,
                                     std::string &orientationInfo) const
{
  REO_POS wordPrevOrient=UNKNOWN, wordNextOrient=UNKNOWN;
  REO_POS phrasePrevOrient=UNKNOWN, phraseNextOrient=UNKNOWN;
  REO_POS hierPrevOrient=UNKNOWN, hierNextOrient=UNKNOWN;

  bool connectedLeftTopP  = isAligned( m_sentence, startF-1, startE-1 );
  bool connectedRightTopP = isAligned( m_sentence, endF+1,   startE-1 );
  bool connectedLeftTopN  = isAligned( m_sentence, endF+1, endE+1 );
  bool connectedRightTopN = isAligned( m_sentence, startF-1,   endE+1 );

  const int countF = m_sentence.source.size();

  if (m_options.isWordModel()) {
    wordPrevOrient = getOrientWordModel(m_sentence, m_options.isWordType(),
                                        connectedLeftTopP, connectedRightTopP,
                                        startF, endF, startE, endE, countF, 0, 1,
                                        &ge, &lt);
    wordNextOrient = getOrientWordModel(m_sentence, m_options.isWordType(),
                                        connectedLeftTopN, connectedRightTopN,
                                        endF, startF, endE, startE, 0, countF, -1,
                                        &lt, &ge);
  }
  if (m_options.isPhraseModel()) {
    phrasePrevOrient = getOrientPhraseModel(m_sentence, m_options.isPhraseType(),
                                            connectedLeftTopP, connectedRightTopP,
                                            startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft);
    phraseNextOrient = getOrientPhraseModel(m_sentence, m_options.isPhraseType(),
                                            connectedLeftTopN, connectedRightTopN,
                                            endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight);
  }
  if (m_options.isHierModel()) {
    hierPrevOrient = getOrientHierModel(m_sentence, m_options.isHierType(),
                                        connectedLeftTopP, connectedRightTopP,
                                        startF, endF, startE, endE, countF-1, 0, 1, &ge, &lt, inBottomRight, inBottomLeft, outBottomRight, outBottomLeft, phrasePrevOrient);
    hierNextOrient = getOrientHierModel(m_sentence, m_options.isHierType(),
                                        connectedLeftTopN, connectedRightTopN,
                                        endF, startF, endE, startE, 0, countF-1, -1, &lt, &ge, inBottomLeft, inBottomRight, outBottomLeft, outBottomRight, phraseNextOrient);
  }

  if (m_options.isWordModel()) {
    orientationInfo = getOrientString(wordPrevOrient, m_options.isWordType()) + " " + getOrientString(wordNextOrient, m_options.isWordType());
  } else {
    orientationInfo = " | " +
                      ((m_options.isPhraseModel())? getOrientString(phrasePrevOrient, m_options.isPhraseType()) + " " + getOrientString(phraseNextOrient, m_options.isPhraseType()) : "") + " | " +
                      ((m_options.isHierModel())? getOrientString(hierPrevOrient, m_options.isHierType()) + " " + getOrientString(hierNextOrient, m_options.isHierType()) : "");
  }
}


REO_POS getOrientWordModel(SentenceAlignmentWithSyntax & sentence, REO_MODEL_TYPE modelType,
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
REO_POS getOrientPhraseModel (SentenceAlignmentWithSyntax & sentence, REO_MODEL_TYPE modelType,
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
    if ((connectedLeftTop = ((it = inBottomRight.find(startE - unit)) != inBottomRight.end() &&
                             it->second.find(indexF) != it->second.end())))
      return DRIGHT;
  connectedRightTop = false;
  for(int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit)
    if ((connectedRightTop = ((it = inBottomLeft.find(startE - unit)) != inBottomLeft.end() &&
                              it->second.find(indexF) != it->second.end())))
      return DLEFT;
  return UNKNOWN;
}

// to be called with countF-1 instead of countF
REO_POS getOrientHierModel (SentenceAlignmentWithSyntax & sentence, REO_MODEL_TYPE modelType,
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

bool isAligned ( SentenceAlignmentWithSyntax &sentence, int fi, int ei )
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
  if (ret.second == false) {
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


bool ExtractTask::checkTargetConstituentBoundaries(int startE, int endE, int startF, int endF,
    ostringstream &outextractstrPhraseProperties) const
{
  if (m_options.isTargetConstituentBoundariesFlag()) {
    outextractstrPhraseProperties << " {{TargetConstituentBoundariesLeft ";
  }

  bool validTargetConstituentBoundaries = false;
  bool outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = true;

  if (m_options.isTargetConstituentBoundariesFlag()) {
    if (startE==0) {
      outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = false;
      outextractstrPhraseProperties << "BOS_";
    }
  }

  if (!m_sentence.targetTree.HasNodeStartingAtPosition(startE)) {

    validTargetConstituentBoundaries = false;

  } else {

    const std::vector< SyntaxNode* >& startingNodes = m_sentence.targetTree.GetNodesByStartPosition(startE);
    for ( std::vector< SyntaxNode* >::const_reverse_iterator iter = startingNodes.rbegin(); iter != startingNodes.rend(); ++iter ) {
      if ( (*iter)->end == endE ) {
        validTargetConstituentBoundaries = true;
        if (!m_options.isTargetConstituentBoundariesFlag()) {
          break;
        }
      }
      if (m_options.isTargetConstituentBoundariesFlag()) {
        if (outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst) {
          outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = false;
        } else {
          outextractstrPhraseProperties << "<";
        }
        outextractstrPhraseProperties << (*iter)->label;
      }
    }
  }

  if (m_options.isTargetConstituentBoundariesFlag()) {
    if (outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst) {
      outextractstrPhraseProperties << "<";
    }
    outextractstrPhraseProperties << "}}";
  }


  if (m_options.isTargetConstituentConstrainedFlag() && !validTargetConstituentBoundaries) {
    // skip over all boundary punctuation and check again
    bool relaxedValidTargetConstituentBoundaries = false;
    int relaxedStartE = startE;
    int relaxedEndE = endE;
    const std::string punctuation = ",;.:!?";
    while ( (relaxedStartE < endE) &&
            (m_sentence.target[relaxedStartE].size() == 1) &&
            (punctuation.find(m_sentence.target[relaxedStartE].at(0)) != std::string::npos) ) {
      ++relaxedStartE;
    }
    while ( (relaxedEndE > relaxedStartE) &&
            (m_sentence.target[relaxedEndE].size() == 1) &&
            (punctuation.find(m_sentence.target[relaxedEndE].at(0)) != std::string::npos) ) {
      --relaxedEndE;
    }

    if ( (relaxedStartE != startE) || (relaxedEndE !=endE) ) {
      const std::vector< SyntaxNode* >& startingNodes = m_sentence.targetTree.GetNodesByStartPosition(relaxedStartE);
      for ( std::vector< SyntaxNode* >::const_reverse_iterator iter = startingNodes.rbegin();
            (iter != startingNodes.rend() && !relaxedValidTargetConstituentBoundaries);
            ++iter ) {
        if ( (*iter)->end == relaxedEndE ) {
          relaxedValidTargetConstituentBoundaries = true;
        }
      }
    }

    if (!relaxedValidTargetConstituentBoundaries) {
      return false;
    }
  }


  if (m_options.isTargetConstituentBoundariesFlag()) {

    outextractstrPhraseProperties << " {{TargetConstituentBoundariesRightAdjacent ";
    outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = true;

    if (endE==(int)m_sentence.target.size()-1) {

      outextractstrPhraseProperties << "EOS_";
      outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = false;

    } else {

      const std::vector< SyntaxNode* >& adjacentNodes = m_sentence.targetTree.GetNodesByStartPosition(endE+1);
      for ( std::vector< SyntaxNode* >::const_reverse_iterator iter = adjacentNodes.rbegin(); iter != adjacentNodes.rend(); ++iter ) {
        if (outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst) {
          outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst = false;
        } else {
          outextractstrPhraseProperties << "<";
        }
        outextractstrPhraseProperties << (*iter)->label;
      }
    }

    if (outextractstrPhrasePropertyTargetConstituentBoundariesIsFirst) {
      outextractstrPhraseProperties << "<";
    }
    outextractstrPhraseProperties << "}}";
  }

  return true;
}


void ExtractTask::addPhrase( int startE, int endE, int startF, int endF,
                             const std::string &orientationInfo)
{
  ostringstream outextractstrPhraseProperties;
  if (m_options.isTargetConstituentBoundariesFlag() || m_options.isTargetConstituentConstrainedFlag()) {
    bool isTargetConstituentCovered = checkTargetConstituentBoundaries(startE, endE, startF, endF, outextractstrPhraseProperties);
    if (m_options.isTargetConstituentBoundariesFlag() && !isTargetConstituentCovered) {
      return;
    }
  }

  if (m_options.placeholders.size() && !checkPlaceholders(startE, endE, startF, endF)) {
    return;
  }

  if (m_options.isOnlyOutputSpanInfo()) {
    cout << startF << " " << endF << " " << startE << " " << endE << std::endl;
    return;
  }

  ostringstream outextractstr;
  ostringstream outextractstrInv;
  ostringstream outextractstrOrientation;

  if (m_options.debug) {
    outextractstr << "sentenceID=" << m_sentence.sentenceID << " ";
    outextractstrInv << "sentenceID=" << m_sentence.sentenceID << " ";
    outextractstrOrientation << "sentenceID=" << m_sentence.sentenceID << " ";
  }

  // source
  for(int fi=startF; fi<=endF; fi++) {
    if (m_options.isTranslationFlag()) outextractstr << m_sentence.source[fi] << " ";
    if (m_options.isOrientationFlag()) outextractstrOrientation << m_sentence.source[fi] << " ";
  }
  if (m_options.isTranslationFlag()) outextractstr << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";


  // target
  for(int ei=startE; ei<=endE; ei++) {

    if (m_options.isTranslationFlag()) {
      outextractstr << m_sentence.target[ei] << " ";
      outextractstrInv << m_sentence.target[ei] << " ";
    }

    if (m_options.isOrientationFlag()) {
      outextractstrOrientation << m_sentence.target[ei] << " ";
    }
  }
  if (m_options.isTranslationFlag()) outextractstr << "|||";
  if (m_options.isTranslationFlag()) outextractstrInv << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";

  // source (for inverse)

  if (m_options.isTranslationFlag()) {
    for(int fi=startF; fi<=endF; fi++)
      outextractstrInv << m_sentence.source[fi] << " ";
    outextractstrInv << "|||";
  }

  // alignment
  if (m_options.isTranslationFlag()) {
    if (m_options.isSingleWordHeuristicFlag() && (startE==endE) && (startF==endF)) {
      outextractstr << " 0-0";
      outextractstrInv << " 0-0";
    } else {
      for(int ei=startE; ei<=endE; ei++) {
        for(unsigned int i=0; i<m_sentence.alignedToT[ei].size(); i++) {
          int fi = m_sentence.alignedToT[ei][i];
          outextractstr << " " << fi-startF << "-" << ei-startE;
          outextractstrInv << " " << ei-startE << "-" << fi-startF;
        }
      }
    }
  }

  if (m_options.isOrientationFlag())
    outextractstrOrientation << orientationInfo;

  if (m_options.isIncludeSentenceIdFlag()) {
    outextractstr << " ||| " << m_sentence.sentenceID;
  }

  if (m_options.getInstanceWeightsFile().length()) {
    if (m_options.isTranslationFlag()) {
      outextractstr << " ||| " << m_sentence.weightString;
      outextractstrInv << " ||| " << m_sentence.weightString;
    }
    if (m_options.isOrientationFlag()) {
      outextractstrOrientation << " ||| " << m_sentence.weightString;
    }
  }

  outextractstr << outextractstrPhraseProperties.str();

  // generate two lines for every extracted phrase:
  // once with left, once with right context
  if (m_options.isFlexScoreFlag()) {

    ostringstream outextractstrContext;
    ostringstream outextractstrContextInv;

    for(int fi=startF; fi<=endF; fi++) {
      outextractstrContext << m_sentence.source[fi] << " ";
    }
    outextractstrContext << "||| ";

    // target
    for(int ei=startE; ei<=endE; ei++) {
      outextractstrContext << m_sentence.target[ei] << " ";
      outextractstrContextInv << m_sentence.target[ei] << " ";
    }
    outextractstrContext << "||| ";
    outextractstrContextInv << "||| ";

    for(int fi=startF; fi<=endF; fi++)
      outextractstrContextInv << m_sentence.source[fi] << " ";

    outextractstrContextInv << "|||";

    string strContext = outextractstrContext.str();
    string strContextInv = outextractstrContextInv.str();

    ostringstream outextractstrContextRight(strContext, ostringstream::app);
    ostringstream outextractstrContextRightInv(strContextInv, ostringstream::app);

    // write context to left
    outextractstrContext << "< ";
    if (startF == 0) outextractstrContext << "<s>";
    else outextractstrContext << m_sentence.source[startF-1];

    outextractstrContextInv << " < ";
    if (startE == 0) outextractstrContextInv << "<s>";
    else outextractstrContextInv << m_sentence.target[startE-1];

    // write context to right
    outextractstrContextRight << "> ";
    if (endF+1 == (int)m_sentence.source.size()) outextractstrContextRight << "<s>";
    else outextractstrContextRight << m_sentence.source[endF+1];

    outextractstrContextRightInv << " > ";
    if (endE+1 == (int)m_sentence.target.size()) outextractstrContextRightInv << "<s>";
    else outextractstrContextRightInv << m_sentence.target[endE+1];

    outextractstrContext << std::endl;
    outextractstrContextInv << std::endl;
    outextractstrContextRight << std::endl;
    outextractstrContextRightInv << std::endl;

    m_extractedPhrasesContext.push_back(outextractstrContext.str());
    m_extractedPhrasesContextInv.push_back(outextractstrContextInv.str());
    m_extractedPhrasesContext.push_back(outextractstrContextRight.str());
    m_extractedPhrasesContextInv.push_back(outextractstrContextRightInv.str());
  }

  if (m_options.isTranslationFlag()) outextractstr << std::endl;
  if (m_options.isTranslationFlag()) outextractstrInv << std::endl;
  if (m_options.isOrientationFlag()) outextractstrOrientation << std::endl;


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

void ExtractTask::extractBase()
{
  ostringstream outextractFile;
  ostringstream outextractFileInv;

  int countF = m_sentence.source.size();
  for(int startF=0; startF<countF; startF++) {
    for(int endF=startF;
        (endF<countF && endF<startF+m_options.maxPhraseLength);
        endF++) {
      for(int fi=startF; fi<=endF; fi++) {
        outextractFile << m_sentence.source[fi] << " ";
      }
      outextractFile << "|||" << endl;
    }
  }

  int countE = m_sentence.target.size();
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && endE<startE+m_options.maxPhraseLength);
        endE++) {
      for(int ei=startE; ei<=endE; ei++) {
        outextractFileInv << m_sentence.target[ei] << " ";
      }
      outextractFileInv << "|||" << endl;
    }
  }
  m_extractFile << outextractFile.str();
  m_extractFileInv << outextractFileInv.str();

}


bool ExtractTask::checkPlaceholders(int startE, int endE, int startF, int endF) const
{
  for (int pos = startF; pos <= endF; ++pos) {
    const string &sourceWord = m_sentence.source[pos];
    if (isPlaceholder(sourceWord)) {
      if (m_sentence.alignedToS.at(pos).size() != 1) {
        return false;
      } else {
        // check it actually lines up to another placeholder
        int targetPos = m_sentence.alignedToS.at(pos).at(0);
        const string &otherWord = m_sentence.target[targetPos];
        if (!isPlaceholder(otherWord)) {
          return false;
        }
      }
    }
  }

  for (int pos = startE; pos <= endE; ++pos) {
    const string &targetWord = m_sentence.target[pos];
    if (isPlaceholder(targetWord)) {
      if (m_sentence.alignedToT.at(pos).size() != 1) {
        return false;
      } else {
        // check it actually lines up to another placeholder
        int sourcePos = m_sentence.alignedToT.at(pos).at(0);
        const string &otherWord = m_sentence.source[sourcePos];
        if (!isPlaceholder(otherWord)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool ExtractTask::isPlaceholder(const string &word) const
{
  for (size_t i = 0; i < m_options.placeholders.size(); ++i) {
    const string &placeholder = m_options.placeholders[i];
    if (word == placeholder) {
      return true;
    }
  }
  return false;
}

}
