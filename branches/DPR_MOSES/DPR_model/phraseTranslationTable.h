/*
**********************************************************
Head file ---------- phraseTranslationTable.h
Declaration of class phraseTranslationTable 
Store the source phrases and their translations (from a phrase table provided by Moses, or our extraction)
* Phrase table format:
  source phrase ||| target phrase ||| others (e.g.~probabilities on Moses and ngram features on our extraction)

Components:
1. phraseTranslationTable --- store source phrases -> target phrase, can extend the dictionary in further version
2. numCluster --- number of clusters (source phrase) 
3. numPhrasePair --- number of phrase pairs stored
              
Functions:
1. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
2. int getNumCluster() --- get the number of clusters (source phrases)
3. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
4. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
5. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations
Special function:
1. phraseTranslationTable() --- constructor,   create empty phraseReorderingTable
2. phraseTranslationTable(char* inputFileName) --- contructor, create the phraseTranslationTable by reading the input file (phrase table)
3. phraseTranslationTable(char* inputFileName, int maxTranslations) --- contructor, create the phraseTranslationTable by reading the input file (phrase table)
                                                                        also prune out the translations if it exceed the maxium number of the translations
4. phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB) --- constructor, create the phraseTranslationTable
                                                                                 only for the phrases that occur in the test phrase database
5. phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB, int maxTranslations) --- constructor, constructor, create the phraseTranslationTable
                                                                                 only for the phrases that occur in the test phrase database
                                                                                 also prune out the translations if it exceed the maxium number of the translations
***********************************************************
*/

//prevent multiple inclusions of head file
#ifndef PHRASETRANSLATIONTABLE_H
#define PHRASETRANSLATIONTABLE_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
#include <math.h>
#include <algorithm>
#include "corpusPhraseDB.h"
using namespace std;
using std::ifstream;
using std::istringstream;
using std::sort;

typedef std::map<string, vector<string>, std::less<string> > sourceTargetMap;
typedef std::map<string, vector<float>, std::less<string> > sourceTargetProbMap;

//phraseTranslationTable class definition


/*For the sort function, return the index of the vector, descend order*/
template<class T> struct index_cmp {
index_cmp(const T arr) : arr(arr) {}
bool operator()(const size_t a, const size_t b) const
{ return arr[a] > arr[b]; }
const T arr;
};



class phraseTranslationTable
{
      public:
             phraseTranslationTable();
             phraseTranslationTable(char* inputFileName);
             phraseTranslationTable(char* inputFileName,corpusPhraseDB* testPhraseDB);
             phraseTranslationTable(char* inputFileName, int maxTranslations);
             phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB, int maxTranslations);
             vector<string> getClusterNames();
             int getNumCluster();
             int getNumPhrasePair();
             vector<string> getTargetTranslation(string sourcePhrase);         //get the target translations
             int getNumberofTargetTranslation(string sourcePhrase);            //get the number of target translations
             
      private:
             sourceTargetMap phraseTable; //store the phrase pairs 
             int numCluster;              //number of examples
             int numPhrasePair;           //number of phrase pairs
             
      }; //end class phraseTranslationTable, remember the ";" after the head file~~~!
#endif
