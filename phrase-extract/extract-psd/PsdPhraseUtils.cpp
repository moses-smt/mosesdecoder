#include "InputFileStream.h"
#include "SafeGetline.h"
#include "PsdPhraseUtils.h"
#include "Util.h"
#include <iostream>

using namespace MosesTraining;
using namespace Moses;

PHRASE makePhrase(const string phrase, Vocabulary &wordVocab){
    PHRASE p;
    vector<string> toks = Tokenize(phrase);
    for(size_t j = 0; j < toks.size(); ++j){
	WORD_ID id = wordVocab.getWordID(toks[j]);
	if (id){
	    p.push_back(id);
	}else{
	  //	    cerr << "Warning: OOV word " << toks[j] << " in phrase " << phrase <<  endl;
	    p.clear();
	    return p;
	}	
    }
    return p;
}

PHRASE makePhraseAndVoc(const string phrase, Vocabulary &wordVocab){
    PHRASE p;
    vector<string> toks = Tokenize(phrase);
    for(size_t j = 0; j < toks.size(); j++){
	WORD_ID id = wordVocab.storeIfNew(toks[j]);
	p.push_back(id);
    }
    return p;
}

PHRASE_ID getPhraseID(const string phrase, Vocabulary &wordVocab, PhraseVocab &vocab){
    PHRASE p = makePhrase(phrase,wordVocab);
    if (p.size() > 0) return vocab.getPhraseID(p);
    return 0;
}

string getPhrase(PHRASE_ID labelid, Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc){
  PHRASE p = tgtPhraseVoc.getPhrase(labelid);
  string phrase = "";
  for(int i = 0; i < p.size(); ++i){
      if (phrase != ""){
	phrase += " ";
      }
      phrase += tgtVocab.getWord(p[i]);
  }  
  return phrase;
}


bool readPhraseVocab(const char* vocabFile, Vocabulary &wordVocab, PhraseVocab &vocab){
    InputFileStream file(vocabFile);
    if (!file) return false;
    while(!file.eof()){
      char line[LINE_MAX_LENGTH];
      SAFE_GETLINE(file, line, LINE_MAX_LENGTH, '\n', __FILE__);
      if (file.eof()) return true;
      PHRASE phrase = makePhraseAndVoc(string(line),wordVocab);
      int pid = vocab.storeIfNew(phrase);
    }
    return true;
}

bool readPhraseTranslations(const char *ptFile, Vocabulary &srcWordVocab, Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable){
    InputFileStream file(ptFile);
    if(!file) return false;
    while(!file.eof()){
	char line[LINE_MAX_LENGTH];
	SAFE_GETLINE(file, line, LINE_MAX_LENGTH, '\n', __FILE__);
	if (file.eof()) return true;
	vector<string> fields = TokenizeMultiCharSeparator(string(line), " ||| ");
	//	cerr << "TOKENIZED: " << fields.size()  << " tokens in " << line << endl;
	if (fields.size() < 2){
	    cerr << "Skipping malformed phrase-table entry: " << line << endl;
	}
	PHRASE_ID src = getPhraseID(fields[0],srcWordVocab,srcPhraseVocab);
	PHRASE_ID tgt = getPhraseID(fields[1],tgtWordVocab,tgtPhraseVocab);
	if (src && tgt){
	  PhraseTranslations::iterator itr = transTable.find(src);
	  if (itr == transTable.end() ){
	    map<PHRASE_ID,int> tgts;
	    tgts.insert(make_pair (tgt,0)); 
	    transTable.insert(make_pair (src,tgts));
	  }else{
	    map<PHRASE_ID,int>::iterator itr2 = itr->second.find(tgt);
	    if (itr2 == itr->second.end()){
	      itr->second.insert(make_pair(tgt,itr->second.size()));
	    }
	  }
	}
	/*	}else{
	    cerr << "Skipping phrase-table entry due to OOV phrase: " << line << endl;
	}
*/
    }
    return true;
}

bool exists(PHRASE_ID src, PHRASE_ID tgt, PhraseTranslations &transTable){
    PhraseTranslations::iterator itr = transTable.find(src);
    if (itr == transTable.end()) return false;
    map<PHRASE_ID,int>::iterator itr2 = (itr->second).find(tgt);
    if (itr2 == (itr->second).end()) return false;
    return true;
}

bool exists(PHRASE_ID src, PhraseTranslations &transTable){
  return (transTable.find(src) != transTable.end());
}

bool printTransToFile(string fileName, PhraseTranslations &transTable, PHRASE_ID src){
    ofstream out(fileName.c_str());
    PhraseTranslations::iterator itr = transTable.find(src);
    if (itr == transTable.end()) return false;
    for(map<PHRASE_ID,int>::iterator itr2 = (itr->second).begin(); itr2 != (itr->second).end(); itr2++){
	out << itr2->second << " " << itr2->first << endl; 
    }
    return true;    
}


bool printTransStringToFile(string fileName, PhraseTranslations &transTable, PHRASE_ID src, PhraseVocab &pVocab, Vocabulary &wVocab){
    ofstream out(fileName.c_str());
    PhraseTranslations::iterator itr = transTable.find(src);
    if (itr == transTable.end()) return false;
    for(map<PHRASE_ID,int>::iterator itr2 = (itr->second).begin(); itr2 != (itr->second).end(); itr2++){
      PHRASE p = pVocab.getPhrase(itr2->first);
      if (p.size() > 0){
	string phrase = wVocab.getWord(p[0]);
	for(int i = 1; i < p.size(); i++){
	  phrase = phrase + " " + wVocab.getWord(p[i]);
	}
	out << itr2->second << " " << phrase << endl; 
      }else{
	return false;
      }
    }
    return true;    
}

bool readPhraseTranslations(const char *ptFile, Vocabulary &srcWordVocab, Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable, map<string,string> &transTableScores){
    InputFileStream file(ptFile);
    if(!file) return false;
    while(!file.eof()){
	char line[LINE_MAX_LENGTH];
	SAFE_GETLINE(file, line, LINE_MAX_LENGTH, '\n', __FILE__);
	if (file.eof()) return true;
	vector<string> fields = TokenizeMultiCharSeparator(string(line)," ||| ");
	if (fields.size() < 2){
	    cerr << "Skipping malformed phrase-table entry: " << line << endl;
	}
	PHRASE_ID src = getPhraseID(fields[0],srcWordVocab,srcPhraseVocab);
	PHRASE_ID tgt = getPhraseID(fields[1],tgtWordVocab,tgtPhraseVocab);
	if (src && tgt){
	  PhraseTranslations::iterator itr = transTable.find(src);
	  string stpair = SPrint(src)+" "+SPrint(tgt);
	  transTableScores.insert(make_pair (stpair,fields[2]));
	  if (itr == transTable.end() ){
	    map<PHRASE_ID,int> tgts;
	    tgts.insert(make_pair (tgt,0));
	    transTable.insert(make_pair (src,tgts));
	  }else{
	    map<PHRASE_ID,int>::iterator itr2 = itr->second.find(tgt);
	    if (itr2 == itr->second.end()){
	      itr->second.insert(make_pair(tgt,itr->second.size()));
	    }
	  }
	}
	/*	}else{
	    cerr << "Skipping phrase-table entry due to OOV phrase: " << line << endl;
	}
*/
    }
    return true;
}
