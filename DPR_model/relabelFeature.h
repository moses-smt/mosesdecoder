/*
**********************************************************
Head file ---------- relabelFeature.h
Declaration of class relabelFeature
Store the relabeled features (to reduce the size of the feature expression)
Components:
1. featureRelabel --- the map function (int, int) original featureIndex -> relabeled feature index
2. countFeatureRelabel --- the number of relabeled features in this dictionary
Functions:
1. int insertFeature(int featureIndex) --- given a feature index, insert the feature if it doesn't exist in the dictionary and return the relabel index
2. int getRelabeledFeature(int featureIndex) --- given a feature inex, return its relabeled feature index
                                                 return -100 if it doesn't exist
3. int getNumFeature() --- return the number of relabeled features
4. void writeRelabelFeatures(char* dictFileName) --- write the relabel features
Special function:
1. relabelFeature() --- constructor,   create empty alignments, initialize countFeatureRelabel
2. relabelFeature(char* relabelFilename) --- contructor, read a relabel dictionary from a .txt file (for test corpus)
***********************************************************
*/


//prevent multiple inclusions of head file
#ifndef RELABELFEATURE_H
#define RELABELFEATURE_H

#include <cstdlib>
#include <map>           //map class-template definition, contain a pair
#include <cstring>        //the string operator
#include <vector>        //the vector operator
#include <fstream>       
using namespace std;
using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
typedef std::map<int, int, std::less<int> > relabelFeatureDict; //shortname the map function

//relabelFeature class definition

class relabelFeature
{
      public:
             relabelFeature();                                                 //constructor (initialze countFeatureRelabel)
             relabelFeature(char* relabelFilename);                           //read the relabel feature dict from .txt file
             int insertFeature(int featureIndex);                       // given a feature index, insert the feature if it doesn't exist in the dictionary
             int getRelabeledFeature(int featureIndex);                  //given a feature inex, return its relabeled feature index;               //return the corresponding source POSs for target word in target POS
             int getNumFeature();                                        //return the number of relabeled features
             void writeRelabelFeatures(char* dictFileName);                // write the relabel features
      private:
             relabelFeatureDict featureRelabel;                             //relabel dictionary
             int countFeatureRelabel;                                       //the number of relabeled features in this dictionary
              
      }; //end class relabelFeature, remember the ";" after the head file~~~!
#endif
