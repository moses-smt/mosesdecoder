/*
**********************************************************
Head file ---------- phraseReorderingTable.h
Declaration of class phraseReorderingTable (and sourceReorderingTable)
Store the phrase pairs with their reordering distance (orientation class)
*Remark:
        There are only two class setup:
        A. three-class setup: d<0; d=0; d>0.
        B. five-class setup: d<=-5; -5<d<0; d=0; 0<d<5; d>=5.
*The user can implement new class setup on his own (on function createOrientationClass()).
Components:
1. phraseReorderingTable --- store source phrase -> target phrase -> vector<int> [orientation class; start pos in phraseDB]
                             the second item is to retrive the ngram features from phraseDB.txt
1*. sourceReorderingTable --- store source phrase -> vector<int> [orientation class; start pos in phraseDB]
                             the second item is to retrive the ngram features from phraseDB.txt
2. numCluster --- number of clusters (source phrase) 
3. numPhrasePair --- number of phrase pairs stored
4. vector<unsigned long long> positionIndex; //store the start position of the ngram features for each phrase pair (each line)
              
Functions:
1. int createOrientationClass(int dist,int classSetup) --- the create the orientation class
2. map<string, vector<int>> getClusterMember(string sourcePhrase) --- get the target phrases and ngram features for a source phrase
2*. int getClusterMember(string sourcePhrase) --- get the number of examples in this cluster
3. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
4. int getNumCluster() --- get the number of clusters (source phrases)
5. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
6. int getNumOrientatin() --- get the class setup
7. vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile) --- get the examples with the ngram features (store in vector)
8. vector<unsigned long long> getPositionIndex() --- get the position index of all phrase pairs
9. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
10. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations

Special function:
1. phraseReorderingTable() --- constructor,   create empty phraseReorderingTable
2. phraseReorderingTable(char* inputFileName, int classSetup, int distCut) --- contructor, create the phraseReorderingTable
Remark: if the reordering distance exceed distCut, don't use this phrase pair (to avoid some word alignment errors)
***********************************************************
*/

//prevent multiple inclusions of head file
#ifndef PHRASEREORDERINGTABLE_H
#define PHRASEREORDERINGTABLE_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
using namespace std;
using std::ifstream;
using std::istringstream;

typedef std::map<string, map<string,vector<int> >,std::less<string> > mapmap;
typedef std::map<string, vector<int>, std::less<string> > mapDict;

//phraseReorderingTable class definition

class phraseReorderingTable
{
      public:
             phraseReorderingTable();
             phraseReorderingTable(char* inputFileName, int classSetup, int distCut);
             int createOrientationClass(int dist,int classSetup);
             mapDict getClusterMember(string sourcePhrase);
             vector<string> getClusterNames();
             int getNumCluster();
             int getNumPhrasePair();
             int getNumOrientatin();
             vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile);
             vector<unsigned long long> getPositionIndex(); //get the position index of all phrase pairs
             vector<string> getTargetTranslation(string sourcePhrase);         //get the target translations
             int getNumberofTargetTranslation(string sourcePhrase);            //get the number of target translations
             
      private:
             mapmap phraseTable; //store the phrase pairs with the orientation classes and the ngram features (position)
             int numCluster;
             int numPhrasePair;
             int numOrientation;
             vector<unsigned long long> positionIndex; //store the start position of the ngram features for each phrase pair (each line)
              
      }; //end class phraseReorderingTable, remember the ";" after the head file~~~!
      
      

/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/

     
/*
This class can be seen as an inherit class from phraseReorderingTable, will do it in the second version
*/
//sourceReorderingTable class definition

class sourceReorderingTable
{
      public:
             sourceReorderingTable();
             sourceReorderingTable(char* inputFileName, int classSetup, int distCut);
             int createOrientationClass(int dist,int classSetup);
             int getClusterMember(string sourcePhrase);
             vector<string> getClusterNames();
             int getNumCluster();
             int getNumPhrasePair();
             int getNumOrientatin();
             vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile);
             vector<unsigned long long> getPositionIndex(); //get the position index of all phrase pairs
             
      private:
             mapDict phraseTable; //store the source phrase with the orientation classes and the ngram features (position)
             int numCluster;
             int numPhrasePair;
             int numOrientation;
             vector<unsigned long long> positionIndex; //store the start position of the ngram features for each phrase pair (each line)
              
              
      }; //end class sourceReorderingTable, remember the ";" after the head file~~~!
#endif

