/*
**********************************************************
Cpp file ---------- sentenceArray.cpp
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

#include "sentenceArray.h"

/*
1. Constructor, initialize the sentence array or read a sentence from a sentence string
*/
sentenceArray::sentenceArray()
{
    sentenceLength=0;
                              }
                              
sentenceArray::sentenceArray(string sentenceString)
{
    char tempWord[100];  //store the word (create size 200 char to store words, avoid potential error)
    sentenceLength=0;
    istringstream tempString(sentenceString);
    while(tempString>>tempWord)
    {
        sentence[sentenceLength]=tempWord; //Get the word
        sentenceLength++;
        }
    }


sentenceArray::sentenceArray(string sentenceString, wordClassDict* wordDict)
{
    char tempWord[100];  //store the word (create size 200 char to store words, avoid potential error)
    sentenceLength=0;
    istringstream tempString(sentenceString);
    while(tempString>>tempWord)
    {
        int wordClassIndex=wordDict->getWordClass(tempWord);
        stringstream wordClassIndexStr;
        wordClassIndexStr<<wordClassIndex;
        sentence[sentenceLength]=wordClassIndexStr.str(); //Get the word
        sentenceLength++;
        }
    }



/*
2. string getPhraseFromSentence(int startPos);                     //return the word sentence[startPos];
*/
string sentenceArray::getPhraseFromSentence(int startPos)
{
       if (startPos>=sentenceLength)
       {
          cerr<<"The phrase zone exceeds the right boundary of the sentence!\n";
          startPos=sentenceLength-1;
          }
       else if (startPos<0)
          {
               cerr<<"The phrase zone exceeds the left boundary of the sentence!\n";
               startPos=0;
               }
       return sentence[startPos];
       }
       
/*
3. string getPhraseFromSentence(int startPos, int endPos);                     //return the phrase sentence[startPos:endPos]
*/
string sentenceArray::getPhraseFromSentence(int startPos, int endPos)
{
       if (endPos>=sentenceLength)
       {
          cerr<<"The phrase zone exceeds the right boundary of the sentence!\n";
          endPos=sentenceLength-1;
          }
       else if (startPos<0)
          {
               cerr<<"The phrase zone exceeds the left boundary of the sentence!\n";
               startPos=0;
               }
          
       string tempPhrase="";
       for (int k=startPos;k<endPos;k++)
       {
           tempPhrase+=sentence[k]; //add the word
           tempPhrase+=" ";         //add the space
           }
       tempPhrase+=sentence[endPos];
       return tempPhrase;
       }
/*       
4. int getSentenceLength();                                        //return the sentence string
*/

int sentenceArray::getSentenceLength()
{return sentenceLength;}


