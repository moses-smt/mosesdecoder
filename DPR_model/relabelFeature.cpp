/*
**********************************************************
Cpp file ---------- relabelFeature.cpp
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

#include "relabelFeature.h" //include definition of class relabelFeature from relabelFeature.h

/*
1. relabelFeature constructor
*/

relabelFeature::relabelFeature()
{countFeatureRelabel=0;}

relabelFeature::relabelFeature(char* relabelFilename)
{
    //1. initialisation
    countFeatureRelabel=0;
    ifstream inputFeatureDict(relabelFilename,ios::in); //get the relabel feature file
    int featureIndex; //original feature index
    int relabelFeatureIndex; //the relabeled feature index
    while (inputFeatureDict>>featureIndex>>relabelFeatureIndex)
    {
          featureRelabel[featureIndex]=relabelFeatureIndex; //insert the pair
          countFeatureRelabel++;                            //update number of items
          }
          
    //2. close the .txt file
    inputFeatureDict.close();
    
    }

/*
2. int insertFeature(int featureIndex) --- insert the featureIndex if it doesn't exist and return the relabel index
*/

int relabelFeature::insertFeature(int featureIndex)
{
    relabelFeatureDict::iterator feature_found=featureRelabel.find(featureIndex);
    if (feature_found==featureRelabel.end())
       countFeatureRelabel++;
       featureRelabel[featureIndex]=countFeatureRelabel;
       
    return countFeatureRelabel;
    }

/*
3. int getRelabeledFeature(int featureIndex) --- get relabeled feature index (-100 if not exist)
*/

int relabelFeature::getRelabeledFeature(int featureIndex)
{
    relabelFeatureDict::iterator feature_found=featureRelabel.find(featureIndex);
    if (feature_found==featureRelabel.end())
       return -100;
    else
       return feature_found->second;
    }
    
/*
4. int getNumFeature() --- get the number of relabeled features
*/

int relabelFeature::getNumFeature()
{
    return countFeatureRelabel;
    }
    
/*
5. void writeRelabelFeatures(char* dictFileName) --- write the relabel features into a file
*/

void relabelFeature::writeRelabelFeatures(char* dictFileName)
{
     ofstream fout(dictFileName,ios::out); //create the outfile
     for (relabelFeatureDict::const_iterator iter=featureRelabel.begin();iter!=featureRelabel.end();iter++)
     {
         fout<<iter->first<<'\t'<<iter->second<<'\n';
         }
     }
