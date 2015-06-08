/*
 *  Permutation.cpp
 *  met - Minimum Error Training
 *
 *  Created by Alexandra Birch 18/11/09.
 *
 */

#include <fstream>
#include <sstream>
#include <cmath>
#include "Permutation.h"
#include "Util.h"

using namespace std;

namespace MosesTuning
{


Permutation::Permutation(const string &alignment, const int sourceLength, const int targetLength )
{
  if (sourceLength > 0) {
    set(alignment, sourceLength);
  }
  m_targetLength = targetLength;
}

size_t Permutation::getLength() const
{
  return int(m_array.size());
}
void Permutation::dump() const
{
  int j=0;
  for (vector<int>::const_iterator i = m_array.begin(); i !=m_array.end(); i++) {
    cout << "(";
    cout << j << ":" << *i ;
    cout << "), ";
    j++;
  }
  cout << endl;
}


//Sent alignment string
//Eg: "0-1 0-0 1-2 3-0 4-5 6-7 "
// Inidiviual word alignments which can be one-one,
// or null aligned, or many-many. The format is sourcepos - targetpos
//Its the output of the berkley aligner subtracting 1 from each number
//sourceLength needed because last source words might not be aligned
void Permutation::set(const string & alignment,const int sourceLength)
{

  //cout << "******** Permutation::set :" << alignment << ": len : " << sourceLength <<endl;

  if(sourceLength <= 0) {
    //not found
    cerr << "Source sentence length not positive:"<< sourceLength << endl;
    exit(0);
  }

  if (alignment.length() <= 0) {
    //alignment empty - could happen but not good
    cerr << "Alignment string empty:"<< alignment << endl;
  }

  //Tokenise on whitespace
  string buf; // Have a buffer string
  stringstream ss(alignment); // Insert the string into a stream
  vector<string> tokens; // Create vector to hold our words
  while (ss >> buf)
    tokens.push_back(buf);

  vector<int> tempPerm(sourceLength, -1);
  //Set tempPerm to have one target position per source position
  for (size_t i=0; i<tokens.size(); i++) {
    string temp = tokens[i];
    int posDelimeter = temp.find("-");
    if(posDelimeter == int(string::npos)) {
      cerr << "Delimiter not found - :"<< tokens[i] << endl;
      exit(1);
    }
    int sourcePos = atoi((temp.substr(0, posDelimeter)).c_str());
    int targetPos = atoi((temp.substr(posDelimeter+1)).c_str());
    //cout << "SP:" << sourcePos << " TP:" << targetPos << endl;
    if (sourcePos > sourceLength) {
      cerr << "Source sentence length:" << sourceLength << " is smaller than alignment source position:" << sourcePos << endl;
      cerr << "******** Permutation::set :" << alignment << ": len : " << sourceLength <<endl;
      exit(1);
    }
    //If have multiple target pos aligned to one source,
    // then ignore all but first alignment
    if (tempPerm[sourcePos] == -1 || tempPerm[sourcePos] > targetPos) {
      tempPerm[sourcePos] = targetPos;
    }
  }

  //TODO
  //Set final permutation in m_array
  //Take care of: source - null
  //              multiple_source - one target
  //              unaligned target
  // Input: 1-9 2-1 4-3 4-4 5-6 6-6 7-6 8-8
  // Convert source: 1  2  3 4 5 6 7 8
  //         target: 9  1 -1 3 6 6 6 8 -> 8 1 2 3 4 5 6 7

  // 1st step: Add null aligned source to previous alignment
  //         target: 9  1 -1 3 6 6 6 8 -> 9  1 1 3 6 6 6 8
  int last=0;
  m_array.assign(sourceLength, -1);
  //get a searcheable index
  multimap<int, int> invMap;
  multimap<int, int>::iterator it;
  //cout << " SourceP  -> TargetP " << endl;
  for (size_t i=0; i<tempPerm.size(); i++) {
    if (tempPerm[i] == -1) {
      tempPerm[i] = last;
    } else {
      last = tempPerm[i];
    }
    //cout << i << " -> " << tempPerm[i] << endl;
    //Key is target pos, value is source pos
    invMap.insert(pair<int,int>(tempPerm[i],int(i)));
  }



  // 2nd step: Get target into index of multimap and sort
  // Convert source: 1  2  3 4 5 6 7 8
  //         target: 9  1  0 3 6 6 6 8 -> 0 1 3 6 6 6 8 9
  //         source:                      3 2 4 5 6 7 8 1
  int i=0;
  //cout << " TargetP  => SourceP : TargetIndex " << endl;
  for ( it=invMap.begin() ; it != invMap.end(); it++ ) {
    //cout << (*it).first << " => " << (*it).second << " : " << i << endl;
    //find source position
    m_array[(*it).second] = i;
    i++;
  }

  bool ok = checkValidPermutation(m_array);
  //dump();
  if (!ok) {
    throw runtime_error(" Created invalid permutation");
  }
}

//Static
vector<int> Permutation::invert(const vector<int> & inVector)
{
  vector<int> outVector(inVector.size());
  for (size_t i=0; i<inVector.size(); i++) {
    outVector[inVector[i]] = int(i);
  }
  return outVector;
}

//Static
//Permutations start at 0
bool Permutation::checkValidPermutation(vector<int> const  & inVector)
{
  vector<int> test(inVector.size(),-1);
  for (size_t i=0; i< inVector.size(); i++) {
    //No multiple entries of same value allowed
    if (test[inVector[i]] > -1) {
      cerr << "Permutation error: multiple entries of same value\n" << endl;
      return false;
    }
    test[inVector[i]] ++;
  }
  for (size_t i=0; i<inVector.size(); i++) {
    //No holes allowed
    if (test[inVector[i]] == -1) {
      cerr << "Permutation error: missing values\n" << endl;
      return false;
    }
  }
  return true;
}


//TODO default to HAMMING
//Note: it returns the distance that is not normalised
float Permutation::distance(const Permutation &permCompare, const distanceMetric_t &type) const
{
  float score=0;

  //bool debug= (verboselevel()>3); // TODO: fix verboselevel()
  bool debug=false;
  if (debug) {
    cout << "*****Permutation::distance" <<endl;
    cout << "Hypo:" << endl;
    dump();
    cout << "Ref: " << endl;
    permCompare.dump();
  }

  if (type == HAMMING_DISTANCE) {
    score = calculateHamming(permCompare);
  } else if (type == KENDALL_DISTANCE) {
    score = calculateKendall(permCompare);
  } else {
    throw runtime_error("Distance type not valid");
  }

  float brevityPenalty = 1.0 - (float) permCompare.getTargetLength()/getTargetLength()  ;//reflength divided by trans length
  if (brevityPenalty < 0.0) {
    score = score * exp(brevityPenalty);
  }

  if (debug) {
    cout << "Distance type:" <<  type << endl;
    cout << "Score: "<< score << endl;
  }
  return score;
}


float Permutation::calculateHamming(const Permutation & compare) const
{
  float score=0;
  vector<int> compareArray = compare.getArray();
  if (getLength() != compare.getLength()) {
    cerr << "1stperm: " << getLength() << " 2ndperm: " << compare.getLength() << endl;
    throw runtime_error("Length of permutations not equal");
  }
  if (getLength() == 0) {
    cerr << "Empty permutation" << endl;
    return 0;
  }
  for (size_t i=0; i<getLength(); i++) {
    if (m_array[i] != compareArray[i]) {
      score++;
    }

  }
  score = 1 - (score / getLength());
  return score;
}

float Permutation::calculateKendall(const Permutation & compare) const
{
  float score=0;
  vector<int> compareArray = compare.getArray();
  if (getLength() != compare.getLength()) {
    cerr << "1stperm: " << getLength() << " 2ndperm: " << compare.getLength() << endl;
    throw runtime_error("Length of permutations not equal");
  }
  if (getLength() == 0) {
    cerr << "Empty permutation" << endl;
    return 0;
  }
  if (getLength() == 1) {
    cerr << "One-word sentence. Kendall score = 1" << endl;
    return 1;
  }
  for (size_t i=0; i<getLength(); i++) {
    for (size_t j=0; j<getLength(); j++) {
      if ((m_array[i] < m_array[j]) && (compareArray[i] > compareArray[j])) {
        score++;
      }
    }
  }
  score = (score / ((getLength()*getLength() - getLength()) /2  ) );
  //Adjusted Kendall's tau correlates better with human judgements
  score = sqrt (score);
  score = 1 - score;

  return score;
}

vector<int> Permutation::getArray() const
{
  vector<int> ret = m_array;
  return ret;
}

//Static
//This function is called with test which is
// the 5th field in moses nbest output when called with -include-alignment-in-n-best
//eg. 0=0 1-2=1-2 3=3 4=4 5=5 6=6 7-9=7-8 10=9 11-13=10-11 (source-target)
string Permutation::convertMosesToStandard(string const & alignment)
{
  if (alignment.length() == 0) {
    cerr << "Alignment input string empty" << endl;
  }
  string working = alignment;
  string out;

  stringstream oss;
  while (working.length() > 0) {
    string align;
    getNextPound(working,align," ");

    //If found an alignment
    if (align.length() > 0) {
      size_t posDelimeter = align.find("=");
      if(posDelimeter== string::npos) {
        cerr << "Delimiter not found = :"<< align << endl;
        exit(0);
      }
      int firstSourcePos,lastSourcePos,firstTargetPos,lastTargetPos;
      string sourcePoss = align.substr(0, posDelimeter);
      string targetPoss = align.substr(posDelimeter+1);
      posDelimeter = sourcePoss.find("-");
      if(posDelimeter < string::npos) {
        firstSourcePos = atoi((sourcePoss.substr(0, posDelimeter)).c_str());
        lastSourcePos  = atoi((sourcePoss.substr(posDelimeter+1)).c_str());
      } else {
        firstSourcePos = atoi(sourcePoss.c_str());
        lastSourcePos = firstSourcePos;
      }
      posDelimeter = targetPoss.find("-");
      if(posDelimeter < string::npos) {
        firstTargetPos = atoi((targetPoss.substr(0, posDelimeter)).c_str());
        lastTargetPos  = atoi((targetPoss.substr(posDelimeter+1)).c_str());
      } else {
        firstTargetPos = atoi(targetPoss.c_str());
        lastTargetPos = firstTargetPos;
      }
      for (int i = firstSourcePos; i <= lastSourcePos; i++) {
        for (int j = firstTargetPos; j <= lastTargetPos; j++) {
          oss << i << "-" << j << " ";
        }
      }

    } //else case where two spaces ?
  }
  out = oss.str();
  //cout <<  "ConverttoStandard: " << out << endl;

  return out;
}

}

