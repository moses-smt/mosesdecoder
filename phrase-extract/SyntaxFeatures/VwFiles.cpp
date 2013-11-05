#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>

#include "contrib/eppex/SafeGetline.h"
#include "moses/TranslationModel/fuzzy-match/Vocabulary.h"
#include "moses/InputFileStream.h"
#include "moses/OutputCollector.h"
#include "tables-core.h"
#include "moses/Util.h"

#include "psd.h"
#include "PsdPhraseUtils.h"
#include "psd/FeatureExtractor.h"
#include "psd/FeatureConsumer.h"
#include "TaggedCorpus.h"

#include "InputTreeRep.h"
#include "XmlTree.h"

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace boost::bimaps;
using namespace PSD;

#define LINE_MAX_LENGTH 10000

//NOTE : HIERO : FORMAT OF EXTRACT.PSD IS WRONG FOR TARGET SIDE : X INSTEAD OF X0.

// globals
CLASSIFIER_TYPE psd_classifier = VWFile;
PSD_MODEL_TYPE psd_model = GLOBAL;
string ptDelim = " ||| ";
string factorDelim = "|";
int subdirsize=1000;
string ext = ".contexts";

// settings for debugging
int StartSNo = 0;
int EndSNo = 0;
bool skipPT = false;

Vocabulary srcVocab;
Vocabulary tgtVocab;

/*int main(int argc,char* argv[]){
  cerr << "PSD and Syntax feature-extractor\n\n";
  if (argc < 8){
    cerr << "syntax: extract-syntax-features extract-file corpus.factored corpus.parsed phrase-table sourcePhraseVocab targetPhraseVocab extractor-config outputdir/filename [options]\n";
    cerr << endl;
    cerr << "Options:" << endl;
    cerr << "\t --ClassifierType vw|megam" << endl;
    cerr << "\t --PsdType phrasal|global" << endl;
    exit(1);
  }

  char* &fileNameExtract = argv[1]; // location in corpus of PSD phrases (optionallly annotated with the position and phrase type of their translations.)
  char* &fileNameSrcTag = argv[2]; // tagged source context in Moses factored format
  string fileNameSrcParse = string(argv[3]); //parsed source file in Moses tagged format
  char* &fileNamePT = argv[4]; // phrase table
  char* &fileNameSrcVoc = argv[5]; // source phrase vocabulary
  char* &fileNameTgtVoc = argv[6]; // target phrase vocabulary
  string configFile = string(argv[7]); // configuration file for the feature extractor
  string output = string(argv[8]);//output directory (for phrasal models) or root of filename (for global models)
  for(int i = 9; i < argc; i++){
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
    else if (strcmp(argv[i],"--PsdType") == 0){
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
    else if (strcmp(argv[i],"--StartSNo") == 0){
      char* s = argv[++i];
      StartSNo = atoi(s);
                }
    else if (strcmp(argv[i],"--EndSNo") == 0){
      char* s = argv[++i];
      EndSNo = atoi(s);
                }
    else if (strcmp(argv[i],"--SkipPT") == 0){
      skipPT = true;
                }
                else {
                        cerr << "failed to parse option: " << argv[i] << endl;
                        exit(1);
                }


  }

  //cerr << "Reading inputs !" << endl;

  InputFileStream extract(fileNameExtract);
  if (extract.fail()){
    cerr << "ERROR: could not open " << fileNameExtract << endl;
  }

  InputFileStream srcTag(fileNameSrcTag);
  if (srcTag.fail()){
    cerr << "ERROR: could not open " << fileNameSrcTag << endl;
  }

  InputFileStream srcParse(fileNameSrcParse);
  if (srcParse.fail()){
    cerr << "ERROR: could not open " << fileNameSrcParse << endl;
  }

  // store word and phrase vocab
  //cerr << "READING RULE VOCAB SOURCE"<< endl;
  PhraseVocab psdPhraseVoc;
  if (!readRuleVocab(fileNameSrcVoc,srcVocab,psdPhraseVoc)){
    cerr << "Error reading in source phrase vocab" << endl;
  }

  //cerr << "READING RULE VOCAB TARGET"<< endl;
  PhraseVocab tgtPhraseVoc;
  if (!readRuleVocab(fileNameTgtVoc,tgtVocab,tgtPhraseVoc)){
    cerr << "Error reading in target phrase vocab" << endl;
  }
  // store translation phrase pair table
  //cerr << "READING RULES"<< endl;
  PhraseTranslations transTable;
  if (! skipPT) {
   if (!readRules(fileNamePT, srcVocab, tgtVocab, psdPhraseVoc, tgtPhraseVoc, transTable)){
     cerr << "Error reading in phrase translation table " << endl;
   }
  }

    // loop through tagged PSD examples in the order they occur in the training corpus
    int extractlinenum = 0;
    int csid = 0;

    // loop through tagged PSD examples in the order they occur in the training corpus

    // create target phrase index for feature extractor
    TargetIndexType extractorTargetIndex;
    for (size_t i = 0; i < tgtPhraseVoc.phraseTable.size(); i++) {
        // label 0 is not allowed
        extractorTargetIndex.insert(TargetIndexType::value_type(getTargetRule(i, tgtVocab, tgtPhraseVoc), i + 1));
    }

    ExtractorConfig config;
    config.Load(configFile);
    FeatureExtractor extractor(extractorTargetIndex, config, true);


    // prep feature consumers for PHRASAL setting
    map<PHRASE_ID, FeatureConsumer*> consumers;

    // feature consumer for GLOBAL setting
    FeatureConsumer *globalOut = NULL;
    if (psd_model == GLOBAL) {
        if (psd_classifier == VWLib)
        globalOut = new VWLibraryTrainConsumer(output);
        else
        globalOut = new VWFileTrainConsumer(output);
    }

  cerr<< "Phrase/Rule tables read. Now reading in psd-extract-file." << endl;

  int prevSNo = 0;
  while(true) {
    if (extract.eof()) break;
    char extractLine[LINE_MAX_LENGTH];

    // get phrase pair
    SAFE_GETLINE((extract),extractLine, LINE_MAX_LENGTH, '\n', __FILE__);
    if (extract.eof()) break;

    if (++extractlinenum % 100000 == 0) cerr << "." << flush;
    vector<string> token = Tokenize(extractLine,"\t");
    size_t sid = Scan<size_t>(token[0].c_str());

    //cerr << "READING EXTRACT FILE : " << extractLine << endl;
       	if (sid < StartSNo) {
			continue;
		}

		if (EndSNo != 0 and sid > EndSNo) {
			break;
		}

		if (StartSNo > 0) {
			if (prevSNo != sid) {
				cerr << "extract line number " << extractlinenum << " SNo " << sid << endl;
				prevSNo = sid;
			}
		}

    size_t src_start = Scan<size_t>(token[1].c_str());
    size_t src_end = Scan<size_t>(token[2].c_str());
    size_t tgt_start = Scan<size_t>(token[3].c_str());
    size_t tgt_end = Scan<size_t>(token[4].c_str());
    string sourceRule = token[5];
    string targetRule = token[6];

    char tagSrcLine[LINE_MAX_LENGTH];
    char parseSrcLine[LINE_MAX_LENGTH];

    // go to current sentence
    // hiero : current sentence and current parse
    while(csid < sid){
      if (srcTag.eof()) break;
      SAFE_GETLINE((srcTag),tagSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);
      SAFE_GETLINE((srcParse),parseSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);
      ++csid;
    }

    assert(csid == sid);
    ContextType factoredSrcLine = parseTaggedString(tagSrcLine, factorDelim,config.GetFactors().size());

    // get surface forms from factored format
    vector<string> sent;
    ContextType::const_iterator tagSrcIt;
    for (tagSrcIt = factoredSrcLine.begin(); tagSrcIt != factoredSrcLine.end(); tagSrcIt++) {
      sent.push_back(*tagSrcIt->begin());
      //cerr << "Pushed back to sentence : " << *tagSrcIt->begin() << endl;
    }

   //cerr << "Extracting syntactic features..." << parseSrcLine << endl;
   //cerr << "Size of sentence : " << sent.size() << endl;

    //std::cerr << "Reading in rule table" << std::endl;
    //std::cerr << "Source Rule : " << sourceRule << std::endl;


    //hiero : use source side in extract file
    //cerr << "GETTING SOURCE ID... " << endl;
    PHRASE_ID srcid = getRuleID(sourceRule, srcVocab, psdPhraseVoc);

    //std::cerr << "Source ID : " << srcid << std::endl;
    //std::cerr << "Target rule " << targetRule << std::endl;

    //get all target phrase for this source phrase
    //cerr << "GETTING TARGET ID... " << endl;
    PHRASE_ID labelid = getRuleID(targetRule,tgtVocab,tgtPhraseVoc) + 1; // label 0 is not allowed

    //std::cerr << "Label ID : " << labelid << std::endl;

    vector<float> losses;
    vector<Translation> translations;
    PhraseTranslations::const_iterator transIt;
    for (transIt = transTable.lower_bound(srcid); transIt != transTable.upper_bound(srcid); transIt++) {
      cerr << "INSERTING TRANSLATION : " << transIt->second.m_index << endl;
      translations.push_back(transIt->second);
      losses.push_back(labelid == transIt->second.m_index ? 0 : 1);
    }

    //only extract featues if id has been found in phrase table
        //std:cerr << "Source ID not zero " << std::endl;
      if (existsRule(srcid, labelid, transTable)) {
          //std::cerr << "Source ID is in table" << std::endl;
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
          //NOTE : check that sourceRule (from extract) is still the right string for context features
          extractor.GenerateFeaturesChart(consumers[srcid], factoredSrcLine, sourceRule, syntFeats, src_start, src_end, translations, losses);
        } else { // GLOBAL model
          //std::cerr << "Generating fetures ..." << std::endl;
          extractor.GenerateFeaturesChart(globalOut, factoredSrcLine, sourceRule, syntFeats, src_start, src_end, translations, losses);
        }
      }
  }
  if (psd_model == GLOBAL) {
    globalOut->Finish();
  } else {
    map<PHRASE_ID, FeatureConsumer*>::iterator i;
    for (i = consumers.begin(); i != consumers.end(); i++) {
      i->second->Finish();
    }
  }
  //freem
}*/


