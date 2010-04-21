/*
=======================================================================================================================
#Remark: the maximum length of a sentence is set as 200! All sentence that exceed this length will cause errors!
The phrase construction function library, including the following functions:
1. smt_construct_phraseNgramDict --- Construct the dictionary (writing a .txt dictionary file, which can be read by a phraseNgramDict class)
1*. smt_construct_phraseNgramDict --- overloaded construct the dictionary (return the dictionary)
2. smt_construct_wordDict --- Read the word class labels and construct the tags corpus for training/test corpus (be read by a wordClassDict class)
2*. smt_construct_wordDict --- overloaded Read the word classes labels and return the wordClassDict
3. smt_extract_ngramFeature --- given the (source/target) sentence and the zone, extract ngram features in these environment (return a vector)
4. smt_consistPhrasePair --- extract phrase pairs from a sentence pair with its word alignments.
4*. smt_consistPhrasePair --- overloaded extract phrase pairs (only appear in the test set) from a sentence pair with its word alignments.
5. smt_constructPhraseReorderingDB --- extract phrase pairs with its reordering distance and ngram features from a corpus (pair)
                                       store then in an output file for further process
5*. smt_constructPhraseReorderingDB --- overloaded extract phrairs (only appear in the test set)
=======================================================================================================================
*/


#ifndef PHRASECONSTRUCTIONFUNCTION_H
#define PHRASECONSTRUCTIONFUNCTION_H


#include <cstdlib>
#include <iostream>
#include<fstream> 
#include<sstream>        //using istringstream
#include <algorithm>     //using the algorithms e.g.~sort
#include "phraseNgramDict.h" //include definition of class phraseNgramdict from phraseNgramDict.h
#include "wordClassDict.h" //include definition of class wordClassDict from wordClassDict.h
#include "alignArray.h" //include definition of class alignArray from alignArray.h
#include "relabelFeature.h" //include definition of class relabelFeature from relabelFeature.h
#include "sentenceArray.h" //include definition of class sentenceArray from sentenceArray.h
#include "corpusPhraseDB.h" //include definition of class corpusPhraseDB from corpusPhraseDB.h


using namespace std;
using std::istringstream; //to convert string into integers
using std::ifstream;//input file stream
using std::ofstream;//output file stream
using std::sort;     //algorithm sort
typedef std::map<vector<int>,int> statePhraseDict; //for extarcting the consistent phrase (used in smt_consistPhrasePair)
typedef std::map<int,int> intintDict; // (used in smt_consistPhrasePair)



/******************************************************************************
A. Function:
Construct the ngram dictionary for the source/target corpus (and tags corpus)
Input:
1.  inputCorpus --- the input corpus file (.txt)
2.  maxNgram --- the maximum length of the ngram features (default 3)
3.  minPrune --- prune the ngram features which occur < minPrune 
Output:
1.  ngramDictFile --- the output ngram dictionary file (.txt) 
******************************************************************************/

bool smt_construct_phraseNgramDict(char* inputCorpusFile, char* ngramDictFile, int maxNgram, int minPrune);


/******************************************************************************
A*. Function (overloaded):
Construct the ngram dictionary for the source/target corpus (and tags corpus)
Input:
1.  inputCorpus --- the input corpus file (.txt)
3.  maxNgram --- the maximum length of the ngram features (default 3)
4.  minPrune --- prune the ngram features which occur < minPrune 
5. overloadFlag --- (bool type value, for overloaded function only)
Output:
2.  ngramDictFile --- the output ngram dictionary file (.txt) 
******************************************************************************/

phraseNgramDict smt_construct_phraseNgramDict(char* inputCorpusFile, char* ngramDictFile, int maxNgram, int minPrune, bool overloadFlag);

/******************************************************************************
B. Function:
Construct the word class dictionary for the source/target corpus
Create the tags corpus for the source/target corpus
Input:
1.  wordClassDictFile --- the word class dictionary (.txt file, provided by mkcls or MOSES)
2.  inputCorpus ---  the input corpus (word corpus, .txt file)
Output:
3.  tagsCorpus --- the output tag corpus (.txt file)
******************************************************************************/

bool smt_construct_wordDict(char* wordClassDictFile, char* inputCorpus, char* tagsCorpus);

/******************************************************************************
B*. Function:
Overloaded Construct the word class dictionary for the source/target corpus
Create the tags corpus for the source/target corpus
return the word class dicitonary directly
Input:
1.  wordClassDictFile --- the word class dictionary (.txt file, provided by mkcls or MOSES)
2.  inputCorpus ---  the input corpus (word corpus, .txt file)
4.  overloadFlag --- (bool type value, for overloaded function only)
Output:
3.  tagsCorpus --- the output tag corpus (.txt file)
******************************************************************************/

wordClassDict smt_construct_wordDict(char* wordClassDictFile, char* inputCorpus, char* tagsCorpus, bool overloadFlag);


/******************************************************************************
C. Function:
Extract the ngram features around a source or target phrase
Input:
1. sentence - the source/target (words/tags) sentence (a sentence array pointer sentenceArray*)
2. ngramDict - the source/target (word/tags) ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
3. zoneL - the left boundary of the environment (int)
4. zoneR - the right boundary of the environment (int)
5. flag - 0: FR words left; 10 FR tags left; 1:FR words right; 11: FR tags right; 2:EN words; 22:EN tags;
6. maxNgramSize - the maximum size of the ngram features (default 3)
Output:
1.  ngramFeatures - the ngram features in the zone (vector<int>)
******************************************************************************/

vector<int> smt_extract_ngramFeature(sentenceArray* sentence, phraseNgramDict* ngramDictionary, int zoneL, int zoneR, int flag, int maxNgramSize);



/******************************************************************************
D. Function:
Extract all consistent phrase pairs upto length maxPhraseLength (from the alignments)
Time complexity O(N^2)
Input:
1. sentenceFR - the source sentence (a sentence array pointer sentenceArray*)
2. sentenceEN - the target sentence (a sentence array pointer sentenceArray*)
3. tagFR -  the source tag sentence (a sentence array pointer sentenceArray*)
4. tagEN -  the target tag sentence (a sentence array pointer sentenceArray*)
5. ngramDictFR - the source ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
6. ngramDictEN - the target ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
7. tagsDictFR - the source tags dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
8. tagsDcitEN - the target tags ditionary (a phraseNgramDict class pointer, phraseNgramDict*)
9. sentenceAlignment - the sentence alignment (an alignArray)
10. zoneConf - the environment zone boundary (int array[2], zoneL and zoneR)
11. maxPhraseLength - the maixmum length of the phrase extracted
12. maxNgramSize - the maximum length of the ngram features
13. featureRelabelDB - the dicitonary of the relabeled feature index (a relabelFeature class pointer, relabelFeature*)
14. fout - write an output file (ofstream& object, should pass reference)
Output:
fout --- write an output file (ofstream object)
         source phrase ||| target phrase ||| reordering distance ||| feature index
******************************************************************************/

void smt_consistPhrasePair(sentenceArray* sentenceFR, sentenceArray* sentenceEN, sentenceArray* tagFR, sentenceArray* tagEN, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, alignArray sentenceAlignment, int zoneConf[], int maxPhraseLength, int maxNgramSize, relabelFeature* featureRelabelDB, ofstream& fout);




/******************************************************************************
D*. Function (overload):
Extract all consistent phrase pairs (appeared in the test set) upto length maxPhraseLength (from the alignments)
Time complexity O(N^2)
Input:
1. sentenceFR - the source sentence (a sentence array pointer sentenceArray*)
2. sentenceEN - the target sentence (a sentence array pointer sentenceArray*)
3. tagFR -  the source tag sentence (a sentence array pointer sentenceArray*)
4. tagEN -  the target tag sentence (a sentence array pointer sentenceArray*)
5. ngramDictFR - the source ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
6. ngramDictEN - the target ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
7. tagsDictFR - the source tags dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
8. tagsDcitEN - the target tags ditionary (a phraseNgramDict class pointer, phraseNgramDict*)
9. sentenceAlignment - the sentence alignment (an alignArray)
10. zoneConf - the environment zone boundary (int array[2], zoneL and zoneR)
11. maxPhraseLength - the maixmum length of the phrase extracted
12. maxNgramSize - the maximum length of the ngram features
13. featureRelabelDB - the dicitonary of the relabeled feature index (a relabelFeature class pointer, relabelFeature*)
14. fout - write an output file (ofstream& object, should pass reference)
15. testPhraseDB - the source phrase dictionary in the test set (corpusPhraseDB* pointer).
Output:
fout --- write an output file (ofstream& object, should pass reference)
         source phrase ||| target phrase ||| reordering distance ||| feature index
******************************************************************************/

void smt_consistPhrasePair(sentenceArray* sentenceFR, sentenceArray* sentenceEN, sentenceArray* tagFR, sentenceArray* tagEN, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, alignArray sentenceAlignment, int zoneConf[], int maxPhraseLength, int maxNgramSize, relabelFeature* featureRelabelDB, ofstream& fout, corpusPhraseDB* testPhraseDB);


/******************************************************************************
E. Function:
Extract all consistent phrase pairs with their reordering distance and ngram features
Calling smt_consistPhrasePair()
Input:
1. sourceCorpusFile --- the source word corpus (.txt file)
2. targetCorpusFile --- the target word corpus (.txt file)
3. wordAlignmentFile --- the word alginment file (produced by GIZA++, store as "aligned.grow-diag-final-and"
4. tagsSourceFile --- the source tags corpus (.txt file)
5. tagsTargetFile --- the target tags corpus (.txt file)
6. phraseDBFile --- output the phrase pairs with the reordering distance and ngram features (.txt file)
7. ngramDictFR --- the source ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
8. ngramDictEN --- the target ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
9. tagsDictFR --- the source tags dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
10. tagsDcitEN --- the target tags ditionary (a phraseNgramDict class pointer, phraseNgramDict*)
11. zoneConf --- the environment zone boundary (int array[2], zoneL and zoneR)
12. maxPhraseLength --- the maixmum length of the phrase extracted
13. maxNgramSize - the maximum length of the ngram features
14. featureRelabelDBFile --- the dicitonary of the relabeled feature index (.txt file) 
Output:
phraseDBFile --- write an output file (ofstream object)
         source phrase ||| target phrase ||| reordering distance ||| feature index
******************************************************************************/
void smt_constructPhraseReorderingDB(char* sourceCorpusFile, char* targetCorpusFile, char* wordAlignmentFile, char* tagsSourceFile, char* tagsTargetFile, char* phraseDBFile, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, int zoneConf[], int maxPhraseLength, int maxNgramSize, char* featureRelabelDBFile);




/******************************************************************************
E*. Function (overloaded):
Extract all consistent phrase pairs (only appear in the test set) with their reordering distance and ngram features
Calling smt_consistPhrasePair()
Input:
1. sourceCorpusFile --- the source word corpus (.txt file)
2. targetCorpusFile --- the target word corpus (.txt file)
3. wordAlignmentFile --- the word alginment file (produced by GIZA++, store as "aligned.grow-diag-final-and"
4. tagsSourceFile --- the source tags corpus (.txt file)
5. tagsTargetFile --- the target tags corpus (.txt file)
6. phraseDBFile --- output the phrase pairs with the reordering distance and ngram features (.txt file)
7. ngramDictFR --- the source ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
8. ngramDictEN --- the target ngram dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
9. tagsDictFR --- the source tags dictionary (a phraseNgramDict class pointer, phraseNgramDict*)
10. tagsDcitEN --- the target tags ditionary (a phraseNgramDict class pointer, phraseNgramDict*)
11. zoneConf --- the environment zone boundary (int array[2], zoneL and zoneR)
12. maxPhraseLength - the maixmum length of the phrase extracted
13. maxNgramSize - the maximum length of the ngram features
14. featureRelabelDBFile --- the dicitonary of the relabeled feature index (.txt file) 
15. testFileName --- the test file (.txt file)
Output:
phraseDBFile --- write an output file (ofstream object)
         source phrase ||| target phrase ||| reordering distance ||| feature index
******************************************************************************/
void smt_constructPhraseReorderingDB(char* sourceCorpusFile, char* targetCorpusFile, char* wordAlignmentFile, char* tagsSourceFile, char* tagsTargetFile, char* phraseDBFile, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, int zoneConf[], int maxPhraseLength, int maxNgramSize, char* featureRelabelDBFile, char* testFileName);

#endif
