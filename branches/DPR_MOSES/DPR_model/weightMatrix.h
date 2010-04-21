/*
**********************************************************
Head file ---------- weightMatrix.h

There are two classes
---------------------------------------------------------------------------------------------------------------
*. weightMatrixW
Components:
1. weightMatrix --- store the source phrase -> start position of the cluster values in a weightMatrix.txt file
2. numCluster --- number of clusters (source phrase) 

Functions:
1. int getNumCluster() --- get the number of clusters
2. void writeWeightMatrix(char* outputFileName) --- write the weight matrix (source phrase ||| start position) in a .txt file
3. void insertWeightCluster(string sourcePhrase, unsigned long long startPos) --- insert the start position of a new weight cluster
4. unsigned long long getWeightClusterPOS(string sourcePhrase) --- get the start position of a weight cluster given a source phrase

Special function:
1. weightMatrixW() --- create an empty dictionary
2. weightMatrixW(char* inputFileName) --- get the source phrase and the start position of the each source clusters
                                          Format: source phrase ||| start position in a .txt file
---------------------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------------------
*. weightClusterW --- (vector<map(int,float)>) store orientation class -> ngram features -> values
Components:
1. weightCluster --- store orientation -> ngram features -> values
2. numOrientation --- the number of orientation classes
3. sourcePhrase --- the cluster name (string)
4. distMatrix[5][5] --- the distance matrix for the structure learning (pre-defined when constructing the cluster)
Functions:
1. int getNumOrientation() --- get the number of orientation classes
2. string getClusterName() --- get the name of the source phrase
3. unsigned long long writeWeightCluster(ofstream& outputFile) --- write the weight matrix and return the number of characters written
                                                                         Format: source phrase '\n'; ngram features values (each line) '\n';
4. void getWeightCluster(ifstream& inputFile,  int numClass, unsigned long long startPos) --- get the weight cluster from a .txt file
5. void structureLearningW(vector<vector<int> > phraseTable, int maxRound, float step, float eTol) --- learn the weight cluster W
                                                                <vector<vector<int> > store the examples, in each vector<int>
                                                                first one is orientation and the others are ngram features
6. vector<float> structureLearningConfidence(vector<int> featureList) --- return the confifence W^{T}phi(x) for each class
6*. vector<float> structureLearningConfidence(vector<int> sourceFeature, vector<int> targetFeature) --- (overloaded function) return the confifence W^{T}phi(x) for each class
Special function:
1. weightClusterW(string sourcePhrasse, int numClass) --- create an empty weight cluster W with numOrientation classes
2. weightClusterW(ifstream& inputFile,  int numClass, unsigned long long startPos) --- get the weight cluster from a .txt weight file
---------------------------------------------------------------------------------------------------------------

*Remark:
        There are only two class setup:
        A. three-class setup: d<0; d=0; d>0.
        B. five-class setup: d<=-5; -5<d<0; d=0; 0<d<5; d>=5.
*The user can implement new class setup on his own.
***********************************************************
*/

//prevent multiple inclusions of head file
#ifndef WEIGHTMATRIX_H
#define WEIGHTMATRIX_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
#include <limits>
#include <algorithm>
#include <math.h>
#include "phraseNgramDict.h"

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
namespace std{ using namespace __gnu_cxx;}
using namespace std;

using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::random_shuffle;
using std::hash_map; //hash_map is faster than map when call? See and compare

typedef std::hash_map<string, unsigned long long> weightMatrixMap;//, std::less<string> > weightMatrixMap;
typedef std::hash_map<int,float> weightClusterMap;

//typedef std::hash_map<string, vector<int> > targetFeatureMapSTR;
//weightMatrixW class definition
class weightMatrixW
{
      public:
             weightMatrixW();                                     //reate an empty dictionary
             weightMatrixW(char* inputFileName);                  
             int getNumCluster();                                 //get the number of clusters
             void writeWeightMatrix(char* outputFileName);       //write the weight matrix (source phrase ||| start position) in a .txt file
             void insertWeightCluster(string sourcePhrase, unsigned long long startPos);             //insert the start position of a new weight cluster
             unsigned long long getWeightClusterPOS(string sourcePhrase);          //get the start position of a weight cluster given a source phrase             
      private:
             weightMatrixMap weightMatrix;              //store the source phrase -> start position of the cluster values in a weightMatrix.txt file
             int numCluster;                 //number of clusters (source phrase)
              
      }; //end class weightMatrixW, remember the ";" after the head file~~~!
      
      
/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/
      
//weightClusterW class definition
class weightClusterW
{
      public:
            weightClusterW(string source, int numClass);                      //construct an empty weight cluster
            weightClusterW(ifstream& inputFile,  int numClass, unsigned long long startPos);        //read a weight cluster
            int getNumOrientation();                                                 //get the number of classes
            string getClusterName();                                                 //get the name of the source phrase
            unsigned long long writeWeightCluster(ofstream& outputFile);                            //write the weight cluster
            void getWeightCluster(ifstream& inputFile,  int numClass, unsigned long long startPos); //read the weight clusters
            void structureLearningW(vector<vector<int> > phraseTable, int maxRound, float step, float eTol);         //update the W using structure learning algorithm
            vector<float> structureLearningConfidence(vector<int> featureList);   //return the confifence W^{T}phi(x) for each class            
            vector<float> structureLearningConfidence(vector<int> sourceFeature, vector<int> targetFeature);//targetFeatureMapSTR::const_iterator targetFound);//
      private:
             vector<weightClusterMap> weightCluster;     //store orientation -> ngram features -> values
             int numOrientation;                         //number of orientation classes
             string sourcePhrase;                        //the source phrase
             float distMatrix[5][5];                     //define the distance matrix (for structure learning)  
              
      }; //end class weightClusterW, remember the ";" after the head file~~~!
#endif

