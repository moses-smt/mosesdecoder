/*
 *  nbest: tool to process moses n-best lists
 *
 *  File: NBest.cpp
 *        basic functions on n-best lists
 *
 *  Created by Holger Schwenk, University of Le Mans, 05/16/2008
 *
 */


#include "NBest.h"

#include "Util.h"  // from Moses

#include <sstream>
#include <algorithm>

//NBest::NBest() {
//cerr << "NBEST: constructor called" << endl;
//}


bool NBest::ParseLine(ifstream &inpf, const int n)
{
  static string line; // used internally to buffer an input line
  static int prev_id=-1; // used to detect a change of the n-best ID
  int id;
  vector<float> f;
  float s;
  int pos=0, epos;
  vector<string> blocks;


  if (line.empty()) {
    getline(inpf,line);
    if (inpf.eof()) return false;
  }

  // split line into blocks
  //cerr << "PARSE line: " << line << endl;
  while ((epos=line.find(NBEST_DELIM,pos))!=string::npos) {
    blocks.push_back(line.substr(pos,epos-pos));
    // cerr << " block: " << blocks.back() << endl;
    pos=epos+strlen(NBEST_DELIM);
  }
  blocks.push_back(line.substr(pos,line.size()));
  // cerr << " block: " << blocks.back() << endl;

  if (blocks.size()<4) {
    cerr << line << endl;
    Error("can't parse the above line");
  }

  // parse ID
  id=Scan<int>(blocks[0]);
  if (prev_id>=0 && id!=prev_id) {
    prev_id=id;  // new nbest list has started
    return false;
  }
  prev_id=id;
  //cerr << "same ID " << id << endl;

  if (n>0 && nbest.size() >= n) {
    //cerr << "skipped" << endl;
    line.clear();
    return true; // skip parsing of unused hypos
  }

  // parse feature function scores
  //cerr << "PARSE features: '" << blocks[2] << "' size: " << blocks[2].size() << endl;
  pos=blocks[2].find_first_not_of(' ');
  while (pos<blocks[2].size() && (epos=blocks[2].find(" ",pos))!=string::npos) {
    string feat=blocks[2].substr(pos,epos-pos);
    //cerr << " feat: '" << feat << "', pos: " << pos << ", " << epos << endl;
    if (feat.find(":",0)!=string::npos) {
      //cerr << "  name: " << feat << endl;
    } else {
      f.push_back(Scan<float>(feat));
      //cerr << "  value: " << f.back() << endl;
    }
    pos=epos+1;
  }

  // eventually parse segmentation
  if (blocks.size()>4) {
    Error("parsing segmentation not yet supported");
  }

  nbest.push_back(Hypo(id, blocks[1], f, Scan<float>(blocks[3])));

  line.clear(); // force read of new line

  return true;
}


NBest::NBest(ifstream &inpf, const int n)
{
  //cerr << "NBEST: constructor with file called" << endl;
  while (ParseLine(inpf,n));
  //cerr << "NBEST: found " << nbest.size() << " lines" << endl;
}


NBest::~NBest()
{
  //cerr << "NBEST: destructor called" << endl;
}

void NBest::Write(ofstream &outf, int n)
{
  if (n<1 || n>nbest.size()) n=nbest.size();
  for (int i=0; i<n; i++) nbest[i].Write(outf);
}


float NBest::CalcGlobal(Weights &w)
{
  //cerr << "NBEST: calc global of size " << nbest.size() << endl;
  for (vector<Hypo>::iterator i = nbest.begin(); i != nbest.end(); i++) {
    (*i).CalcGlobal(w);
  }
}


void NBest::Sort()
{
  sort(nbest.begin(),nbest.end());
}

