/*
**********************************************************
Head file ---------- sentencePhraseOption.h
Declaration of class sentencePhraseOption
Store the phrase options (including target phrases and reordering probilities) for each sentence

Components:
1. phraseOption --- sentence phrase option. sentence index -> [position_left,position_right]->target phrase -> [reordering probabilities]
2. numSen --- the number of senences
              
Functions:
1. void createPhraseOption(int sentenceIndex, int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
1*. void createPhraseOption(int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
    Remark(*): only for one sentence so no need to store the sentenceIndex
2. vector<float> getPhraseProbs(int sentenceIndex, int phrase_boundary[], string targetPhrase, int numClass) --- get the phrase reordering probability
3. void outputPhraseOption(char* outputFileName) --- output all phrase options to a .txt file
3* void outputPhraseOption(ofstream& outputFile, int sentenceIndex, sentenceArray* sentence, phraseTranslationTable* trainingPhraseTable) --- output a sentence's phrase options to a .txt file
3* void outputPhraseOption(ofstream& outputFile) --- output all phrase options to a .txt ifle (only for a test sentence) 
4. int getNumSentence() --- get the number of sentences
                            
Special function:
1. sentencePhraseOption() --- constructor, create a phraseOption list
2. sentencePhraseOption(char* inputFileName) --- get the phrase options from a .txt file
***********************************************************
*/

//prevent multiple inclusions of head file
#ifndef SENTENCEPHRASEOPTION_H
#define SENTENCEPHRASEOPTION_H
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
#include "sentenceArray.h" //the sentence array class
#include "phraseTranslationTable.h" //the phrase translation table
#include "weightMatrix.h"

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif


namespace std{ using namespace __gnu_cxx;}
using namespace std;
using std::ifstream;
using std::ofstream;
using std::istringstream;



//for sentencePhraseOption
typedef std::map<int, map<vector<unsigned short>, map<unsigned short, vector<float> > >, std::less<int> > mapSentenceOption;
typedef std::map<vector<unsigned short>, map<unsigned short, vector<float> > > mapPhraseOption;
typedef std::map<unsigned short, vector<float> > mapTargetProbOption;

//for sentencePhraseOptionSTR
typedef std::map<int, map<vector<unsigned short>, hash_map<string, vector<float> > >, std::less<int> > mapSentenceOptionSTR;
typedef std::map<vector<unsigned short>, hash_map<string, vector<float> > > mapPhraseOptionSTR;
typedef std::hash_map<string, vector<float> > mapTargetProbOptionSTR;

//sentencePhraseOption class definition (only for extracting and output the sentence options)

class sentencePhraseOption
{
      public:
             sentencePhraseOption();           //create an empty phrase option list
             void createPhraseOption(int sentenceIndex, unsigned short phrase_boundary[], mapTargetProbOption targetProbs);//update the reordering probabilities for a phrase pair
             void createPhraseOption(unsigned short phrase_boundary[], mapTargetProbOption targetProbs);
             void outputPhraseOption(ofstream& outputFile, int sentenceIndex, sentenceArray* sentence, phraseTranslationTable* trainingPhraseTable);
             int getNumSentence();                         //get the number of sentences
             //~sentencePhraseOption();                      //destructor
      private:
              mapSentenceOption phraseOption; //store the phrase option
      protected:
              int numSen;                     //store the number of sentences
              
      }; //end class sentencePhraseOption, remember the ";" after the head file~~~!
      
      
/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/
     
      
class sentencePhraseOptionSTR : public sentencePhraseOption
{
      public:
      sentencePhraseOptionSTR();           //empty phrase options
      sentencePhraseOptionSTR(char* inputFileName);           //get the phrase options from a .txt file
      void outputPhraseOption(char* outputFileName); //output all phrase options to a .txt file
      void outputPhraseOption(ofstream& outputFile);
      void createPhraseOption(int sentenceIndex, unsigned short phrase_boundary[], mapTargetProbOptionSTR targetProbs);//update the reordering probabilities for a phrase pair
      void createPhraseOption(unsigned short phrase_boundary[], mapTargetProbOptionSTR targetProbs);       
      vector<float> getPhraseProbs(int sentenceIndex, unsigned short phrase_boundary[], string targetPhrase, int numClass);   //get the phrase reordering probability
             
      private:
             mapSentenceOptionSTR phraseOption;     
      };
#endif      
