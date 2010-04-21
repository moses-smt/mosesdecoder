/*
**********************************************************
Head file ---------- sentenceArray.h
Declaration of class sentenceArray
Store the words for a sentence
Components:
1. sentence --- store the sentence array (string array)
2. sentenceLengh --- store the sentence length (int)
Functions:
1. string getPhraseFromSentence(int startPos, int endPos) --- get a phrase from the sentence
2. string getPhraseFromSentence(int startPos) --- (overload function) get a word from the sentence
3. int getSentenceLength() --- return the length of the sentence

Special function:
1. sentenceArray() --- constructor,   create an empty sentence array
2. sentenceArray(string sentenceString) --- contructor, create the sentence array using sentence string
3. sentenceArray(string sentenceString, wordClassDict* wordDict) --- constructor, create the tags for a sentence
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef SENTENCEARRAY_H
#define SENTENCEARRAY_H

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include "wordClassDict.h" //the word class
using namespace std;
using std::string;
using std::istringstream;
using std::stringstream;


//sentenceArray class definition

class sentenceArray
{
      public:
             sentenceArray();                                                 //constructor (do nothing)
             sentenceArray(string sentenceString);                           //get the words from the sentence string
             sentenceArray(string sentenceString, wordClassDict* wordDict);  //get the wrods and transform them to tags
             string getPhraseFromSentence(int startPos, int endPos);         //return the phrase sentence[startPos:endPos]
             string getPhraseFromSentence(int startPos);                     //return the word sentence[startPos];
             int getSentenceLength();                                        //return the sentence string
      private:
             string sentence[200];                                             //the sentence array
             int sentenceLength;                                             //return the sentence length
              
      }; //end class sentenceArray, remember the ";" after the head file~~~!
#endif
