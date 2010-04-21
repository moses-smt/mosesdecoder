/*
**********************************************************
Head file ---------- alignArray.h
Declaration of class alignArray
Store the word alignments for each sentence
Components:
1. align_FRtoEN --- the source to target alignemnt [source word Pos]->[target word Pos] (int, vector<int>)
2. align_ENtoFR --- the target to source alignemnt [target word Pos]->[source word Pos] (int, vector<int>)
Functions:
1. vector<int> getFRtoEN_alignment(int sourcePos) --- given the source position, return the corresponding alignments
                                                      if null, return vector {-1}
2. vector<int> getENtoFR_alignment(int targetPos) --- give the target position, return the corresponding alignements
                                                      if null, return vector {-1}
3. bool checkFRtoEN_alignment(int sourcePos) --- check the word is null alignment or not
4. bool checkENtoFR_alignment(int targetPos) --- check the word is null alignment or not

Special function:
1. alignArray() --- constructor,   create empty alignments
2. alignArray(string alignmentString) --- contructor, create the word alignment array using the word alignment strings
                                          the strings is sourcePos-targetPos, produced by GIZA++
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef ALIGNARRAY_H
#define ALIGNARRAY_H

#include <cstdlib>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
using namespace std;
using std::vector;
using std::string;
using std::istringstream;
typedef std::map<int, vector<int>, std::less<int> > alignDict; //shortname the map function

//alignArray class definition

class alignArray
{
      public:
             alignArray();                                                 //constructor (do nothing)
             alignArray(string alignmentString);                           //get the word alignments from the string
             vector<int> getFRtoEN_alignment(int sourcePos);               //return the corresponding target POSs for source word in source POS
             vector<int> getENtoFR_alignment(int targetPos);               //return the corresponding source POSs for target word in target POS
             bool checkFRtoEN_alignment(int sourcePos);                    //check the word is null alignment or not
             bool checkENtoFR_alignment(int targetPos);                    //check the word is null alignment or not
      private:
             alignDict align_FRtoEN;                                       //FRtoEN alignemnts
             alignDict align_ENtoFR;                                       //ENtoFR alignments
              
      }; //end class alignArray, remember the ";" after the head file~~~!
#endif
