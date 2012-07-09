#include "VwFiles.h"
#include "StringUtils.h"
#include "PsdPhraseUtils.h"
#include <set>

string escapeVwSpecialChars(string input){
  string output = input;
  replace(output,":","_COLON_");
  replace(output,"|","_PIPE_");
  return output;
}

string makeVwHeaderLine(int label, vector<string>& label_features){
  string vwHeader = int2string(label);
  vwHeader.append(":-1 |l");
  for(int i = 0; i < label_features.size(); i++){
    vwHeader.append(" ");
    vwHeader.append(label_features[i]);
  } 
  return vwHeader;
}

string makeVwTestingInstance(vector<string>& context){
  string vwInstance = makeVwLine(0,-1,"s",context);
  return vwInstance;
}

string makeVwTrainingInstance(vector<string>& context,int label){
  string vwInstance = makeVwLine(0,-1,"s",context);
  vwInstance.append("\n");
  string pairf = int2string(label+1); // Vw label ids are 1-based
  pairf.append(":0 |p dummy");
  vwInstance.append(pairf);
  vwInstance.append("\n");
  //  string vwInstance = int2string(label);
  //  vwInstance += "|s " + makeVwTestingInstance(context);
  return vwInstance;
}

//TODO string makeVwLine(int label, float cost, string namesp, vector<string> features);
string makeVwLine(int label, float cost, string namesp, vector<string>& label_features){
  string vwHeader = int2string(label);
  if (cost != -2){
    vwHeader.append(":");
    vwHeader.append(float2string(cost));
  }
  vwHeader.append(" |");
  vwHeader.append(namesp);
  for(int i = 0; i < label_features.size(); i++){
    vwHeader.append(" ");
    vwHeader.append(label_features[i]);
  } 
  return vwHeader;
}

string makeVwGlobalTrainingInstance(PHRASE_ID src, vector<string>& context,set<PHRASE_ID> labels, PhraseTranslations &transTable, PhraseVocab &pVocab, Vocabulary &wVocab){
  string vwInstance = makeVwLine(0,-1,"s",context);
  vwInstance.append("\n");

  PhraseTranslations::iterator itr = transTable.find(src);
  for(map<PHRASE_ID,int>::iterator itr2 = (itr->second).begin(); itr2 != (itr->second).end(); itr2++){
    PHRASE p = pVocab.getPhrase(itr2->first);
    if (p.size() > 0){
      string phrase = wVocab.getWord(p[0]);
      vector<string> feats;
      feats.push_back("dummy_feature");
      /*
      feats.push_back("w^"+wVocab.getWord(p[0]));
      for(int i = 1; i < p.size(); i++){
	phrase = phrase + "_" + wVocab.getWord(p[i]);
	feats.push_back("w^"+wVocab.getWord(p[i]));
      }
      feats.push_back("p^"+phrase);
      */
      float cost = 1;
      if (labels.find(itr2->first) != labels.end()) cost = 0;
      vwInstance.append(makeVwLine(itr2->first,cost,"p",feats));
      vwInstance.append("\n");
      //      string makeVwLine(int label, float cost, string namesp, vector<string>& label_features){
    }
  }
  
  //  string vwInstance = int2string(label);
  //  vwInstance += "|s " + makeVwTestingInstance(context);
  // // // vwInstance.append("\n");
  return vwInstance;
}

string makeVwGlobalTestingInstance(PHRASE_ID src, vector<string>& context, PhraseTranslations &transTable, PhraseVocab &pVocab, Vocabulary &wVocab){
  string vwInstance = makeVwLine(0,-1,"s",context);
  vwInstance.append("\n");

  PhraseTranslations::iterator itr = transTable.find(src);
  for(map<PHRASE_ID,int>::iterator itr2 = (itr->second).begin(); itr2 != (itr->second).end(); itr2++){
    PHRASE p = pVocab.getPhrase(itr2->first);
    if (p.size() > 0){
      string phrase = wVocab.getWord(p[0]);
      vector<string> feats;
      feats.push_back("dummy_feature");
      float cost = -2;
      vwInstance.append(makeVwLine(itr2->first,cost,"p",feats));
      vwInstance.append("\n");
      //      string makeVwLine(int label, float cost, string namesp, vector<string>& label_features){
    }
  }
  cout << vwInstance << endl;
  return vwInstance;
}

bool printVwHeaderFile(string fileName, PhraseTranslations &transTable, PHRASE_ID src, PhraseVocab &pVocab, Vocabulary &wVocab){
  ofstream out(fileName.c_str());
  PhraseTranslations::iterator itr = transTable.find(src);
  if (itr == transTable.end()) return false;
  for(map<PHRASE_ID,int>::iterator itr2 = (itr->second).begin(); itr2 != (itr->second).end(); itr2++){
    PHRASE p = pVocab.getPhrase(itr2->first);
    if (p.size() > 0){
      string phrase = escapeVwSpecialChars(wVocab.getWord(p[0]));
      vector<string> feats;
      feats.push_back("w^"+escapeVwSpecialChars(wVocab.getWord(p[0])));
      for(int i = 1; i < p.size(); i++){
	phrase = phrase + "_" + escapeVwSpecialChars(wVocab.getWord(p[i]));
	feats.push_back("w^"+escapeVwSpecialChars(wVocab.getWord(p[i])));
      }
      feats.push_back("p^"+phrase);
      out << makeVwHeaderLine((itr2->second)+1,feats) << endl;
    }else{
      return false;
    }
  }
  return true;    
}

bool printVwHeaderFile(string fileName, PhraseTranslations &transTable, PhraseVocab &pVocab, Vocabulary &wVocab){
  ofstream out(fileName.c_str());
  for(PhraseTranslations::iterator sitr = transTable.begin(); sitr != transTable.end(); sitr++){
    PHRASE_ID src = sitr->first;
    if (sitr == transTable.end()) return false;
    for(map<PHRASE_ID,int>::iterator itr2 = (sitr->second).begin(); itr2 != (sitr->second).end(); itr2++){
      PHRASE p = pVocab.getPhrase(itr2->first);
      if (p.size() > 0){
	string phrase = escapeVwSpecialChars(wVocab.getWord(p[0]));
	vector<string> feats;
	feats.push_back("w^"+escapeVwSpecialChars(wVocab.getWord(p[0])));
	for(int i = 1; i < p.size(); i++){
	  phrase = phrase + "_" + escapeVwSpecialChars(wVocab.getWord(p[i]));
	  feats.push_back("w^"+escapeVwSpecialChars(wVocab.getWord(p[i])));
	}
	feats.push_back("p^"+phrase);
	out << makeVwHeaderLine(itr2->first,feats) << endl;
	cerr << p.size() << " translation candidates for " <<  phrase << endl;	  
      }else{
	return false;
      }
    }
  }
  return true;    
}

