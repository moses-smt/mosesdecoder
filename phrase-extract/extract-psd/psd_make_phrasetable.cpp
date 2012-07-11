#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include "SafeGetline.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "tables-core.h"
#include "Util.h"

#include "psd.h"
#include "PsdPhraseUtils.h"

using namespace std;
using namespace Moses;

#define LINE_MAX_LENGTH 10000

// globals
CLASSIFIER_TYPE psd_classifier = RAW;
PSD_MODEL_TYPE psd_model = GLOBAL;
string ptDelim = " ||| ";
string factorDelim = "|";
int subdirsize=1000;
string labelext = ".labels";
string predext = ".vw.pred";

MosesTraining::Vocabulary srcVocab;
MosesTraining::Vocabulary tgtVocab;

string output_dir="psd";

int main(int argc,char* argv[]){
    cerr << "constructor for phrase-table augmented with context-dependent PSD scores.\n\n";
    if (argc < 8){
	cerr << "syntax: make-psd-table path-to-psd-predictions corpus.psd corpus.raw base-phrase-table sourcePhraseVocab targetPhraseVocab [options]\n";
	cerr << endl;
	cerr << "Options:" << endl;
	cerr << "\t --ClassifierType vw|megam|none" << endl;
	cerr << "\t --PsdType phrasal|global" << endl;
	exit(1);
    }
    char* &fileNamePred = argv[1]; // path to PSD predictions
    char* &fileNamePsd = argv[2]; // location in corpus of PSD phrases (optionallly annotated with the position and phrase type of their translations.)
    char* &fileNameSrcRaw = argv[3]; // raw source context
    char* &fileNamePT = argv[4]; // phrase table
    char* &fileNameSrcVoc = argv[5]; // source phrase vocabulary
    char* &fileNameTgtVoc = argv[6]; // target phrase vocabulary
    char* &fileNameOutput = argv[7]; //root name for the integerized output corpus and associated phrase-table
    for(int i = 8; i < argc; i++){
	if (strcmp(argv[i],"--ClassifierType") == 0){
	    char* format = argv[++i];
	    if (strcmp(format,"vw") == 0){
		psd_classifier = VW;
		predext = ".vw.pred";
	    }else if (strcmp(format,"none") == 0){
		psd_classifier = RAW;
	    }else if (strcmp(format,"megam") == 0){
		psd_classifier = MEGAM;
		predext = ".megam";
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
    string corpus = string(fileNameOutput) + ".corpus";
    ofstream outCorpus(corpus.c_str());
    string pt = string(fileNameOutput) + ".phrase-table";
    ofstream outPT(pt.c_str());

    InputFileStream src(fileNameSrcRaw);
    if (src.fail()){
      cerr << "ERROR: could not open " << fileNameSrcRaw << endl;
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

    // store baseline phrase-table
    PhraseTranslations transTable;
    map<string,string> transTableScores;
    if (!readPhraseTranslations(fileNamePT, srcVocab, tgtVocab, psdPhraseVoc, tgtPhraseVoc, transTable, transTableScores)){
    //    if (!readPhraseTranslations(fileNamePT, srcVocab, tgtVocab, psdPhraseVoc, tgtPhraseVoc, transTable)){
	cerr << "Error reading in phrase translation table " << endl;
    }

    // get ready to read in VW predictions
    map<MosesTraining::PHRASE_ID,InputFileStream*> vwPredFiles; //for phrasal model
    InputFileStream* vwPredFile; //for global model

    
    // go through tagged PSD examples in the order they occur in the test corpus
    int i = 0;
    int csid = 0;
    map<MosesTraining::PHRASE_ID, istream*> predFiles;

    int toks_covered = -1; //last token position covered in test corpus
    cerr<< "Phrase tables read. Now reading in corpus." << endl;
    while(true) {
	if (psd.eof()) break;
	if (++i % 100000 == 0) cerr << "." << flush;
	char psdLine[LINE_MAX_LENGTH];
	SAFE_GETLINE((psd),psdLine, LINE_MAX_LENGTH, '\n', __FILE__);
	if (psd.eof()) break;
	vector<string> token = Tokenize(psdLine);
	assert(token.size() > 2);
	int sid = Scan<int>(token[0].c_str());
	int src_start = Scan<int>(token[1].c_str());
	int src_end = Scan<int>(token[2].c_str());
	//	int tgt_start = Scan<int>(token[3].c_str());
	//	int tgt_end = Scan<int>(token[4].c_str());
		
	char rawSrcLine[LINE_MAX_LENGTH];	
	while(csid < sid){
	  if (src.eof()) break;
	  SAFE_GETLINE((src),rawSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);	
	  vector<string> sent = Tokenize(rawSrcLine);
	  // print integerized test set sentence
	  string isent;
	  for(int j = 0; j < sent.size(); j++){
	    if (isent != "") isent+=" ";
	    isent += SPrint(toks_covered+j+1);
	  }
	  outCorpus << isent << endl;
	  toks_covered += sent.size();
	  ++csid;
	}
	assert(csid == sid);
	vector<string> sent = Tokenize(rawSrcLine);
	string phrase = sent[src_start];
	assert(src_end < sent.size());
	for(size_t j = src_start + 1; j < src_end + 1; j++){
	    phrase = phrase+ " " + sent[j];
	}
	
	MosesTraining::PHRASE_ID srcid = getPhraseID(phrase,srcVocab,psdPhraseVoc);
	if (srcid != 0){
	  // make integerized source id
	  string src2int = SPrint(toks_covered-sent.size()+src_start+1);
	  for(int i = src_start+1; i < src_end + 1; ++i){
	    src2int = src2int + " " + SPrint(toks_covered-sent.size()+i+1);
	  }

	  // for now don't get PSD prediction, only integerize PT without adding context-dependent score
	  if (psd_classifier != RAW){
	    cerr << "classifier type not supported yet" << endl;
	    exit(1);
	  }

	  // find candidate translations
	  PhraseTranslations::iterator itr = transTable.find(srcid);
	  if ( itr != transTable.end()){
	    for(map<MosesTraining::PHRASE_ID,int>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++){
	      MosesTraining::PHRASE_ID labelid = itr2->first;
	      string label = getPhrase(labelid, tgtVocab, tgtPhraseVoc);
	      string sl_key = SPrint(srcid) + " " + SPrint(labelid);
	      map<string,string>::iterator itr3 = transTableScores.find(sl_key);
		if (itr3 != transTableScores.end()){
		  // print integerized phrase-table
		  outPT << src2int << " ||| " << label << " ||| " << itr3->second << endl;	      
		}
	    }
	  }
	}
    }
    outPT.close();
    outCorpus.close();
}
