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
#include <boost/version.hpp>

#include "SafeGetline.h"
#include "SentenceAlignment.h"
#include "tables-core.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "../moses/src/ThreadPool.h"
#include "../moses/src/OutputCollector.h"
#include "PhraseExtractionOptions.h"
using namespace std;
using namespace MosesTraining;

namespace MosesTraining {


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

}
namespace MosesTraining{
class ExtractTask : public Moses::Task{
        private:
        size_t m_id;
        SentenceAlignment *m_sentence;
        PhraseExtractionOptions &m_options;
        Moses::OutputCollector* m_extractCollector;
        Moses::OutputCollector* m_extractCollectorInv;
        Moses::OutputCollector* m_extractCollectorOrientation;
        Moses::OutputCollector* m_extractCollectorSentenceId;
        Moses::OutputCollector* m_extractCollectorPsd;
public:
  ExtractTask(size_t id, SentenceAlignment *sentence,PhraseExtractionOptions &initoptions, Moses::OutputCollector *extractCollector, Moses::OutputCollector *extractCollectorInv,Moses::OutputCollector *extractCollectorOrientation,Moses::OutputCollector* extractCollectorSentenceId,Moses::OutputCollector* extractCollectorPsd  ):
    m_id(id),
    m_sentence(sentence),
    m_options(initoptions),
    m_extractCollector(extractCollector),
    m_extractCollectorInv(extractCollectorInv),
    m_extractCollectorOrientation(extractCollectorOrientation),
    m_extractCollectorSentenceId(extractCollectorSentenceId),
    m_extractCollectorPsd(extractCollectorPsd)
  {}
  ~ExtractTask() { delete m_sentence; }
void Run();
private:
  vector< string > m_extractedPhrases;
  vector< string > m_extractedPhrasesInv;
  vector< string > m_extractedPhrasesOri;
  vector< string > m_extractedPhrasesSid;
  vector< string > m_extractedPsdOutput;
  void extractBase(SentenceAlignment &);
  void extract(SentenceAlignment &);
  void extractAllMaxSize(SentenceAlignment &, int, int, HPhraseVector &,
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, 
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, bool);
  void extractMTU(SentenceAlignment &, int, int, HPhraseVector &,
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, 
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, bool);
  bool getAlignedSpan( int, int, int *, int *, vector< int >, vector<vector<int> > & );
  void extractPhrase(SentenceAlignment &, int, int, int, int, int, HPhraseVector &,
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &,
    HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, HSentenceVertices &, bool);
  void addPhrase(SentenceAlignment &, int, int, int, int, string &);
  void writePhrasesToFile();
};
}

int main(int argc, char* argv[])
{
  cerr	<< "PhraseExtract v1.4, written by Philipp Koehn\n"
        << "phrase extraction from an aligned parallel corpus\n";
#ifdef WITH_THREADS
  int thread_count = 1;
#endif
  cerr << "Boost version: " << BOOST_LIB_VERSION << endl;
 if (argc < 6) {
    cerr << "syntax: extract en de align extract max-length [orientation [ --model [wbe|phrase|hier]-[msd|mslr|mono] ] ";
    #ifdef WITH_THREADS

    cerr<< "| --threads NUM ";
    #endif
    cerr<<"| --OnlyOutputSpanInfo | --OutputPsdInfo | --NoTTable | --SentenceId | --GZOutput ]\n";
    exit(1);
  }

  Moses::OutputFileStream extractFilePsd;
  Moses::OutputFileStream extractFile;
  Moses::OutputFileStream extractFileInv;
  Moses::OutputFileStream extractFileOrientation;
  Moses::OutputFileStream extractFileSentenceId;
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
    } else if (strcmp(argv[i],"--NoTTable") == 0) {
      options.initTranslationFlag(false);
    } else if (strcmp(argv[i],"--OutputPsdInfo") == 0) {
      options.initOutputPsd(true);
    } else if (strcmp(argv[i], "--SentenceId") == 0) {
      options.initSentenceIdFlag(true);  
    } else if (strcmp(argv[i], "--GZOutput") == 0) {
      options.initGzOutput(true);  
    } else if (strcmp(argv[i], "--MTU") == 0) { // minimal translation units
      options.initMTU(true);
    } else if(strcmp(argv[i],"--model") == 0) {
      if (i+1 >= argc) {
        cerr << "extract: syntax error, no model's information provided to the option --model " << endl;
        exit(1);
      }
      char*  modelParams = argv[++i];
      char*  modelName = strtok(modelParams, "-");
      char*  modelType = strtok(NULL, "-");

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
 #ifdef WITH_THREADS
    }else if (strcmp(argv[i],"-threads") == 0 ||
               strcmp(argv[i],"--threads") == 0 ||
               strcmp(argv[i],"--Threads") == 0) {
        if(argc>(i+1))thread_count = atoi(argv[++i]);
        else {cerr<<"extract: syntax error, NUM is missing for --threads NUM option"<<endl;
        exit(1);
        }
        if(thread_count==0){
                cerr<<"extract: error, NUM is missing for --threads NUM option or --threads 0 is given"<<endl;
                exit(1);
        }
     #endif

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

  if (options.isSentenceIdFlag()) {
    string fileNameExtractSentenceId = fileNameExtract + ".sid" + (options.isGzOutput()?".gz":"");
    extractFileSentenceId.Open(fileNameExtractSentenceId.c_str());
  }

  if (options.isOutputPsd()) {
    string fileNameExtractPsd = fileNameExtract + ".psd" + (options.isGzOutput()?".gz":"");
    extractFilePsd.Open(fileNameExtractPsd.c_str());
  }

    Moses::OutputCollector* extractCollector = new Moses::OutputCollector(&extractFile);//r
    Moses::OutputCollector* extractCollectorInv = new Moses::OutputCollector(&extractFileInv);//r
    Moses::OutputCollector* extractCollectorOrientation = new Moses::OutputCollector(&extractFileOrientation);//r
    Moses::OutputCollector* extractCollectorSentenceId = new Moses::OutputCollector(&extractFileSentenceId); //r
    Moses::OutputCollector* extractCollectorPsd = new Moses::OutputCollector(&extractFilePsd); //r
#ifdef WITH_THREADS
  // set up thread pool
     Moses::ThreadPool pool(thread_count);
     pool.SetQueueLimit(1000);
#endif

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
    SentenceAlignment *sentence=new SentenceAlignment;
	// cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (options.isOnlyOutputSpanInfo()) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
	if (sentence->create( englishString, foreignString, alignmentString, i)) {
   	ExtractTask *task = new ExtractTask(i-1, sentence, options, extractCollector , extractCollectorInv, extractCollectorOrientation, extractCollectorSentenceId, extractCollectorPsd);
#ifdef WITH_THREADS
      if (thread_count == 1) {
        task->Run();
        delete task;
      }
      else {
        pool.Submit(task);
      }
#else
      task->Run();
      delete task;
#endif

    }
    if (options.isOnlyOutputSpanInfo()) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

#ifdef WITH_THREADS
  // wait for all threads to finish
  pool.Stop(true);
#endif

  eFile.Close();
  fFile.Close();
  aFile.Close();
      delete extractCollector;
      delete extractCollectorInv;
      delete extractCollectorOrientation;
      delete extractCollectorSentenceId;
      delete extractCollectorPsd;
  //az: only close if we actually opened it
  if (!options.isOnlyOutputSpanInfo()) {
    if (options.isTranslationFlag()) {
      extractFile.Close();
      extractFileInv.Close();
      
    }
    if (options.isOrientationFlag()){ 
	extractFileOrientation.Close();
	}
    if (options.isSentenceIdFlag()) {
      extractFileSentenceId.Close();
    }
    if (options.isOutputPsd()){
      extractFilePsd.Close();
    }
  }
}

namespace MosesTraining
{
void ExtractTask::Run() {
  extract(*m_sentence);
  writePhrasesToFile();
  m_extractedPhrases.clear();
  m_extractedPhrasesInv.clear();
  m_extractedPhrasesOri.clear();
  m_extractedPhrasesSid.clear();
  m_extractedPsdOutput.clear();
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

  bool buildExtraStructure = m_options.isPhraseModel() || m_options.isHierModel();

  if (m_options.isMTU()) {
    extractMTU( sentence, countE, countF, inboundPhrases,
                inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                buildExtraStructure);
  }
  else {
    extractAllMaxSize( sentence, countE, countF, inboundPhrases,
                       inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                       outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                       buildExtraStructure);
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

void ExtractTask::extractMTU(SentenceAlignment &sentence,
  int countE, int countF,
  HPhraseVector &inboundPhrases,
  HSentenceVertices &inTopLeft, HSentenceVertices &inTopRight, 
  HSentenceVertices &inBottomLeft, HSentenceVertices &inBottomRight,
  HSentenceVertices &outTopLeft, HSentenceVertices &outTopRight, 
  HSentenceVertices &outBottomLeft, HSentenceVertices &outBottomRight,
  bool buildExtraStructure)
{
  bool relaxLimit = m_options.isHierModel();

  // some handy data structures needed only for MTUs
  vector<int> alignedCountT;
  vector<vector<int> > alignedToS(countF);
  for(int ei=0; ei<countE; ei++) {
    alignedCountT.push_back(0);
    for(size_t i=0; i<sentence.alignedToT[ei].size(); i++) {
      alignedToS[sentence.alignedToT[ei][i]].push_back(ei);
      alignedCountT[ei]++;
    }
  }
  set< pair<int,int> > processedSpan;

  // try to find MTU for each English word
  for(int ei=0; ei<countE; ei++) {
    if (alignedCountT[ei] > 0) { // handle non-aligned words elsewhere
      // transitive closure that contains ei
      int minF, maxF;
      int minE = ei;
      int maxE = ei;
      bool out_of_bounds = false;
      do {
        out_of_bounds = getAlignedSpan( minE, maxE, &minF, &maxF, sentence.alignedCountS, sentence.alignedToT);
        if (!out_of_bounds || // transitive closure complete?
            (!relaxLimit && maxF-minF >= m_options.maxPhraseLength)) { // aligned phrase too big?
          break;
        }  
        out_of_bounds = getAlignedSpan( minF, maxF, &minE, &maxE, alignedCountT, alignedToS);
      } 
      while( out_of_bounds && maxE-minE < m_options.maxPhraseLength );

      // found transitive closure that is not too big
      if (maxE-minE < m_options.maxPhraseLength &&
          (relaxLimit || maxF-minF < m_options.maxPhraseLength)) {

        // check if this was already processed
        pair< int, int> span = make_pair( minE, maxE );
        if (!processedSpan.count( span )) {
         processedSpan.insert( span );

         // expand by null aligned words in all directions
         for( int startE = minE; startE>=0 && (startE==minE || alignedCountT[startE] == 0); startE--) {
          for( int endE = maxE; endE<countE && (endE==maxE || alignedCountT[endE] == 0) && endE-startE < m_options.maxPhraseLength; endE++) {
            for( int startF = minF; startF>=0 && (startF==minF || sentence.alignedCountS[startF] == 0); startF--) {
              for( int endF = maxF; endF<countF && (endF==maxF || sentence.alignedCountS[endF] == 0) && (relaxLimit || endF-startF < m_options.maxPhraseLength); endF++) {
                // extract phrase pair
                extractPhrase( sentence, startE, endE, startF, endF, countF, inboundPhrases,
                               inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                               outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                               buildExtraStructure);
              }
            }
          }
         }
        }
      }
    }
  }
}

void ExtractTask::extractAllMaxSize(SentenceAlignment &sentence,
  int countE, int countF,
  HPhraseVector &inboundPhrases,
  HSentenceVertices &inTopLeft, HSentenceVertices &inTopRight, 
  HSentenceVertices &inBottomLeft, HSentenceVertices &inBottomRight,
  HSentenceVertices &outTopLeft, HSentenceVertices &outTopRight, 
  HSentenceVertices &outBottomLeft, HSentenceVertices &outBottomRight,
  bool buildExtraStructure)
{
  bool relaxLimit = m_options.isHierModel();
  // check alignments for target phrase startE...endE
  // loop over extracted phrases which are compatible with the word-alignments
  for(int startE=0; startE<countE; startE++) {
    for(int endE=startE;
        (endE<countE && (relaxLimit || endE<startE+m_options.maxPhraseLength));
        endE++) {

      int minF, maxF;
      bool out_of_bounds = getAlignedSpan( startE, endE, &minF, &maxF, sentence.alignedCountS, sentence.alignedToT);
      /* int minF = 9999;
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
      } */

      if (maxF >= 0 && // aligned to any source words at all
          (relaxLimit || maxF-minF < m_options.maxPhraseLength)) { // source phrase within limits

        // check if source words are aligned to out of bound target words
        /* bool out_of_bounds = false;
        for(int fi=minF; fi<=maxF && !out_of_bounds; fi++) {
          if (usedF[fi]>0) {
            // cout << "ouf of bounds: " << fi << "\n";
            out_of_bounds = true;
          }
        } */

        // cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n";
        if (!out_of_bounds) {
          // start point of source phrase may retreat over unaligned
          for(int startF=minF;
              (startF>=0 &&
               (relaxLimit || startF>maxF-m_options.maxPhraseLength) && // within length limit
               (startF==minF || sentence.alignedCountS[startF]==0)); // unaligned
              startF--) {
            // end point of source phrase may advance over unaligned
            for(int endF=maxF;
                (endF<countF &&
                 (relaxLimit || endF<startF+m_options.maxPhraseLength) && // within length limit
                 (endF==maxF || sentence.alignedCountS[endF]==0)); // unaligned
                endF++) { 
              // extract code:
              // at this point we have extracted a phrase
              extractPhrase( sentence, startE, endE, startF, endF, countF, inboundPhrases,
                             inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                             outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                             buildExtraStructure);
            } 
          }
        }
      }
    }
  }
}


bool ExtractTask::getAlignedSpan( int minE, int maxE, int *minF, int *maxF, 
                                  vector< int > usedF, vector<vector<int> > &alignedToE ) {
  *minF = 9999;
  *maxF = -1;
  // go over all English words, aligned foreign words
  for(int ei=minE; ei<=maxE; ei++) {
    for(size_t i=0; i<alignedToE[ei].size(); i++) {
      // extend span with each new aligned word
      int fi = alignedToE[ei][i];
      if (fi < *minF) {
        *minF = fi;
      }
      if (fi > *maxF) {
        *maxF = fi;
      }
      usedF[ fi ]--;
    }
  }
  // check if there are any words f that have additional alignment points
  bool out_of_bounds = false;
  for(int fi=*minF; fi<=*maxF && !out_of_bounds; fi++) {
    if (usedF[fi]>0) {
      out_of_bounds = true;
    }
  }
  return out_of_bounds;
}


void ExtractTask::extractPhrase(SentenceAlignment &sentence,
  int startE, int endE, int startF, int endF, int countF,
  HPhraseVector &inboundPhrases,
  HSentenceVertices &inTopLeft, HSentenceVertices &inTopRight, 
  HSentenceVertices &inBottomLeft, HSentenceVertices &inBottomRight,
  HSentenceVertices &outTopLeft, HSentenceVertices &outTopRight, 
  HSentenceVertices &outBottomLeft, HSentenceVertices &outBottomRight,
  bool buildExtraStructure)
{
  if(buildExtraStructure) { // phrase || hier
    if(endE-startE < m_options.maxPhraseLength && endF-startF < m_options.maxPhraseLength) { // within limit
      inboundPhrases.push_back(HPhrase(HPhraseVertex(startF,startE),
                               HPhraseVertex(endF,endE)));
      insertPhraseVertices(inTopLeft, inTopRight, inBottomLeft, inBottomRight,
                           startF, startE, endF, endE);
    } 
    else {
      insertPhraseVertices(outTopLeft, outTopRight, outBottomLeft, outBottomRight,
                           startF, startE, endF, endE);
    } 
  }
  else {
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
    }
    addPhrase(sentence, startE, endE, startF, endF, orientationInfo);
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

void ExtractTask::addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF , string &orientationInfo)
{
  // source
  //   // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n";
  	ostringstream outextractstr;
  	ostringstream outextractstrInv;
  	ostringstream outextractstrOrientation;
  	ostringstream outextractstrSentenceId;
  	ostringstream outextractstrPsd;

  if (m_options.isOnlyOutputSpanInfo()) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  }

  if (m_options.isOutputPsd()) {
    outextractstrPsd << sentence.sentenceID << "\t" << startF << "\t" << endF << "\t" << startE << "\t" << endE << "\t";
  }
  for(int fi=startF; fi<=endF; fi++) {
    if (m_options.isTranslationFlag()) outextractstr << sentence.source[fi] << " ";
    if (m_options.isOrientationFlag()) outextractstrOrientation << sentence.source[fi] << " ";
    if (m_options.isSentenceIdFlag()) outextractstrSentenceId << sentence.source[fi] << " ";
    if (m_options.isOutputPsd()) outextractstrPsd << sentence.source[fi];
    if (m_options.isOutputPsd() && fi != endF) outextractstrPsd << " "; 
  }
  if (m_options.isTranslationFlag()) outextractstr << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";
  if (m_options.isSentenceIdFlag()) outextractstrSentenceId << "||| ";
  if (m_options.isOutputPsd()) outextractstrPsd << "\t";

  // target
  for(int ei=startE; ei<=endE; ei++) {
    if (m_options.isTranslationFlag()) outextractstr << sentence.target[ei] << " ";
    if (m_options.isTranslationFlag()) outextractstrInv << sentence.target[ei] << " ";
    if (m_options.isOrientationFlag()) outextractstrOrientation << sentence.target[ei] << " ";
    if (m_options.isSentenceIdFlag()) outextractstrSentenceId << sentence.target[ei] << " ";
    if (m_options.isOutputPsd()) outextractstrPsd << sentence.target[ei];
    if (m_options.isOutputPsd() && ei != endE) outextractstrPsd << " "; 
  }
  if (m_options.isTranslationFlag()) outextractstr << "|||";
  if (m_options.isTranslationFlag()) outextractstrInv << "||| ";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "||| ";
  if (m_options.isSentenceIdFlag()) outextractstrSentenceId << "||| ";
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

  if (m_options.isSentenceIdFlag()) {
    outextractstrSentenceId << sentence.sentenceID;
  }


 if (m_options.isTranslationFlag()) outextractstr << "\n";
  if (m_options.isTranslationFlag()) outextractstrInv << "\n";
  if (m_options.isOrientationFlag()) outextractstrOrientation << "\n";
  if (m_options.isSentenceIdFlag()) outextractstrSentenceId << "\n";
  if (m_options.isOutputPsd()) outextractstrPsd << "\n";

    m_extractedPhrases.push_back(outextractstr.str());
    m_extractedPhrasesInv.push_back(outextractstrInv.str());
    m_extractedPhrasesOri.push_back(outextractstrOrientation.str());
    m_extractedPhrasesSid.push_back(outextractstrSentenceId.str());
    m_extractedPsdOutput.push_back(outextractstrPsd.str());
}


void ExtractTask::writePhrasesToFile(){

    ostringstream outextractFile;
    ostringstream outextractFileInv;
    ostringstream outextractFileOrientation;
    ostringstream outextractFileSentenceId;
    ostringstream outextractFilePsd;

    for(vector<string>::const_iterator phrase=m_extractedPhrases.begin();phrase!=m_extractedPhrases.end();phrase++){
        outextractFile<<phrase->data();
    }
    for(vector<string>::const_iterator phrase=m_extractedPhrasesInv.begin();phrase!=m_extractedPhrasesInv.end();phrase++){
        outextractFileInv<<phrase->data();
    }
    for(vector<string>::const_iterator phrase=m_extractedPhrasesOri.begin();phrase!=m_extractedPhrasesOri.end();phrase++){
        outextractFileOrientation<<phrase->data();
    }
    for(vector<string>::const_iterator phrase=m_extractedPhrasesSid.begin();phrase!=m_extractedPhrasesSid.end();phrase++){
        outextractFileSentenceId<<phrase->data();
    }

    for(vector<string>::const_iterator it=m_extractedPsdOutput.begin();it!=m_extractedPsdOutput.end();it++){
        outextractFilePsd<<it->data();
    }
      m_extractCollector->Write(m_id, outextractFile.str());
      m_extractCollectorInv->Write(m_id,outextractFileInv.str());
      m_extractCollectorOrientation->Write(m_id,outextractFileOrientation.str());
      m_extractCollectorSentenceId->Write(m_id,outextractFileSentenceId.str());
      m_extractCollectorPsd->Write(m_id, outextractFilePsd.str());
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
    m_extractCollector->Write(m_id, outextractFile.str());
    m_extractCollectorInv->Write(m_id,outextractFileInv.str());

}

}

