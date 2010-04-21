/*
**********************************************************
Head file ---------- wordClassDict.h
Declaration of class wordClassDict
Store the word class labels for each word
Components:
1. wordClassDictionary - map (function), store the word (string) and the word class index (int)
2. readDictCheck - return 0 if the phraseNgramDict can't read a dictionary file (can't find) or 1 otherwise;
3. numWords - the number of words in this corpus
Functions:
1. int getWordClass(string word) --- get the word class index (integer)
2. void createWCFile(char* inputFile,char* outputFile) --- generate word class files for the training/test corpus,
                                                           basing on the dictionary the objects hold.
3. bool checkReadFileStatus() --- check the read of the dictionary
4. int getNumWords()  --- get the vocaburary of the corpus
Special function:
1. wordClassDict(char* fileName) --- constructor,  read the word class dictionary from the file
                                     the word dictionary is provided by MOSES, in fr.vcb.classes (or en.vcb.classes)
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef WORDCLASSDICT_H
#define WORDCLASSDICT_H

#include <cstdlib>
#include <iostream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
#include<fstream>        //file strem
#include<sstream>        //using istringstream
using namespace std;
using std::istringstream; //to convert string into integers
using std::ifstream;//input file stream
using std::ofstream;//output file stream
using std::vector;
using std::string;
typedef std::map<string, int, std::less<string> > wordDict; //shortname the map function

//wordClassDict class definition

class wordClassDict
{
      public:
             wordClassDict(char* dictFileName);                           //constructor (read a dictionary file)
             bool checkReadFileStatus();                                    //check the read of a dictionary
             void createWCFile(char* inputFile,char* outputFile);          //output the dictionary into a file
             int getNumWords();                                            //get the vocaburary of the corpus
             int getWordClass(string word);                                //get the index of the words
      private:
             wordDict wordClassDictionary;                             //the word class dictionary
             bool readDictCheck;                                //the read dictionary file flag
             int numWords;                                    //the number of words in this dictionary
              
      }; //end class wordClassDict, remember the ";" after the head file~~~!
#endif
