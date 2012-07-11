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

string factorDelim = "|";
MosesTraining::Vocabulary srcVocab;
MosesTraining::Vocabulary tgtVocab;

int main(int argc,char* argv[]){
    if (argc < 7){
      cerr << "Tag source phrases that are candidates for PSD disambiguation in test set\n\n";
      cerr << "syntax: tag-psd-test corpus.raw base-phrase-table source-phrase-vocab tgt-phrase-vocab maxPhraseLength output-file-name [options]\n";
      cerr << endl;
      cerr << "Options:" << endl;
      exit(1);
    }
    char* &fileNameSrcRaw = argv[1]; // raw source context
    char* &fileNamePT = argv[2]; // phrase table
    char* &fileNameSrcVoc = argv[3]; // source phrase vocabulary
    char* &fileNameTgtVoc = argv[4];
    int maxPhraseLength = Scan<int>(argv[5]);
    char* &fileNameOut = argv[6]; // output file
        
    // store word and phrase vocab and phrase table
    PhraseVocab psdPhraseVoc;
    if (!readPhraseVocab(fileNameSrcVoc,srcVocab,psdPhraseVoc)){
	cerr << "Error reading in source phrase vocab" << endl;
	exit(1);
    }

    PhraseVocab tgtPhraseVoc;
    if (!readPhraseVocab(fileNameTgtVoc,tgtVocab,tgtPhraseVoc)){
	cerr << "Error reading in target phrase vocab" << endl;
	exit(1);
    }

    PhraseTranslations transTable;
    if (!readPhraseTranslations(fileNamePT, srcVocab, tgtVocab, psdPhraseVoc, tgtPhraseVoc, transTable)){
      cerr <<  "Error reading in phrase translation table " << endl;
      exit(1);
    }

    // we will print to psdOut
    ofstream psdOut;
    psdOut.open(fileNameOut);
    if ( !psdOut ){
      cerr <<  "Error opening " << fileNameOut << endl;
      exit(1);
    }

    // read in corpus
    InputFileStream src(fileNameSrcRaw);
    if (src.fail()){
      cerr << "ERROR: could not open " << fileNameSrcRaw << endl;
      exit(1);
    }

    int sid = 0;
    
    while(true){
      if (src.eof()) break;
      char srcLine[LINE_MAX_LENGTH];
      SAFE_GETLINE((src),srcLine,LINE_MAX_LENGTH, '\n', __FILE__);
      sid++;
      if (src.eof()) break;      
      vector<string> sentence = Tokenize(srcLine);
      for(int s = 0; s < sentence.size(); s++){
	string phrase = "";
	for(int e = s; e < s+maxPhraseLength && e < sentence.size(); e++){
	  if (phrase != "") phrase += " ";
	  phrase += sentence[e];
	  MosesTraining::PHRASE_ID pid = getPhraseID(phrase, srcVocab, psdPhraseVoc);
	  if (pid == 0) break;
	  PhraseTranslations::iterator itr = transTable.find(pid);
	  if (itr == transTable.end()) break;
	  psdOut << sid << "\t" << s << "\t" << e << endl;
	}
      }
    }
}
