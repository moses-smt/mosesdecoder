/*
**********************************************************
cpp file ---------- phraseNgramDict.cpp
Member-function definitions class phraseNgramDict
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
8. void outputNgramDict(char* filename) ---- output the dictionary into a file (.txt file)
9. bool checkReadFileStatus() --- (if it contains a file name), check whether the read of the dictionary is sucessful.
10. int getNumFeature() --- get the number of features in this dictionary.
Special function:
1. phraseNgramDict(char* fileName) --- constructor, if the file name is empty, construct an empty dictionary (do nothing)
                                        else, read the dictionary from the file
***********************************************************
*/

#include "phraseNgramDict.h" //include definition of class phraseNgramdict from phraseNgramDict.h


/*1. PhraseNgramDict constructor
If there is no a file name, then construct the dictionary
*/
phraseNgramDict::phraseNgramDict()
{                               
            readDictCheck=true;
            ngramIndex=0; //feature index
}

/*1. PhraseNgramDict constructor
If there is a file name string, then read the dictionary
*/
phraseNgramDict::phraseNgramDict(char* dictFileName)
{
     ngramIndex=0; //feature index
     ifstream inputDict;
     inputDict.open(dictFileName,ios::in);
     if (inputDict.is_open())
     {
         string keyInfo; //store each feature
         while(getline(inputDict,keyInfo,'\n'))
         {
             string tempKey;       //the key (string)
             int ngramFeatureIndex;       //the feature index (int)
             int ngramLength;      //the feature length (int)
             int ngramOccurance;   //the ngram occurance
             vector<int> keyValue;
             size_t keyFound = keyInfo.find(" |||"); //find the separationg between the key and the values
             if (keyFound==string::npos)
                 cerr<<"Error in the dictionary file: can't find the ngram feature key.\n";
             else
             {
                 tempKey=keyInfo.substr(0,keyFound);
                 istringstream tempString(keyInfo.substr(keyFound+4));//store the values
                 tempString>>ngramFeatureIndex>>ngramLength>>ngramOccurance;
                 }    
             
             
             keyValue.push_back(ngramFeatureIndex); //push the feature index
             keyValue.push_back(ngramLength);//push the ngram length
             keyValue.push_back(ngramOccurance); //pus the occurance
             ngramDict[tempKey]=keyValue;        //assign the dictionary
             ngramIndex++;
             } 
                 
              
         readDictCheck=true;     
         }
     else
     {
         cerr<<"There is no dictionary found!\n";
         readDictCheck=false;
         }
     inputDict.close(); 
}
        


/*
2. void insertNgram (string key,int ngramLength) -- insert the key, if it already exists, update the occurance 
*/
void phraseNgramDict::insertNgram(string stringKey, int ngramLength)
{
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
       {
       ngramIndex++;                          //create the feature index
       vector<int> keyValue (3,-1);          //initialize the key value
       keyValue[0]=ngramIndex;
       keyValue[1]=ngramLength;
       keyValue[2]=1;                        //occur once
       ngramDict[stringKey]=keyValue; //add the (key,value) pair to the dictionary
       }
    else
        ngramDict[stringKey][2]+=1; //update the occurance
                                    }


/*
3. void deleteNgram (string key)  --- delete the key 
*/
void phraseNgramDict::deleteNgram(string stringKey)
{
    ngramDict.erase(stringKey); //delete the selected key
                                    }


/*
4. bool findNgram (string key)  --- if the key is in the dictionary (return bool 1 or -1)
*/
bool phraseNgramDict::findNgram(string stringKey)
{
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
       return false;
    else
       return true;
                                    }
                                    
/*
5. int getNgramIndex (string key) --- if the key exists, return the index, else return -1;
*/
int phraseNgramDict::getNgramIndex(string stringKey)
{
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
       return -1;
    else
        return ngramDict[stringKey][0]; //return the feature index
    }


/*
6. int getNgramOccurance (string key) --- if the key exists, return the occurance, else return -1;
*/
int phraseNgramDict::getNgramOccurance(string stringKey)
{
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
       return -1;
    else
        return ngramDict[stringKey][2]; //return the ngram occurance
    }


/*
7. int getNgramLength(string key) --- if the key exists, return the length of this ngram, else return -1;
*/
int phraseNgramDict::getNgramLength(string stringKey)
{
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
       return -1;
    else
        return ngramDict[stringKey][1]; //return the ngram length
    }


/*
8. vector<int> getNgramItems(string key) --- if the key exists, return index,length,occurance, else return {-1,-1,-1}
*/
vector<int>  phraseNgramDict::getNgramItems(string stringKey)
{
    vector<int> keyValue (3,-1); //3 {-1} items
    phraseDict::iterator findKey=ngramDict.find(stringKey);
    if (findKey==ngramDict.end())//if the key doesn't exist
        return keyValue;
    else
        keyValue=ngramDict[stringKey]; //get the key value
        return keyValue;
    }


/*
9. void outputNgramDict(char* dictFileName) --- (output the dictionary into a file (.txt file)
*/

void phraseNgramDict::outputNgramDict(char* dictFileName,int minOccurenceCut)
{
     ofstream outputDict(dictFileName,ios::out);
     for (phraseDict::const_iterator iter=ngramDict.begin();iter!=ngramDict.end();iter++)
        if (iter->second[2]>minOccurenceCut)//if the feature appear more than minimum-occurence
           outputDict<<iter->first<<" ||| "<<iter->second[0]<<'\t'<<iter->second[1]<<'\t'<<iter->second[2]<<'\n'; //output the dictionary key
    outputDict.close();   
     }


/*
10. bool checkReadFileStatus() --- (if it contains a file name), check whether the read of the dictionary is sucessful.
*/

bool phraseNgramDict::checkReadFileStatus()
{
     return readDictCheck; //get the dictionary read flag
     }

/*
11. int getNumFeature() --- get the number of features in this dictionary
*/

int phraseNgramDict::getNumFeature()
{return ngramIndex;}
