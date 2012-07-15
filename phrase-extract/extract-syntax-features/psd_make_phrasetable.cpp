#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include "../SafeGetline.h"
#include "../InputFileStream.h"
#include "../OutputFileStream.h"
#include "../tables-core.h"

#include "psd.h"
#include "PsdPhraseUtils.h"
#include "ContextFeatureSet.h"
#include "MegamFormat.h"
#include "StringUtils.h"
using namespace std;

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
	cerr << "syntax: make-psd-table path-to-psd-predictions corpus.psd corpus.raw base-phrase-table sourcePhraseVocab targetPhraseVocab outputName [options]\n";
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

    Moses::InputFileStream src(fileNameSrcRaw);
    if (src.fail()){
      cerr << "ERROR: could not open " << fileNameSrcRaw << endl;
    }
    Moses::InputFileStream psd(fileNamePsd);
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
    map<MosesTraining::PHRASE_ID,Moses::InputFileStream*> vwPredFiles; //for phrasal model
    Moses::InputFileStream* vwPredFile; //for global model
    
    // go through tagged PSD examples in the order they occur in the test corpus
    int i = 0;
    int csid = 0;
    map<MosesTraining::PHRASE_ID, istream*> predFiles;

    int toks_covered = -1; //last token position covered in test corpus
    cerr<< "Phrase tables read. Now reading in corpus." << endl;

    char rawSrcLine[LINE_MAX_LENGTH]; //current sentence (raw)
    vector<string> sent;
    string isent; // current sentence (integerized)
    vector<string> iSentVec; 

    while(true) {
	if (psd.eof()) break;
	if (++i % 100000 == 0) cerr << "." << flush;
	char psdLine[LINE_MAX_LENGTH];
	SAFE_GETLINE((psd),psdLine, LINE_MAX_LENGTH, '\n', __FILE__);
	if (psd.eof()) break;
	vector<string> token = tokenize(psdLine);
	assert(token.size() > 2);
	int sid = std::atoi(token[0].c_str());
	int src_start = std::atoi(token[1].c_str());
	int src_end = std::atoi(token[2].c_str());
	//	int tgt_start = std::atoi(token[3].c_str());
	//	int tgt_end = std::atoi(token[4].c_str());

       
	// read corpus and convert all sentences up to the one
	// containing the current PSD instance
	while(csid < sid){
	  if (src.eof()) break;
	  SAFE_GETLINE((src),rawSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);	
	  cout << "CLEARING ALL SENTENCES NOW..."; 
	  sent.clear();
	  isent.clear();
	  iSentVec.clear();
	  cout << " DONE." << endl;
	  cout << "READING NEW SENTENCE " << endl;
	  sent = tokenize(rawSrcLine);
	  for(int j = 0; j < sent.size(); j++){
	    if (isent != "") isent.append(" ");
	    // if token is OOV in the phrase-table, copy string
	    MosesTraining::WORD_ID wid = srcVocab.getWordID(sent[j]);
	    MosesTraining::PHRASE p; p.push_back(wid);
	    MosesTraining::PHRASE_ID pid = psdPhraseVoc.getPhraseID(p);
	    if ( transTable.find(pid) == transTable.end()){
	      isent.append(sent[j]);
	      cout << "OOV: " << sent[j] << endl;
	      iSentVec.push_back(sent[j]);
	    }else{
	      // otherwise, integerize (ie replace it by its position in the corpus)
	      isent.append(int2string(toks_covered+j+1));
	      iSentVec.push_back(int2string(toks_covered+j+1));
	    }
	  }
	  //print out integerized sentence
	  outCorpus << isent << endl;
	  cout << "RAW of size " << sent.size() << " " << rawSrcLine <<endl;
	  cout << "INT of size " << iSentVec.size() << " " << isent <<endl;
	  toks_covered += sent.size();
	  ++csid;
	}
	if (csid != sid){
	  cerr << "sid mismatch" << endl;
	  exit(1);
	}
	cout << "Now processing sentence " << sid << " " << csid << ": end position " << src_end << " in sentence of length " << sent.size() << " " << iSentVec.size() << endl;
	cout << psdLine << endl;
	cout << rawSrcLine << endl;
	cout << isent << endl;
	if(src_end >= sent.size()){
	  cerr << "Mismatch in sentence " << sid << " " << csid << ": position " << src_end << " does not exist in sentence of length " << sent.size() << " " << iSentVec.size() << endl;
	  cerr << psdLine << endl;
	  cerr << rawSrcLine << endl;
	  cerr << isent << endl;
	  exit(1);
	}

	// get original source phrase
	string phrase = sent[src_start];
	for(size_t j = src_start + 1; j < src_end + 1; j++){
	  phrase = phrase+ " " + sent[j];
	}
	MosesTraining::PHRASE_ID srcid = getPhraseID(phrase,srcVocab,psdPhraseVoc);

	// if source phrase is not OOV
	if (srcid != 0){
	  // get integerized version of source phrase
	  /*
	  string src2int = int2string(toks_covered-sent.size()+src_start+1);
	  for(int i = src_start+1; i < src_end + 1; ++i){
	    src2int = src2int + " " + int2string(toks_covered-sent.size()+i+1);
	  }
	  */
	  string src2int =  iSentVec[src_start];
	  for(int i = src_start+1; i < src_end + 1; ++i){
	    src2int = src2int + " " + iSentVec[i];
	  }

	  // if no PSD classifier, only integerize PT without adding context-dependent score (can be used to integerize lexicalized reordering table)
	  if (psd_classifier != RAW){
	    cerr << "classifier type not supported yet" << endl;
	    exit(1);
	  }

	  // find candidate translations
	  PhraseTranslations::iterator itr = transTable.find(srcid);
	  if ( itr != transTable.end() ){
	    for(map<MosesTraining::PHRASE_ID,int>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++){
	      MosesTraining::PHRASE_ID labelid = itr2->first;
	      string label = getPhrase(labelid, tgtVocab, tgtPhraseVoc);
	      string sl_key = int2string(srcid) + " " + int2string(labelid);
	      cout << "PT ENTRY: " << src2int << " ||| " << label << endl; 
	      map<string,string>::iterator itr3 = transTableScores.find(sl_key);
	      if (itr3 != transTableScores.end()){
		cout << "PT ENTRY: " << src2int << " ||| " << label << " ||| " << itr3->second << endl; 
		// print integerized phrase-table
		outPT << src2int << " ||| " << label << " ||| " << itr3->second << endl;
	      }else{
		cerr << "Mismatch in phrase-table formatting" << endl;
	      }
	    }
	  }
	}else{
	  cerr << "OOV source phrase: "  << phrase << endl;
	}
    }
    cout << "Done! # sentences read: " << csid << endl;
    outPT.close();
    outCorpus.close();
}
