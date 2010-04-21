/*
**********************************************************
Head file ---------- corpusPhraseDB.h
Declaration of class corpusPhraseDB
Store the phrases that appear in the train/test sentences
Components:
1. phraseDB --- store the phrases that appear in the train/test sentences
2. numPhrase --- the number of phrases
3. maxPhraseLength --- the max phrase length in this phrase DB
Functions:
1. bool checkPhraseDB(string phrase) --- check whether the phrase appear in the phrase DB or not
2. int getNumPhrase() --- return the number of phrases
3. int getMaxPhraseLength() --- return the maximum length of the phrases
4. void outAllPhrases(char* outFileName) --- output all phrases in to a .txt file; phrase ||| phraseIndex
Special function:
1. corpusPhraseDB() --- constructor,   create an empty phrase DB
2. corpusPhraseDB(char* inFileName, int MAXPLENGTH) --- create the phrase DB for the input corpus
3. corpusPhraseDB(char* inFileName, int MAXPLENGTH, bool readDict) --- create the phrase DB for the input phrase dctionary
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef CORPUSPHRASEDB_H
#define CORPUSPHRASEDB_H

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator

#include "sentenceArray.h" //for creating the sentence array
using namespace std;
using std::string;
using std::ifstream;
using std::ofstream;
using std::istringstream;

typedef std::map<string,int> stringintDict; //store the phrase (for test/training corpus) dictionary

//corpusPhraseDB class definition

class corpusPhraseDB
{
      public:
             corpusPhraseDB();                                                 //constructor (do nothing)
             corpusPhraseDB(char* inFileName, int MAXPLENGTH);                 //create the phrase DB for the input corpus
             corpusPhraseDB(char* inFileName, int MAXPLENGTH, bool readDict);  // read the phrase DB from the input phrase dctionary
             bool checkPhraseDB(string phrase);                                // check whether the phrase appear in the phrase DB or not
             int getNumPhrase();                                               //return the number of phrases
             int getMaxPhraseLength();                                         //return the maximum length of the phrases
             void outAllPhrases(char* outFileName);                            //output all phrases in to a .txt file; phrase ||| phraseIndex
      private:
             stringintDict phraseDB;                                             //the phrase DB
             int numPhrase;                                                      //return the number of phrases
             int maxPhraseLength;                                                //return the max length of the phrase
              
      }; //end class sentenceArray, remember the ";" after the head file~~~!
#endif

