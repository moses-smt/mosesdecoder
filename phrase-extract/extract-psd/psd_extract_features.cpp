#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>

#include "SafeGetline.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "tables-core.h"
#include "Util.h"

#include "psd.h"
#include "PsdPhraseUtils.h"
#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
#include "TaggedCorpus.h"

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace boost::bimaps;

#define LINE_MAX_LENGTH 10000

// globals
CLASSIFIER_TYPE psd_classifier = VWFile;
PSD_MODEL_TYPE psd_model = GLOBAL;
string ptDelim = " ||| ";
string factorDelim = "|";
int subdirsize=1000;
string ext = ".contexts";

Vocabulary srcVocab;
Vocabulary tgtVocab;

int main(int argc,char* argv[]){
  cerr << "PSD phrase-extractor\n\n";
  if (argc < 8){
    cerr << "syntax: extract-psd context.template corpus.psd corpus.raw corpus.factored phrase-table sourcePhraseVocab targetPhraseVocab outputdir/filename [options]\n";
    cerr << endl;
    cerr << "Options:" << endl;
    cerr << "\t --ClassifierType vw|megam" << endl;
    cerr << "\t --PsdType phrasal|global" << endl;
    exit(1);
  }
  char* &fileNameContext = argv[1]; // context template
  char* &fileNamePsd = argv[2]; // location in corpus of PSD phrases (optionallly annotated with the position and phrase type of their translations.)
  char* &fileNameSrcRaw = argv[3]; // raw source context
  char* &fileNameSrcTag = argv[4]; // tagged source context in Moses factored format
  char* &fileNamePT = argv[5]; // phrase table
  char* &fileNameSrcVoc = argv[6]; // source phrase vocabulary
  char* &fileNameTgtVoc = argv[7]; // target phrase vocabulary
  string output = string(argv[8]);//output directory (for phrasal models) or root of filename (for global models)
  for(int i =9; i < argc; i++){
    if (strcmp(argv[i],"--ClassifierType") == 0){
      char* format = argv[++i];
      if (strcmp(format,"vw") == 0){
        psd_classifier = VWFile;
        ext = ".vw-data";
      }else if (strcmp(format,"megam") == 0){
        psd_classifier = MEGAM;
        ext = ".megam";
        cerr << "megam format isn't supported" << endl;
        exit(1);
      }else{
        cerr << "classifier " << format << "isn't supported" << endl;
        exit(1);
      }
    } 
    if (strcmp(argv[i],"--PsdType") == 0){
      char* format = argv[++i];
      if (strcmp(format,"global") == 0){
        psd_model = GLOBAL;
      }else if (strcmp(format,"phrasal") == 0){
        psd_model = PHRASAL;
      }else{
        cerr << "PSD model type " << format << "isn't supported" << endl;
        exit(1);
      }
    } 
  }
  InputFileStream src(fileNameSrcRaw);
  if (src.fail()){
    cerr << "ERROR: could not open " << fileNameSrcRaw << endl;
  }
  InputFileStream srcTag(fileNameSrcTag);
  if (srcTag.fail()){
    cerr << "ERROR: could not open " << fileNameSrcTag << endl;
  }

  InputFileStream psd(fileNamePsd);
  if (psd.fail()){
    cerr << "ERROR: could not open " << fileNamePsd << endl;
  }

  // store word and phrase vocab
  PhraseVocab psdPhraseVoc;
  if (!readPhraseVocab(fileNameSrcVoc,srcVocab,psdPhraseVoc)){
    cerr << "Error reading in source phrase vocab" << endl;
  }
  PhraseVocab tgtPhraseVoc;
  if (!readPhraseVocab(fileNameTgtVoc,tgtVocab,tgtPhraseVoc)){
    cerr << "Error reading in target phrase vocab" << endl;
  }
  // store translation phrase pair table
  PhraseTranslations transTable;
  if (!readPhraseTranslations(fileNamePT, srcVocab, tgtVocab, psdPhraseVoc, tgtPhraseVoc, transTable)){
    cerr << "Error reading in phrase translation table " << endl;
  }

  // loop through tagged PSD examples in the order they occur in the training corpus
  int i = 0;
  int csid = 0;

  // configure features
  FeatureTypes ft;
  ft.m_sourceExternal = true;
  ft.m_sourceInternal = true;
  ft.m_targetInternal = true;
  ft.m_paired = false;
  ft.m_bagOfWords = false;
  ft.m_contextWindow = 2;
  ft.m_factors.push_back(0);
  ft.m_factors.push_back(1);
  ft.m_factors.push_back(2);

  // create target phrase index for feature extractor
  TargetIndexType extractorTargetIndex;
  for (size_t i = 0; i < tgtPhraseVoc.phraseTable.size(); i++) {
    extractorTargetIndex.insert(TargetIndexType::value_type(getPhrase(i, tgtVocab, tgtPhraseVoc), i));
  }
  FeatureExtractor extractor(ft, extractorTargetIndex, true);

  // prep feature consumers for PHRASAL setting
  map<PHRASE_ID, FeatureConsumer*> consumers;

  // feature consumer for GLOBAL setting
  FeatureConsumer *globalOut = NULL;
  if (psd_classifier == VWLib) 
    globalOut = new VWLibraryTrainConsumer(output);
  else
    globalOut = new VWFileTrainConsumer(output);

  cerr<< "Phrase tables read. Now reading in corpus." << endl;
  while(true) {
    if (psd.eof()) break;
    if (++i % 100000 == 0) cerr << "." << flush;
    char psdLine[LINE_MAX_LENGTH];

    // get phrase pair
    SAFE_GETLINE((psd),psdLine, LINE_MAX_LENGTH, '\n', __FILE__);
    if (psd.eof()) break;

    vector<string> token = Tokenize(psdLine,"\t");

    size_t sid = Scan<size_t>(token[0].c_str());
    size_t src_start = Scan<size_t>(token[1].c_str());
    size_t src_end = Scan<size_t>(token[2].c_str());
    size_t tgt_start = Scan<size_t>(token[3].c_str());
    size_t tgt_end = Scan<size_t>(token[4].c_str());

    char rawSrcLine[LINE_MAX_LENGTH];
    char tagSrcLine[LINE_MAX_LENGTH];

    // go to current sentence
    while(csid < sid){
      if (src.eof()) break;
      SAFE_GETLINE((src),rawSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);
      if (srcTag.eof()) break;
      SAFE_GETLINE((srcTag),tagSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);
      ++csid;
    }

    assert(csid == sid);
    vector<string> sent = Tokenize(rawSrcLine);
    string phrase;
    if (token.size() > 6){
      phrase = token[5];
    }else{
      phrase = sent[src_start];
      assert(src_end < sent.size());
      for(size_t j = src_start + 1; j < src_end + 1; j++){
        phrase = phrase+ " " + sent[j];
      }
    }

    PHRASE_ID srcid = getPhraseID(phrase, srcVocab, psdPhraseVoc);
    cout << "PHRASE : " << srcid << " " << phrase << endl;
    ContextType factoredSrcLine = parseTaggedString(tagSrcLine, factorDelim);

    string tgtphrase = token[6];
    PHRASE_ID labelid = getPhraseID(tgtphrase,tgtVocab,tgtPhraseVoc);
    vector<float> losses;
    vector<size_t> translations;
    PhraseTranslations::const_iterator transIt;
    for (transIt = transTable.lower_bound(srcid); transIt != transTable.upper_bound(srcid); transIt++) {
      translations.push_back(transIt->second);
      losses.push_back(labelid == transIt->second ? 0 : 1);
    }

    if (srcid != 0){
      if (exists(srcid, labelid, transTable)) {
        if (psd_model == PHRASAL){
          map<PHRASE_ID, FeatureConsumer*>::iterator i = consumers.find(srcid);
          if (i == consumers.end()){
            int low = floor(srcid/subdirsize)*subdirsize;
            int high = low+subdirsize-1;
            string output_subdir = output +"/"+SPrint(low)+"-"+SPrint(high);
            mkdir(output_subdir.c_str(),S_IREAD | S_IWRITE | S_IEXEC);
            string fileName = output_subdir + "/" + SPrint(srcid) + ext;
            FeatureConsumer *fc = NULL;
            if (psd_classifier == VWLib) 
              fc = new VWLibraryTrainConsumer(fileName);
            else
              fc = new VWFileTrainConsumer(fileName);
            consumers.insert(pair<PHRASE_ID,FeatureConsumer*>(srcid, fc));
          }
          extractor.GenerateFeatures(consumers[srcid], factoredSrcLine, src_start, src_end, translations, losses);
        } else { // GLOBAL model
          extractor.GenerateFeatures(globalOut, factoredSrcLine, src_start, src_end, translations, losses);
        }
      }
    }
  }
  //freem
}
