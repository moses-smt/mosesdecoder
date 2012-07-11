#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <cassert>
//#include "inifile++.hpp"
#include "TaggedCorpus.h"
#include "ContextFeatureSet.h"
#include "StringUtils.h"

using namespace std;

ContextFeatureSet::ContextFeatureSet(){
  // default feature set consists of BOW context
  //  bowContext.insert(0);
  // position sensitive include all features in the [-2;+2] window
  vector<int> window;
  for(int i = -2; i < 2; i++)
  {
    window.push_back(i);
  }
  posContext[0] = window;  // surface word
  posContext[1] = window; // lemma
  posContext[2] = window; // POS
};

void ContextFeatureSet::printConfig(){
    cout << "Position sensitive features" << endl;
    for( map<int, vector<int> >::iterator i = posContext.begin(); i != posContext.end(); i++){
	cout << "Feature id: " << i->first << endl;
	for(int j = 0; j < (i->second).size(); j++){
	    cout << "Feature positions: " << i->second[j] << endl;
	}
    }
    cout << "Bag of word features" << endl;

};

vector<string> ContextFeatureSet::extract(int startPos, int endPos, string scontext, string fdelim){
  vector< vector<string> > taggedContext = parseTaggedString(scontext,fdelim);
  vector<string> posfeatures = extractPosContext(startPos,endPos,taggedContext);
  if (bowContext.size() != 0){
    vector<string> bowfeatures = extractBowContext(startPos,endPos,taggedContext);
    posfeatures.insert( posfeatures.end(), bowfeatures.begin(), bowfeatures.end() );

  }
  //  vector<string> out = std::copy(bowfeatures.begin(),bowfeatures.end(),std::back_inserter (posfeatures));
  return posfeatures;
};

vector<string> ContextFeatureSet::extractPosContext(int startPos, int endPos, vector< vector<string> > context){
  vector<string> features;
  for( map<int, vector<int> >::iterator i = posContext.begin(); i != posContext.end(); i++){
    int tagId = i->first;
    vector<int> featpos = i->second;
    for(int k = 0; k < featpos.size(); k++){
      string phrasal_feature = "";
      if (featpos[k] > 0){
	if (endPos + featpos[k] < context.size()){
	  assert(context[endPos+featpos[k]].size() > tagId );
	  if (context[endPos+featpos[k]].size() > tagId ){
	    phrasal_feature = context[endPos+featpos[k]][tagId];
	  }else{
	    cerr << "malformed context entry: need field " << tagId << " but " << context[endPos+featpos[k]].size() << " are available"  << endl;
	  }
	}else{
	  phrasal_feature = "_EOS_";
	}
      }else if(featpos[k] < 0){
	if (startPos + featpos[k] > -1 ){
	  assert(context[startPos+featpos[k]].size() > tagId );
	  if (context[startPos+featpos[k]].size() > tagId ){
	    phrasal_feature = context[startPos+featpos[k]][tagId];
	  }else{
	    cerr << "malformed context entry: need field " << tagId << " but " << context[startPos+featpos[k]].size() << " are available"  << endl;
	  }
	}else{
	  phrasal_feature = "_SOS_";
	}
      }else{
	if (startPos == endPos){
	  assert(context[startPos+featpos[k]].size() > tagId );
	  if (context[startPos+featpos[k]].size() > tagId ){
	    phrasal_feature = context[startPos+featpos[k]][tagId];
	  }else{
	    cerr << "malformed context entry: need field " << tagId << " but " << context[startPos+featpos[k]].size() << " are available"  << endl;
	  }
	}else if(startPos < endPos){
	  assert(context[startPos].size() > tagId );
	  if (context[startPos].size() > tagId ){
	    phrasal_feature = context[startPos][tagId];
	  }else{
	    cerr << "malformed context entry: need field " << tagId << " but " << context[startPos+featpos[k]].size() << " are available"  << endl;
	  }
	  for(int j = startPos+1; j < endPos + 1; j++){
	    assert(context[j].size() > tagId );
	    if (context[j].size() > tagId ){
	      phrasal_feature += "_"+context[j][tagId];
	    }else{
	      cerr << "malformed context entry: need field " << tagId << " but " << context[j].size() << " are available"  << endl;
	    }
	  } 
	}else{
	  cerr << "ERROR: target phrase starts at position " << startPos << " but ends at " << endPos << endl;
	}
      }
      if (phrasal_feature.size() > 0){
	features.push_back(int2string(tagId)+"_"+float2string(featpos[k])+"_"+phrasal_feature);
	//features.push_back(int2string(tagId)+"_"+float2string(featpos[k])+"_"+phrasal_feature+" 1");
      }
    }
  }
  return features;
};


vector<string> ContextFeatureSet::extractBowContext(int startPos, int endPos, vector< vector<string> > context){
  vector<string> features;
  for(set<int>::iterator i = bowContext.begin(); i != bowContext.end(); i++){
    map<string,int> bow;  
    // add all context features outside of the target phrase
    for(int j = 0; j < context.size(); j++){
      if (j < startPos || j > endPos){
	map<string,int>::iterator k = bow.find(context[j][*i]);
	if (k == bow.end()){
	  int f = *i;
	  string feature = context[j][f];
	  bow.insert( pair<string, int> (feature, 1));
	}else{
	  k->second+=1;
	}
      }
    }
    for(map<string,int>::iterator j = bow.begin(); j != bow.end(); j++){
      features.push_back(int2string(*i)+"_bow_"+j->first);
//      features.push_back(int2string(*i)+"_bow_"+j->first+" "+int2string(j->second));
    }
    bow.clear();
  }
  return features;
};

ContextFeatureSet::ContextFeatureSet(const char* config){
/*
  try {
    inifilepp::parser p(config);
    const inifilepp::parser::entry *ent;
    std::cout << "Reading in feature set config file: " << config << endl;
    int previd = -1;
    int fid = -1; 
    int bow = 0; 
    int wleft = 0;
    int wright = 0;
    string lexicon,stoplist;
    const inifilepp::parser::entry *prevent = NULL;
    const char* prevsect = "";
    bool new_section = 0;
    map<int,string> fids;
    while((ent = p.next())) {
	if (new_section){
	    //set feature template for previous section
	    vector<int> window;
	    for(int i = wleft; i < wright + 1; i++)
	    {
		window.push_back(i);
	    }
	    posContext.insert(std::pair<int, vector<int> >(fid,window));
	    if (bow == 1){
		bowContext.insert(fid);
	    }
	    new_section = 0;
	}
	if (strcmp(ent->key,"field") == 0){
	    prevsect = ent->sect;
	    cout << "field" << endl;
	    fid = atoi(ent->val);
	    std::cout << "READING FID: " << ent->val << endl;
	    map<int,string>::iterator p = fids.find(fid);
	    if ( p == fids.end()){
		fids.insert(pair<int,string>(fid,prevsect));
		new_section = 1;
	    }
	};
	if (strcmp(ent->key ,"bow") == 0){
	    cout << "bow" << endl;    bow = atoi(ent->val) ;
	};
	if (strcmp(ent->key,"window_left") == 0){
	    wleft = atoi(ent->val);
	}
	if (strcmp(ent->key ,"window_right") == 0){
	    wright = atoi(ent->val);
	}
	if (strcmp(ent->key,"lexicon") == 0 ){
	    lexicon = ent->val;
	}
	if (strcmp(ent->key,"stoplist") == 0){
	    stoplist = ent->val;
	}
//	prevent = ent;
    }    
    
    vector<int> window;
    for(int i = wleft; i < wright + 1; i++)
    {
	window.push_back(i);
    }
    posContext.insert(std::pair<int, vector<int> >(fid,window));
    if (bow == 1){
	bowContext.insert(fid);
    }
  } catch(inifilepp::parser::exception &e){
      std::cerr << "In config file:" << config << ", parse error at (" << e.line << "," << e.cpos << ")\n";
  }
*/
    std:: cerr << "tmp" << endl;
};

