#include "InputFileStream.h"
#include "SafeGetline.h"
#include "PsdPhraseUtils.h"
#include "Util.h"
#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace MosesTraining;
using namespace Moses;

PHRASE makePhrase(const string phrase, Vocabulary &wordVocab){
    //cerr << "MAKING PHRASE : " << phrase << std::endl;
    PHRASE p;
    vector<string> toks = Tokenize(phrase);
    for(size_t j = 0; j < toks.size(); ++j){
    //cerr << "Making phrase : " << toks[j] << std::endl;
	WORD_ID id = wordVocab.getWordID(toks[j]);
	//cerr << "Found ID : " << id << endl;
	if (id){
	    //cerr << "ID pushed back" << endl;
	    p.push_back(id);
	}else{
	  	    cerr << "Warning: OOV word " << toks[j] << " in phrase " << phrase <<  endl;
	    p.clear();
	    return p;
	}
    }
    return p;
}

void createIdForRule(int id, string word, PHRASE &p)
{
    size_t maxNumberOfNonTerms = 20;
    std::string nonTermString = "X";
    std::string parentNonTerm = "[X]";
    if (id){
	    //cerr << "ID pushed back" << endl;
	    //hiero : add
	    p.push_back(id+maxNumberOfNonTerms);
	}
	else{

        size_t foundPar = word.find(parentNonTerm);
        if(foundPar != string::npos)
        {
            //cerr << "Parent Non Terminal found : " << word << endl;
            //cerr <<" Only one non term : " << endl;
            WORD_ID newId = 1;
            p.push_back(newId);
        }
        else
        {
            //If not non-terminal, is it parent?
                size_t found = word.find(nonTermString);
                if(found != string::npos)
                {
                    //cerr << "Non Terminal found : " << endl;
                    if(word.size() == 1)
                    {
                        WORD_ID newId = 2;
                        p.push_back(newId);
                    }
                    else
                    {
                        string indexString = word.substr(1,1);
                        //cerr << "Found index : " << indexString << endl;
                        int index = boost::lexical_cast<int>( indexString );
                        WORD_ID newId = 2+index;
                        p.push_back(newId);
                    }
                }
                else
                {
                    cerr << "Warning: OOV word " << word;
                    p.clear();
                }
        }
    }
}

PHRASE makeRule(const string phrase, Vocabulary &wordVocab){
    //cerr << "MAKGING RULE : " << phrase << std::endl;
    PHRASE p;

    vector<string> toks = Tokenize(phrase);
    for(size_t j = 0; j < toks.size(); ++j){
	WORD_ID id = wordVocab.getWordID(toks[j]);
	//cerr << "Found ID : " << id << endl;
	createIdForRule(id,toks[j],p);
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

PHRASE makeRuleAndVoc(const string rule, Vocabulary &wordVocab){

    //cerr << "Making phrase and voc..." << endl;
    PHRASE p;
    vector<string> toks = Tokenize(rule);
    for(size_t j = 0; j < toks.size(); j++){
        //cerr << "Using word" << j << endl;
        WORD_ID id = wordVocab.storeIfNew(toks[j]);
        createIdForRule(id,toks[j],p);
    }
    return p;
}

PHRASE_ID getPhraseID(const string phrase, Vocabulary &wordVocab, PhraseVocab &vocab){
    PHRASE p = makePhrase(phrase,wordVocab);
    if (p.size() > 0) return vocab.getPhraseID(p);
    return 0;
}

PHRASE_ID getRuleID(const string rule, Vocabulary &wordVocab, PhraseVocab &vocab){
    PHRASE p = makeRule(rule,wordVocab);
    if (p.size() > 0) return vocab.getPhraseID(p);
    return 0;
}

string getPhrase(PHRASE_ID labelid, Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc){
  PHRASE p = tgtPhraseVoc.getPhrase(labelid);
  string phrase = "";
  for(size_t i = 0; i < p.size(); ++i){
    if (phrase != ""){
      phrase += " ";
    }
    phrase += tgtVocab.getWord(p[i]);
  }
  return phrase;
}

string getTargetRule(PHRASE_ID labelid, Vocabulary &tgtVocab, PhraseVocab &tgtPhraseVoc){

  string parentNonTerm = "[X]";
  string nonTerm = "X";
  PHRASE p = tgtPhraseVoc.getPhrase(labelid);
  string phrase = "";
  for(size_t i = 0; i < p.size(); ++i){
    if (phrase != ""){
      phrase += " ";
    }
    //cerr << "Looking for phrase : " << p[i] << "in vocab of size : " << tgtVocab.getSize() << endl;
    if(p[i] > 20)
    {
        phrase += tgtVocab.getWord(p[i] - 20);
    }
    else
    {
        if(p[i] == 1)
        {
            phrase += parentNonTerm;
        }
        else
        {
            if(p[i] == 2)
            {
                phrase += nonTerm;
                phrase += SPrint(0);
            }
            else
            {
                if(p[i] < 20)
                {
                    CHECK(p[i]>2);
                    phrase += nonTerm;
                    phrase += SPrint(p[i]-2);
                }
            }
        }
    }
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
      vocab.storeIfNew(phrase);
    }
    return true;
}


bool readRuleVocab(const char* vocabFile, Vocabulary &wordVocab, PhraseVocab &vocab){
    InputFileStream file(vocabFile);
    if (!file) return false;
    while(!file.eof()){
      char line[LINE_MAX_LENGTH];
      SAFE_GETLINE(file, line, LINE_MAX_LENGTH, '\n', __FILE__);
      if (file.eof()) return true;
      //std::cout << "Line to make vocab : " << line << std::endl;
      PHRASE phrase = makeRuleAndVoc(string(line),wordVocab);
      vocab.storeIfNew(phrase);
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
      transTable.insert(make_pair(src, tgt));
    }
  }
  /*	}else{
    cerr << "Skipping phrase-table entry due to OOV phrase: " << line << endl;
  }
  */
  return true;
}

bool readRules(const char *ptFile, Vocabulary &srcWordVocab, Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable){
  InputFileStream file(ptFile);
  if(!file) return false;
  while(!file.eof()){
    char line[LINE_MAX_LENGTH];
    SAFE_GETLINE(file, line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (file.eof()) return true;
    vector<string> fields = TokenizeMultiCharSeparator(string(line), " ||| ");
    	//cerr << "TOKENIZED: " << fields.size()  << " tokens in " << line << endl;
    if (fields.size() < 2){
      cerr << "Skipping malformed phrase-table entry: " << line << endl;
    }

    //increment target ids to match target indexes
    PHRASE_ID src = getRuleID(fields[0],srcWordVocab,srcPhraseVocab);
    PHRASE_ID tgt = getRuleID(fields[1],tgtWordVocab,tgtPhraseVocab);
    if (src && tgt){
      transTable.insert(make_pair(src, tgt));
    }
  }
  /*	}else{
    cerr << "Skipping phrase-table entry due to OOV phrase: " << line << endl;
  }
  */
  return true;
}


bool exists(PHRASE_ID src, PHRASE_ID tgt, PhraseTranslations &transTable){

  //std::cerr << "Looking for translations involving " << src << " : " << tgt << std::endl;
  PhraseTranslations::const_iterator it;
  for (it = transTable.lower_bound(src); it != transTable.upper_bound(src); it++) {
    //std::cerr << "Target : " << it->second << std::endl;
    if (it->second == tgt)
      return true;
  }
  return false;
}


bool existsRule(PHRASE_ID src, PHRASE_ID tgt, PhraseTranslations &transTable){

   //Hack : decrement passed target ID because incremented in extract-syntax-features
  tgt--;
  //std::cerr << "Looking for translations involving " << src << " : " << tgt << std::endl;
  PhraseTranslations::const_iterator it;
  for (it = transTable.lower_bound(src); it != transTable.upper_bound(src); it++) {
    //std::cerr << "Target : " << it->second << std::endl;
    if (it->second == tgt)
      return true;
  }
  return false;
}


bool exists(PHRASE_ID src, PhraseTranslations &transTable){
  return (transTable.find(src) != transTable.end());
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

	//cerr << "Source Phrase " << fields[0]<< " : " << endl;
    //cerr << "Target Phrase " << fields[1]<< " : " << endl;

	PHRASE_ID src = getPhraseID(fields[0],srcWordVocab,srcPhraseVocab);
	PHRASE_ID tgt = getPhraseID(fields[1],tgtWordVocab,tgtPhraseVocab);
	if (src && tgt){
    transTable.insert(make_pair(src, tgt));
	  string stpair = SPrint(src)+" "+SPrint(tgt);
	  transTableScores.insert(make_pair (stpair,fields[2]));
	}
	/*	}else{
	    cerr << "Skipping phrase-table entry due to OOV phrase: " << line << endl;
	}
*/
    }
    return true;
}
