/*
**********************************************************
Cpp file ---------- corpusPhraseDB.cpp
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
***********************************************************
*/


#include "corpusPhraseDB.h"

/*
1. constructor
*/

corpusPhraseDB::corpusPhraseDB()
{
    numPhrase=0;
    maxPhraseLength=0;
    }
    
corpusPhraseDB::corpusPhraseDB(char* inFileName, int MAXPLENGTH)
{
    //1. initilisation
    maxPhraseLength=MAXPLENGTH;
    numPhrase=0;
    stringintDict::iterator phrase_found; //to find the phrase in phraseDB
    string tempSentence;
    ifstream inputSentence(inFileName,ios::in); //get the input corpus
    
    //2. for each sentence
    if (!inputSentence.is_open())
    {
        cerr<<"Can't open the input file!";
        }
    else
    {
        while (getline(inputSentence,tempSentence,'\n'))
        {
              //2.1 create the sentence array
              sentenceArray tempSentenceArray(tempSentence);
              int sentenceLength= tempSentenceArray.getSentenceLength();
              
              //2.2 get each ngram (up to length maxPhraseLength)
              for (int ngram=1;ngram<=maxPhraseLength;ngram++)
              {
                  for (int i=0;i<=sentenceLength-ngram;i++)
                  {
                      string tempString=tempSentenceArray.getPhraseFromSentence(i,i+ngram-1); //get the phrase
                      phrase_found=phraseDB.find(tempString);
                      //2.3 if the ngram is new, put it into phraseDB
                      if (phrase_found==phraseDB.end())
                      {
                          numPhrase++; //update the number of phrase
                          phraseDB[tempString]=numPhrase;//set the phrase index
                          }
                      }
                  } 
              }
        
        }
    //3. close the input stream
    inputSentence.close();
    
    }
    
//May have better choice of overloaded function? readDict not used    
corpusPhraseDB::corpusPhraseDB(char* inFileName, int MAXPLENGTH, bool readDict)
{
    //1. initilisation
    maxPhraseLength=MAXPLENGTH;
    numPhrase=0;
    ifstream inputPhraseDB(inFileName,ios::in); //get the input corpus
    string tempSentence;
    
    //2. for each sentence
    if (!inputPhraseDB.is_open())
    {
        cerr<<"Can't open the input file!";
        }
    else
    {
        while (getline(inputPhraseDB,tempSentence,'\n'))
        {
              numPhrase++;
              size_t temp_found=tempSentence.find(" |||");
              //2.1 Get the phrase
              string phraseFeature=tempSentence.substr(0,temp_found);
              //2.2 Get the phrase index
              istringstream tempSubstring(tempSentence.substr(temp_found+4));
              int tempIndex;
              tempSubstring>>tempIndex;
              //2.3 update the phraseDB
              phraseDB[phraseFeature]=tempIndex;
              }
        
        }
    //3. close the input stream
    inputPhraseDB.close();
    } 
 
   
/*
2. bool checkPhraseDB(string phrase) --- find the phrase in the phraseDB
*/

bool corpusPhraseDB::checkPhraseDB(string phrase)
{
     stringintDict::iterator phrase_found=phraseDB.find(phrase);
     if (phrase_found==phraseDB.end())
        return false; //not found the phrase in phraseDB
     else
         return true;
     }

/*
3. int getNumPhrase() --- return the number of phrases
*/
int corpusPhraseDB::getNumPhrase()
{return numPhrase;}

/*
4. int getMaxPhraseLength()  --- get the max length of the phrase
*/

int corpusPhraseDB::getMaxPhraseLength()
{return maxPhraseLength;}

/*
5. void outAllPhrases(char* outFileName) --- output all phrases in to a .txt file; phrase ||| phraseIndex
*/
void corpusPhraseDB::outAllPhrases(char* outFileName)
{
     //1. initialisation
     ofstream fout(outFileName,ios::out); 
     //2. for each phrase, output phrase ||| phrase index
     for (stringintDict::const_iterator phrase_found=phraseDB.begin();phrase_found!=phraseDB.end();phrase_found++)
     {
         fout<<phrase_found->first<<" ||| "<<phrase_found->second<<'\n';
         }
     fout.close();
     }
