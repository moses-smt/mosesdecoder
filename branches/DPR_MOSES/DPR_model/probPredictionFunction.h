/*
=======================================================================================================================
#Remark: the maximum length of a sentence is set as 200! All sentence that exceed this length will cause errors!
The phrase probability prediction function library, including the following functions:
1. smt_sourceClusterPrediction ---  For each source phrae cluster, read the W cluster and predict the reordering probabilities (normalised) for each instance
2. smt_createSourceCluster --- Given a test corpus, extract all source phrases (appear in the training set)
3. smt_collectPhraseOptions --- Create the phrase options for each test sentence
=======================================================================================================================
*/
#ifndef MAXCHAR
#define MAXCHAR 100
#endif

#ifndef PROBPREDICTIONFUNCTION_H
#define PROBPREDICTIONFUNCTION_H



#include <cstdlib>
#include <iostream>
#include<fstream> 
#include<sstream>        //using istringstream
#include <algorithm>     //using the algorithms e.g.~sort
#include <limits>
#include <math.h>
#include "phraseConstructionFunction.h"
#include "weightMatrix.h"
#include "sentencePhraseOption.h"
#include "phraseReorderingTable.h"
#include "phraseTranslationTable.h"

namespace std{ using namespace __gnu_cxx;}
using namespace std;
using std::istringstream; //to convert string into integers
using std::stringstream;  //to convert int to string
using std::ifstream;//input file stream
using std::ofstream;//output file stream


//define the structure for the phrase opition index (sentenceIndex, left_boundary, right_boundary)
struct phraseOptionIndex
{
       int sentenceIndex;
       unsigned short left_boundary;
       unsigned short right_boundary;  
       //use a constructor to assigned the values
       phraseOptionIndex(const int A, const unsigned short B, const unsigned short C):
                               sentenceIndex(A),left_boundary(B),right_boundary(C){}  
       //defne the operator "<" for the map function (compare the sentence key)
       bool operator<(const phraseOptionIndex& A) const   
       { return ((sentenceIndex < A.sentenceIndex) || (sentenceIndex == A.sentenceIndex && left_boundary<A.left_boundary) || (sentenceIndex == A.sentenceIndex && left_boundary==A.left_boundary && right_boundary<A.right_boundary)); } 

       }; 
       

typedef std::hash_map<string, map<phraseOptionIndex, unsigned long long> > sourcePositionMap;//, std::less<string> > sourcePositionMap;
typedef std::map<phraseOptionIndex, unsigned long long> phraseFeaturePositionMap;

//for using the class sentencePhraseOption
typedef std::map<string, map<unsigned short, vector<int> > > sourceTargetFeatureMap;
typedef std::map<unsigned short, vector<int> > targetFeatureMap;

//for using the class sentencePhraseOptionSTR
typedef std::hash_map<string, hash_map<string, vector<int> > > sourceTargetFeatureMapSTR;
typedef std::hash_map<string, vector<int> > targetFeatureMapSTR;

/******************************************************************************
A. Function:
For each source phrae cluster, read the W cluster and predict the reordering probabilities (normalised) for each instance
Input:
1. wt (weightCluster*) --- the weight cluster for the source phrase
2. sourceFeatureFile (ifstream&) --- the file store the source features
3. sourceFeaturePosition (phraseFeaturePositionMap) --- <sentence Index, boundary_left, boundary_right> -> feature start position
4. targetTranslation (target translation index->vecot<int>) --- the target translations for the source phrase (target features)
5. phraseOption (sentencePhraseOption*) --- update the phrase options
Output:
*.  phraseOption --- Update the phrase options
******************************************************************************/
void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFileName, phraseFeaturePositionMap sourceFeaturePosition, targetFeatureMap targetTranslation, sentencePhraseOption* phraseOption);


/******************************************************************************
A*. Function:
For each source phrae cluster, read the W cluster and predict the reordering probabilities (normalised) for each instance
-------------------------------------------------------------------------------
Different from A
1) the sourceFeaturePosition is: <boundary_left, boundary_right> -> features
-------------------------------------------------------------------------------
Input:
1. wt (weightCluster*) --- the weight cluster for the source phrase
2. sourceFeatureFile (ifstream&) --- the file store the source features
3. sourceFeaturePosition (phraseFeaturePositionMap) --- <boundary_left, boundary_right> -> feature start position
4. targetTranslation (string->vecot<int>) --- the target translations for the source phrase (target features)
5. phraseOption (sentencePhraseOptionSTR*) --- update the phrase options
Output:
*.  phraseOption --- Update the phrase options
******************************************************************************/
//void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFile, phraseFeaturePositionMap sourceFeaturePosition, targetFeatureMap targetTranslation, sentencePhraseOptionSTR* phraseOption);
void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFile, phraseFeaturePositionMap sourceFeaturePosition, sourceTargetFeatureMapSTR::const_iterator sourceTargetFound, sentencePhraseOptionSTR* phraseOption);


/******************************************************************************
B. Function:
Given a test corpus, extract all source phrases (appear in the training set)
Store the source features in a .txt file and return a sourcePositionMap dictionary
Input:
1. inputFileName (char*) --- the input source corpus
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseTranslationTable*) --- store the source->target phrases in the training set
13. outputFileName (char*) --- output the source features
                               Format: source string ||| sentence_index boundary_left boundary_right ||| features
14. sourcePositionDict (sourcePositionMap*) --- source phrase -> <sentence index boundary_left boundary_right> -> startPos for features
Output:
*. outputFileName (char*) --- output the source features
                               Format: source string ||| sentence_index boundary_left boundary_right ||| features
*. sourcePositionDict (sourcePositionMap*) --- source phrase -> <sentence index boundary_left boundary_right> -> startPos for features

******************************************************************************/
void smt_createSourceCluster(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseReorderingTable* trainingPhraseTable, char* outputFileName,  sourcePositionMap* sourcePositionDict);


/******************************************************************************
B*. (overloaded) Function:
Given a test corpus, extract all source phrases (appear in the training set)
Store the source features in a .txt file and return a sourcePositionMap dictionary
-------------------------------------------------------------------------------
B* different from B. only at input 12: using phraseTranslationTable to collect source-> target phrases
The corresponding smt_collectPhraseOptions is C*.
The corresponding smt_sourceClusterPrediction is A.
-------------------------------------------------------------------------------
Input:
1. inputFileName (char*) --- the input source corpus
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseTranslationTable*) --- store the source->target phrases in the training set
13. outputFileName (char*) --- output the source features
                               Format: source string ||| sentence_index boundary_left boundary_right ||| features
14. sourcePositionDict (sourcePositionMap*) --- source phrase -> <sentence index boundary_left boundary_right> -> startPos for features
Output:
*. outputFileName (char*) --- output the source features
                               Format: source string ||| sentence_index boundary_left boundary_right ||| features
*. sourcePositionDict (sourcePositionMap*) --- source phrase -> <sentence index boundary_left boundary_right> -> startPos for features

******************************************************************************/
void smt_createSourceCluster(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* outputFileName,  sourcePositionMap* sourcePositionDict);

/******************************************************************************
B**. (overloaded) Function:
Given a source test sentence, extract all source phrases (appear in the training set)
Store the source features in a .txt file and return a sourcePositionMap dictionary
-------------------------------------------------------------------------------
B** different from B. B*. Only deal with a sentence at each time;
The construction of the target phrase features are constructed by function: 
smt_collectPhraseOptions (C** overload function)
The corresponding smt_collectPhraseOptions is C**.
The corresponding smt_sourceClusterPrediction is A*.
-------------------------------------------------------------------------------
Input:
1. sourceSentence (string) --- the input source sentence 
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseTranslationTable*) --- store the source->target phrases in the training set
13. outputFileName (char*) --- output the source features
                               Format: source string ||| boundary_left boundary_right ||| features
14. sourcePositionDict (sourcePositionMap*) --- source phrase -> <boundary_left boundary_right> -> startPos for features
Output:
*. outputFileName (char*) --- output the source features
                               Format: source string ||| boundary_left boundary_right ||| features
*. sourcePositionDict (sourcePositionMap*) --- source phrase -> <boundary_left boundary_right> -> startPos for features

******************************************************************************/
void smt_createSourceCluster(string sourceSentence, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* outputFileName, sourcePositionMap* sourcePositionDict);


/******************************************************************************
C. Function:
Create the phrase options for each test sentence
phrase option format: sentenceIndex->[left_boundary,right_boundary]->target translations -> reordering probabilities
Input:
1. inputFileName (char*) --- the input source corpus
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseReorderingTable*) --- store the source->target phrases in the training set
13. weightFileName (char*) --- the .txt file of the weights
14. weightMatrix (weightMatrixW*) --- store the start position of each weight clusters

Output:
* phraseOption --- return the phrase options
******************************************************************************/
sentencePhraseOption smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseReorderingTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix);



/******************************************************************************
C*.(overloaded) Function:
Create the phrase options for each test sentence
phrase option format: sentenceIndex->[left_boundary,right_boundary]->target translations -> reordering probabilities
-------------------------------------------------------------------------------
* different from C. only at input 12: using phraseTranslationTable to collect source-> target phrases
-------------------------------------------------------------------------------
Input:
1. inputFileName (char*) --- the input source corpus
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseTranslationTable*) --- store the source->target phrases in the training set
13. weightFileName (char*) --- the .txt file of the weights
14. weightMatrix (weightMatrixW*) --- store the start position of each weight clusters
15. classSetup (int) --- the number of orientations
Output:
* phraseOption --- return the phrase options
******************************************************************************/
sentencePhraseOption smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix, int classSetup);

/******************************************************************************
C**.(overloaded) Function:
Create the phrase options for each test sentence
phrase option format: sentenceIndex->[left_boundary,right_boundary]->target translations -> reordering probabilities
-------------------------------------------------------------------------------
* different from C. and C*. 
1) do the sourceTargetFeatureMap in this function (get target features)
2) for smt_createSourceCluster (using B**) funciton, only given one input sentence to reduce the memory
3) the corresponding smt_sourceClusterPrediction function is A* (see the difference)
4) the output is on a .txt file directly to reduce the memory
-------------------------------------------------------------------------------

Input:
1. inputFileName (char*) --- the input source corpus
2. ngramDictFR (phraseNgramDict*) --- source word dictionary
3. ngramDictEN (phraseNgramDict*) --- target word dictionary
4. tagsDict_fr (phraseNgramDict*) --- source tag dictionary
5. tagsDict_en (phraseNgramDict*) --- target tag dictionary
6. wordDict_fr (wordClassDict*) --- source word class dictionary
7. wordDict_en (wordCLassDict*) --- target word class dictionary
8. maxPhraseLength (int) --- the max length of phrases to be extracted
9. maxNgramSize (int) --- the maximum size of the ngram dictionary
10. zoneConf (int array[2], zoneL and zoneR) --- the environment zone boundary 
11. relabelDict (relabelFeature*) --- relabel of the training features
12. trainingPhraseTable (phraseTranslationTable*) --- store the source->target phrases in the training set
13. weightFileName (char*) --- the .txt file of the weights
14. weightMatrix (weightMatrixW*) --- store the start position of each weight clusters
15. classSetup (int) --- the number of orientations
16. outPhraseOptionFileName (char*) --- store the output phrase options
Output:
* outPhraseOptionFileName --- store the sentence phrase options, output to a .txt file
******************************************************************************/
void smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix, int classSetup, char* outPhraseOptionFileName);



#endif
