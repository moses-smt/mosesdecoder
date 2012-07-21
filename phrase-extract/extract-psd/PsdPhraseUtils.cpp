#include "InputFileStream.h"
#include "SafeGetline.h"
#include "PsdPhraseUtils.h"
#include "Util.h"
#include "FeatureExtractor.h"
#include <iostream>

using namespace MosesTraining;
using namespace Moses;
using namespace PSD;

PHRASE makePhrase(const string &phrase, Vocabulary &wordVocab){
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

PHRASE makePhraseAndVoc(const string &phrase, Vocabulary &wordVocab){
  PHRASE p;
  vector<string> toks = Tokenize(phrase);
  for(size_t j = 0; j < toks.size(); j++){
    WORD_ID id = wordVocab.storeIfNew(toks[j]);
    p.push_back(id);
  }
  return p;
}

PHRASE_ID getPhraseID(const string &phrase, Vocabulary &wordVocab, PhraseVocab &vocab){
  PHRASE p = makePhrase(phrase,wordVocab);
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

    Translation translation;
    translation.m_index = tgt + 1; // one based!!!

    if (fields.size() >= 3) {
      translation.m_scores = Scan<float>(Tokenize(fields[2], " "));
    }

    if (fields.size() >= 4) {
      vector<string> alignPoints = Tokenize(fields[3], " ");
      vector<string>::const_iterator alignIt;
      for (alignIt = alignPoints.begin(); alignIt != alignPoints.end(); alignIt++) {
        vector<string> point = Tokenize(*alignIt, "-");
        if (point.size() == 2) {
          translation.m_alignment.insert(make_pair(Scan<size_t>(point[0]), Scan<size_t>(point[1])));
        }
      }
    }

    transTable.insert(make_pair(src, translation));
  }
  return true;
}

bool exists(PHRASE_ID src, PHRASE_ID tgt, PhraseTranslations &transTable){
  PhraseTranslations::const_iterator it;
  for (it = transTable.lower_bound(src); it != transTable.upper_bound(src); it++) {
    if (it->second.m_index == tgt)
      return true;
  }
  return false;
}

bool exists(PHRASE_ID src, PhraseTranslations &transTable){
  return (transTable.find(src) != transTable.end());
}

