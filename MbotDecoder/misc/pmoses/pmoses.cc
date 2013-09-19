/*
 * Copyright (C) 2009 Felipe Sánchez-Martínez
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <string>
#include <vector>

#include "TypeDef.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "Phrase.h"
#include "TargetPhraseCollection.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"

using namespace std;
using namespace Moses;

//Delete white spaces from the end and the begining of the string
string trim(string str) {
  string::iterator it;
  
  while ((str.length()>0)&&((*(it=str.begin()))==' ')) {
    str.erase(it);
  }
           
  while ((str.length()>0)&&((*(it=(str.end()-1)))==' ')) {
    str.erase(it);
  }
                    
  for(unsigned i=0; i<str.length(); i++) {
    if ((str[i]==' ') && ((i+1)<str.length()) && (str[i+1]==' ')) {
      str=str.erase(i,1);
      i--;
    }
  }
                                            
  return str;
}
                                              

int main (int argc, char *argv[]) {
  vector<FactorType> input, output;
  vector<float> weight;
  int numScoreComponent=5;
  int numInputScores=0;
  int tableLimit=0;
  int weightWP=0;
  LMList lmList;

  input.push_back(0);
  output.push_back(0);
  
  weight.push_back(0);
  weight.push_back(0);
  weight.push_back(0);
  weight.push_back(0);
  weight.push_back(0);				

  if (argc<3) {
    cerr<<"Error: Wrong number of parameters."<<endl;
    cerr<<"Sintax: "<<argv[0]<<" /path/to/phrase/table source phrase"<<endl;
    exit(EXIT_FAILURE);
  }

  string filePath=argv[1];

  string source_str="";
  for(unsigned i=2; i<argc; i++) {
    if (source_str.length()>0) source_str+=" ";
    source_str+=argv[i];
  }

  cerr<<"numScoreComponent: "<<numScoreComponent<<endl;
  cerr<<"numInputScores: "<<numInputScores<<endl;  

  PhraseDictionaryTreeAdaptor *pd=new PhraseDictionaryTreeAdaptor(numScoreComponent, numInputScores);
				
  cerr<<"Table limit: "<<tableLimit<<endl;
  cerr<<"WeightWordPenalty: "<<weightWP<<endl;
  cerr<<"Source phrase: ___"<<source_str<<"___"<<endl;
  
  if (!pd->Load(input, output, filePath, weight, tableLimit, lmList, weightWP)) {
    delete pd;
    return false;
  }
				
  cerr<<"-------------------------------------------------"<<endl;
  FactorDirection direction;
  Phrase phrase(direction);

  phrase.CreateFromString(input, source_str, "|");
  TargetPhraseCollection *tpc = (TargetPhraseCollection*) pd->GetTargetPhraseCollection(phrase);

  if (tpc == NULL) 
    cerr<<"Not found."<<endl;
  else {				
    TargetPhraseCollection::iterator iterTargetPhrase;
    for (iterTargetPhrase = tpc->begin(); iterTargetPhrase != tpc->end();  ++iterTargetPhrase) {
      //cerr<<(*(*iterTargetPhrase))<<endl;
    
      stringstream strs;
      strs<<static_cast<const Phrase&>(*(*iterTargetPhrase));   
      cerr<<source_str<<" => ___"<<trim(strs.str())<<"___ ";
      ScoreComponentCollection scc = (*iterTargetPhrase)->GetScoreBreakdown();
      cerr<<"Scores: ";
      for(unsigned i=0; i<scc.size(); i++) {
        cerr<<scc[i]<<" ";
      }
      cerr<<endl;
    }
  }				                                                                        
  cerr<<"-------------------------------------------------"<<endl;				
}
