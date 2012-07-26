#include "InputFileStream.h"
#include "SafeGetline.h"
#include "PsdPhraseUtils.h"
#include "Util.h"
#include "FeatureExtractor.h"
#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace MosesTraining;
using namespace Moses;
using namespace PSD;

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
                    cerr << " Warning: OOV word : " << word << endl;
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

PHRASE_ID getRuleID(const string rule, Vocabulary &wordVocab, PhraseVocab &vocab){
    PHRASE p = makeRule(rule,wordVocab);
    if (p.size() > 0) return vocab.getPhraseID(p);
    return 0;
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

bool readRules(const char *ptFile, Vocabulary &srcWordVocab, Vocabulary &tgtWordVocab, PhraseVocab &srcPhraseVocab, PhraseVocab &tgtPhraseVocab, PhraseTranslations &transTable){
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
    PHRASE_ID src = getRuleID(fields[0],srcWordVocab,srcPhraseVocab);
    PHRASE_ID tgt = getRuleID(fields[1],tgtWordVocab,tgtPhraseVocab);

    Translation translation;
    translation.m_index = tgt + 1; // one based!!!

    if (fields.size() >= 3) {
      translation.m_scores = Scan<float>(Tokenize(fields[2], " "));
    }

    if (fields.size() >= 4) {
      vector<string> alignPoints = Tokenize(fields[3], " ");
      //check which alignments are form non-terminals by looking at target side
        //store target side of alignment between non-terminals
      vector<string> targetAligns;
      vector<string> targetToken = Tokenize(fields[1], " ");
      //look for non-terminals in target side
      vector<string> :: iterator itr_targets;
      std::string nonTermString = "X";

      cerr << "TARGET STRING : " << fields[1] << endl;

      for(itr_targets = targetToken.begin();itr_targets != targetToken.end(); itr_targets++)
      {
            cerr << "TARGET TOKEN : " << *itr_targets << endl;

            size_t found = (*itr_targets).find(nonTermString);
            if(found != string::npos)
            {
                CHECK((*itr_targets).size() > 1);
                string indexString = (*itr_targets).substr(1,1);
                cerr << "INDEX STRING" << indexString << endl;
                targetAligns.push_back(indexString);
            }
      }

      vector<string>::const_iterator alignIt;
      for (alignIt = alignPoints.begin(); alignIt != alignPoints.end(); alignIt++) {
        vector<string> point = Tokenize(*alignIt, "-");
        bool isNonTerm = false;
        if (point.size() == 2) {
          //damt_hiero : NOTE : maybe inefficient change if hurts performance
          //WRONG : TODO : REMOVE THE i-th index STRING INDEX
          for(itr_targets = targetAligns.begin(); itr_targets != targetAligns.end(); itr_targets++)
          {
              if(point.back()==(*itr_targets)) isNonTerm = true;
          }
          if(!isNonTerm)
          {
              cerr << "INSERTING : " << point[0] <<  " : " << point[1] << endl;
              translation.m_alignment.insert(make_pair(Scan<size_t>(point[0]), Scan<size_t>(point[1])));
          }
        }
      }
    }
    transTable.insert(make_pair(src, translation));
  }
  return true;
}


bool existsRule(PHRASE_ID src, PHRASE_ID tgt, PhraseTranslations &transTable){

   //Hack : decrement passed target ID because incremented in extract-syntax-features
  //tgt--;
  //std::cerr << "Looking for translations involving " << src << " : " << tgt << std::endl;
  PhraseTranslations::const_iterator it;
  for (it = transTable.lower_bound(src); it != transTable.upper_bound(src); it++) {
    //std::cerr << "Target : " << it->second << std::endl;
    if (it->second.m_index == tgt)
      return true;
  }
  return false;
}
