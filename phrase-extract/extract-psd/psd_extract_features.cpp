#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>

#include "../SafeGetline.h"
#include "../InputFileStream.h"
#include "../OutputFileStream.h"
#include "../tables-core.h"

#include "psd.h"
#include "PsdPhraseUtils.h"
#include "ContextFeatureSet.h"
#include "VwFiles.h"
#include "StringUtils.h"

using namespace std;

#define LINE_MAX_LENGTH 10000

// globals
CLASSIFIER_TYPE psd_classifier = RAW;
PSD_MODEL_TYPE psd_model = GLOBAL;
string ptDelim = " ||| ";
string factorDelim = "|";
int subdirsize=1000;
string ext = ".contexts";
string labelext = ".labels";
bool psd_train_mode = true;

Vocabulary srcVocab;
Vocabulary tgtVocab;

int main(int argc,char* argv[]){
    cerr << "PSD phrase-extractor\n\n";
    if (argc < 8){
	cerr << "syntax: extract-psd context.template corpus.psd corpus.raw corpus.factored phrase-table sourcePhraseVocab targetPhraseVocab outputdir/filename [options]\n";
	cerr << endl;
	cerr << "Options:" << endl;
	cerr << "\t --ClassifierType vw|megam|none" << endl;
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
		psd_classifier = VW;
		ext = ".vw-data";
		labelext = ".vw-header";
	    }else if (strcmp(format,"none") == 0){
		psd_classifier = RAW;
		ext = ".context";
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
    Moses::InputFileStream src(fileNameSrcRaw);
    if (src.fail()){
      cerr << "ERROR: could not open " << fileNameSrcRaw << endl;
    }
    Moses::InputFileStream srcTag(fileNameSrcTag);
    if (srcTag.fail()){
      cerr << "ERROR: could not open " << fileNameSrcTag << endl;
    }

    Moses::InputFileStream psd(fileNamePsd);
    if (psd.fail()){
      cerr << "ERROR: could not open " << fileNamePsd << endl;
    }

    // define context features
//    ContextFeatureSet fs(fileNameContext);
    ContextFeatureSet fs;
//    cerr << "Using context feature template:" << endl;
//    fs.printConfig();
//    cerr << "-------------------------" << endl;

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

    //  prep output files for PHRASAL setting
    map<PHRASE_ID, ostream*> outFiles;

    // get label info and prep output data file for GLOBAL setting
    ofstream* globalDataOut = new ofstream();
    //Moses::OutputFileStream* globalDataOut;
    if (psd_model == GLOBAL){
      //globalDataOut = new Moses::OutputFileStream(output+ext);
      globalDataOut->open((output+ext).c_str());
      string labelName = output+labelext;
      if (psd_train_mode && ! printVwHeaderFile(labelName,transTable,tgtPhraseVoc,tgtVocab)){
	cerr << "Failed to write file " << labelName << endl;
      }     
    }


    cerr<< "Phrase tables read. Now reading in corpus." << endl;
      while(true) {
	if (psd.eof()) break;
	if (++i % 100000 == 0) cerr << "." << flush;
	char psdLine[LINE_MAX_LENGTH];
	SAFE_GETLINE((psd),psdLine, LINE_MAX_LENGTH, '\n', __FILE__);
	if (psd.eof()) break;

	vector<string> token = tokenizeString(psdLine,"\t");
	//y	cerr << "TOKENIZED: " << token.size()  << " tokens in " << psdLine << endl;
	//  in test mode if no labels are provided
	if (token.size() == 3){
	  psd_train_mode = false;
	}else if (token.size() == 7 ){
	  psd_train_mode = true;
	}else{
	  cerr << "Malformed psd entry: " << psdLine << endl;
	  exit(1);
	}

	int sid = std::atoi(token[0].c_str());
	int src_start = std::atoi(token[1].c_str());
	int src_end = std::atoi(token[2].c_str());
	int tgt_start = -1; 
	int tgt_end = -1;
	if (psd_train_mode == true){
	  tgt_start = std::atoi(token[3].c_str());
	  tgt_end = std::atoi(token[4].c_str());
	}		

	char rawSrcLine[LINE_MAX_LENGTH];	
	char tagSrcLine[LINE_MAX_LENGTH];
	while(csid < sid){
	    if (src.eof()) break;
	    SAFE_GETLINE((src),rawSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);	
	    if (srcTag.eof()) break;
	    SAFE_GETLINE((srcTag),tagSrcLine, LINE_MAX_LENGTH, '\n', __FILE__);
	    ++csid;
	}
	assert(csid == sid);
	vector<string> sent = tokenize(rawSrcLine);
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

	PHRASE_ID srcid = getPhraseID(phrase,srcVocab,psdPhraseVoc);
	cout << "PHRASE : " << srcid << " " << phrase << endl;
	string escapedTagSrcLine = string(tagSrcLine);
	if (psd_classifier == VW){
	  escapedTagSrcLine = escapeVwSpecialChars(string(tagSrcLine));
	  factorDelim = "_PIPE_";
	}

	PHRASE_ID labelid = -1;
	if (psd_train_mode == true){
	  string tgtphrase = token[6];
	  labelid = getPhraseID(tgtphrase,tgtVocab,tgtPhraseVoc);
	}

	if (srcid != 0){
	  if ( psd_train_mode && exists(srcid,labelid,transTable) || !psd_train_mode && exists(srcid,transTable)){	    
	    vector<string> features = fs.extract(src_start,src_end,escapedTagSrcLine,factorDelim);
	    if (psd_model == PHRASAL){
	      map<PHRASE_ID, ostream*>::iterator i = outFiles.find(srcid);
	      if (i == outFiles.end()){
		int low = floor(srcid/subdirsize)*subdirsize;
		int high = low+subdirsize-1;
		string output_subdir = output +"/"+int2string(low)+"-"+int2string(high);
		struct stat stat_buf;
		mkdir(output_subdir.c_str(),S_IREAD | S_IWRITE | S_IEXEC);
		string fileName = output_subdir + "/" + int2string(srcid) + ext;
		ofstream* testFile = new ofstream();
		testFile->open(fileName.c_str());
		outFiles.insert(pair<PHRASE_ID,ostream*>(srcid,testFile));
		// also print out label dictionary
		string labelName = output_subdir + "/" + int2string(srcid) + labelext;
		switch(psd_classifier) {
		case VW:
		  if ( psd_train_mode && ! printVwHeaderFile(labelName,transTable,srcid,tgtPhraseVoc,tgtVocab)){
		    cerr << "ERROR: could not write file " << labelName << endl;
		  }
		  break;
		case RAW:
		  break;
		default:
		  if ( psd_train_mode && ! printTransStringToFile(labelName,transTable,srcid,tgtPhraseVoc,tgtVocab)){
		    cerr << "ERROR: could not write file " << labelName << endl;
		  }	
		}		
	      }
	      int perPhraseLabelid = transTable[srcid][labelid];
	      string sf = "";
	      if (psd_train_mode){
		sf = makeVwTrainingInstance(features,perPhraseLabelid);
	      }else{
		sf = makeVwTestingInstance(features);
	      }
	      switch (psd_classifier) {
	      case VW:
		//		(*outFiles[srcid]) << perPhraseLabelid << " | " << sf << endl;
		(*outFiles[srcid]) << sf << endl;
		break;
	      default:
		(*outFiles[srcid]) << src_start << "\t" << src_end << "\t" << perPhraseLabelid << "\t" << tagSrcLine << endl;
		break;
	      }
	    }
	    if (psd_model == GLOBAL && psd_classifier == VW ){
	      set<PHRASE_ID> labels;
	      labels.insert(labelid); //TODO: collect all translation candidates with fractional counts!
	      string sfeatures = "";
	      if (psd_train_mode){
		sfeatures = makeVwGlobalTrainingInstance(srcid,features,labels,transTable,tgtPhraseVoc,tgtVocab);
		(*globalDataOut) << sfeatures << endl;
	      }else{
		sfeatures = makeVwGlobalTestingInstance(srcid,features,transTable,tgtPhraseVoc,tgtVocab);
		cout << "FEATURES " << sfeatures << endl;
		(*globalDataOut) << sfeatures << endl;
	      }
	    }
	    if (psd_model == GLOBAL && psd_classifier == RAW ){
	      (*globalDataOut) << psdLine << "\t" << srcid  << "\t" << labelid << "\t" << tagSrcLine << endl;
	    }
	  }
	}
    }
    //freem
}
