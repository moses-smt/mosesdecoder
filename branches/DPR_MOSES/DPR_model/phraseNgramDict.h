/*
**********************************************************
Head file ---------- phraseNgramDict.h
Declaration of class phraseNgramDict
Store the phrase ngram features
Components:
1. phraseDict - map (function), store the phrase (ngrams), feature index, lengh of the ngram and the number of occurances
2. readDictCheck - return 0 if the phraseNgramDict can't read a dictionary file (can't find) or 1 otherwise;
3. ngramIndex - the ngram index (used when construct the dictionary)
Functions:
1. void insertNgram (string key, vector<int> values) -- insert the key, if it already exists, update the occurance
2. void deleteNgram (string key)  --- delete the key
3. bool findNgram (string key)  --- if the key is in the dictionary (return bool 1 or -1)
4. int getNgramIndex (string key) --- if the key exists, return the index, else return -1;
5. int getNgramOccurance (string key) --- if the key exists, return the occurance, else return -1;
6. int getNgramLength(string key) --- if the key exists, return the length of this ngram, else return -1;
7. vector<int> getNgramItems(string key) --- if the key exists, return index,length,occurance, else return {-1,-1,-1}
8. void outputNgramDict(char* filename, int minimalOccurenceCut) ---- output the dictionary into a file (.txt file)
9. bool checkReadFileStatus() --- (if it contains a file name), check whether the read of the dictionary is sucessful.
10. int getNumFeature() --- get the number of features in this dictionary.
Special function:
1. phraseNgramDict(char* fileName) --- constructor,  read the dictionary from the file
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef PHRASENGRAMDICT_H
#define PHRASENGRAMDICT_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <string>        //the string operator
#include <vector>        //the vector operator
#include<fstream>        //file strem
/*
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

//********************************************************************
//The following definition is for the use of hash_map with string key
namespace __gnu_cxx
{
        template<> struct hash< std::string >
        {
                size_t operator()( const std::string& x ) const
                {
                        return hash< const char* >()( x.c_str() );
                }
        };
}
//********************************************************************
namespace std{ using namespace __gnu_cxx;}*/
using namespace std;
using std::istringstream; 
using std::ifstream;//input file stream
using std::ofstream;//output file stream
using std::vector;
using std::string;
typedef std::map<string, vector<int> > phraseDict; //std::less<string> > phraseDict; //shortname the map function

//phraseNgramDict class definition

class phraseNgramDict
{
      public:
             phraseNgramDict(char* dictFileName);               //constructor (read a dictionary file)
             phraseNgramDict();                                  //constructor
             void insertNgram(string key, int ngramLength); //insert new ngram features
             void deleteNgram(string key);                     //delete a ngram feature
             int getNgramIndex(string key);                    //get the feature index
             int getNgramOccurance(string key);                //get the feature occurance
             int getNgramLength(string key);                   //get the length of the ngram
             vector<int> getNgramItems(string key);            //get the items (index, feature, length) of the ngram feature
             bool findNgram(string key);                       //find whether the key exists or not
             bool checkReadFileStatus();                        //check the read of a dictionary
             void outputNgramDict(char* dictFileName,int minOccurenceCut);          //output the dictionary into a file
             int getNumFeature();                                                   //get the number of features in this dictionary
      private:
             phraseDict ngramDict;                             //the phrase ngram dictionary
             bool readDictCheck;                                //the read dictionary file flag
             int ngramIndex;                                    //the ngramIndex (when construct)
              
      }; //end class phraseNgramDict, remember the ";" after the head file~~~!
#endif
