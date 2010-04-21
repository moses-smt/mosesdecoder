/*
**********************************************************
Cpp file ---------- phraseTranslationTable.cpp
Declaration of class phraseTranslationTable 
Store the source phrases and their translations (from a phrase table provided by Moses, or our extraction)
* Phrase table format:
  source phrase ||| target phrase ||| others (e.g.~probabilities on Moses and ngram features on our extraction)

Components:
1. phraseTranslationTable --- store source phrases -> target phrase, can extend the dictionary in further version
2. numCluster --- number of clusters (source phrase) 
3. numPhrasePair --- number of phrase pairs stored
              
Functions:
1. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
2. int getNumCluster() --- get the number of clusters (source phrases)
3. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
4. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
5. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations

Special function:
1. phraseTranslationTable() --- constructor,   create empty phraseReorderingTable
2. phraseTranslationTable(char* inputFileName) --- contructor, create the phraseTranslationTable by reading the input file (phrase table)
3. phraseTranslationTable(char* inputFileName, int maxTranslations) --- contructor, create the phraseTranslationTable by reading the input file (phrase table)
                                                                        also prune out the translations if it exceed the maxium number of the translations
4. phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB) --- constructor, create the phraseTranslationTable
                                                                                 only for the phrases that occur in the test phrase database
5. phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB, int maxTranslations) --- constructor, constructor, create the phraseTranslationTable
                                                                                 only for the phrases that occur in the test phrase database
    ***********************************************************
*/


#include "phraseTranslationTable.h"

/*
1. constructor
*/

phraseTranslationTable::phraseTranslationTable()
{
     numCluster=0; //Create an empty phraseReorderingTable
     numPhrasePair=0;
    }
    
phraseTranslationTable::phraseTranslationTable(char* inputFileName)
{
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
        
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase table file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
                  size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                  if (targetFound==string::npos)
                     {cerr<<"Error in the phrase table file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                      exit(1);}
                  else
                  {
                      //2.2 Get the target phrase
                      target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      
                      //3. Update the phraseTable
                      sourceTargetMap::iterator sourceFound = phraseTable.find(source);
                      if (sourceFound==phraseTable.end())
                      {
                          numCluster++;
                          numPhrasePair++;
                          vector<string> tempString;
                          tempString.push_back(target);
                          phraseTable[source]=tempString;
                          }
                      else
                      {
                          sourceFound->second.push_back(target);
                          numPhrasePair++;
                          }
                      
                      }  
                  }  
              sentenceNum++;         //get the line index
              if (sentenceNum%10000==0)
                  cout<<"Have processed "<<sentenceNum<<" phrase pairs.\n";
              }
        }
    else
    {
        cerr<<"Error in phraseTranslationTable.cpp: Can't open the phrase table file!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    }

phraseTranslationTable::phraseTranslationTable(char* inputFileName,  int maxTranslations)
{
    sourceTargetMap phraseTableT;                    //store the temp phrase table
    sourceTargetProbMap phraseTableTProb;            //store the probabilitys for the translations
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
        
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase table file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
              
                  size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                  if (targetFound==string::npos)
                  {    cerr<<"Error in the phrase table file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                      exit(1);}
                  else
                  {
                      //2.2 Get the target phrase
                      target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      
                      //2.3 Get the translation probability
                      size_t probFound=eachSentence.rfind(" |||");
                      istringstream tempString(eachSentence.substr(probFound+5));
                      double targetProb=0.0;
                      double tempProb;
                      while (tempString>>tempProb)
                          targetProb+=log10(tempProb); 
                      
                      //3. Update the phraseTable
                      sourceTargetMap::iterator sourceFound = phraseTableT.find(source);
                      if (sourceFound==phraseTableT.end())
                      {
                          numCluster++;
                          numPhrasePair++;
                          vector<string> tempString;
                          tempString.push_back(target);
                          vector<float> tempProbVector;
                          tempProbVector.push_back(targetProb);
                          phraseTableT[source]=tempString;
                          phraseTableTProb[source]=tempProbVector;
                          }
                      else
                      {
                          numPhrasePair++;
                          sourceFound->second.push_back(target);
                          phraseTableTProb[source].push_back(targetProb);
                          }
                      } 
                  }  
              sentenceNum++;         //get the line index
              if (sentenceNum%10000==0)
                  cout<<"Have processed "<<sentenceNum<<" phrase pairs.\n";
              }
        }
    else
    {
        cerr<<"Error in phraseTranslationTable.cpp: Can't open the phrase table file!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    
    //4. filter out the translations
    for (sourceTargetMap::iterator sourceFound = phraseTableT.begin();sourceFound!=phraseTableT.end();sourceFound++)
    {
        //4.1 reserve the translation vector
        vector<string> targetTranslation; //store the target translation
        int numTrans=sourceFound->second.size();
        targetTranslation.reserve(min(numTrans,maxTranslations));
        //4.2 if the translation vector not exceed the maximum translation size
        if (numTrans<=maxTranslations)
        {
             for (int j=0; j<numTrans; j++)
             {
                 targetTranslation.push_back(sourceFound->second[j]);
                 }
             }
        //4.3 else 
        else
        {
            //4.3.1 sort the vector (descend order)
            vector<int> translationIndex;
            translationIndex.reserve(numTrans);
            for (int j=0;j<numTrans;j++)
                translationIndex.push_back(j);
            sort(translationIndex.begin(),translationIndex.end(),index_cmp<vector<float>&>(phraseTableTProb[sourceFound->first]));
            
            
            //4.3.2 get the top maxTranslations translation
            for (int j=0;j<maxTranslations;j++)
            {
                targetTranslation.push_back(sourceFound->second[translationIndex[j]]);
                //cout<<phraseTableTProb[sourceFound->first][translationIndex[j]]<<" ";
                }
            }            
        //cout<<'\n';
        //cin.get();
        phraseTable[sourceFound->first]=targetTranslation;
        }
    }



phraseTranslationTable::phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB)
{
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
        
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase table file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
                  if (testPhraseDB->checkPhraseDB(source))
                  {
                      size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                      if (targetFound==string::npos)
                         {cerr<<"Error in the phrase table file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                         exit(1);}
                         else
                         {
                             //2.2 Get the target phrase
                             target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      
                             //3. Update the phraseTable
                             sourceTargetMap::iterator sourceFound = phraseTable.find(source);
                             if (sourceFound==phraseTable.end())
                             {
                                 numCluster++;
                                 numPhrasePair++;
                                 vector<string> tempString;
                                 tempString.push_back(target);
                                 phraseTable[source]=tempString;
                                 }
                             else
                             {
                                 numPhrasePair++;
                                 sourceFound->second.push_back(target);
                                 }
                       }
                     } 
                  }  
              sentenceNum++;         //get the line index
              if (sentenceNum%10000==0)
                  cout<<"Have processed "<<sentenceNum<<" phrase pairs.\n";
              }
        }
    else
    {
        cerr<<"Error in phraseTranslationTable.cpp: Can't open the phrase table file!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    }


phraseTranslationTable::phraseTranslationTable(char* inputFileName, corpusPhraseDB* testPhraseDB, int maxTranslations)
{
    sourceTargetMap phraseTableT;                    //store the temp phrase table
    sourceTargetProbMap phraseTableTProb;            //store the probabilitys for the translations
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
        
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase table file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
                  if (testPhraseDB->checkPhraseDB(source))
                  {
                      size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                      if (targetFound==string::npos)
                         {cerr<<"Error in the phrase table file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                         exit(1);}
                         else
                         {
                             //2.2 Get the target phrase
                             target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      
                             //2.3 Get the translation probability
                             size_t probFound=eachSentence.rfind(" |||");
                             istringstream tempString(eachSentence.substr(probFound+5));
                             double targetProb=0.0;
                             double tempProb;
                             while (tempString>>tempProb)
                                   targetProb+=log10(tempProb); 
                      
                             //3. Update the phraseTable
                             sourceTargetMap::iterator sourceFound = phraseTableT.find(source);
                             if (sourceFound==phraseTableT.end())
                             {
                                 numCluster++;
                                 numPhrasePair++;
                                 vector<string> tempString;
                                 tempString.push_back(target);
                                 vector<float> tempProbVector;
                                 tempProbVector.push_back(targetProb);
                                 phraseTableT[source]=tempString;
                                 phraseTableTProb[source]=tempProbVector;
                                 }
                             else
                             {
                                 numPhrasePair++;
                                 sourceFound->second.push_back(target);
                                 phraseTableTProb[source].push_back(targetProb);
                                 }
                       }
                     } 
                  }  
              sentenceNum++;         //get the line index
              if (sentenceNum%10000==0)
                  cout<<"Have processed "<<sentenceNum<<" phrase pairs.\n";
              }
        }
    else
    {
        cerr<<"Error in phraseTranslationTable.cpp: Can't open the phrase table file!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    
    //4. filter out the translations
    for (sourceTargetMap::iterator sourceFound = phraseTableT.begin();sourceFound!=phraseTableT.end();sourceFound++)
    {
        //4.1 reserve the translation vector
        vector<string> targetTranslation; //store the target translation
        int numTrans=sourceFound->second.size();
        targetTranslation.reserve(min(numTrans,maxTranslations));
        //4.2 if the translation vector not exceed the maximum translation size
        if (numTrans<=maxTranslations)
        {
             for (int j=0; j<numTrans; j++)
             {
                 targetTranslation.push_back(sourceFound->second[j]);
                 }
             }
        //4.3 else 
        else
        {
            //4.3.1 sort the vector (descend order)
            vector<int> translationIndex;
            translationIndex.reserve(numTrans);
            for (int j=0;j<numTrans;j++)
                translationIndex.push_back(j);
            sort(translationIndex.begin(),translationIndex.end(),index_cmp<vector<float>&>(phraseTableTProb[sourceFound->first]));
            
            
            //4.3.2 get the top maxTranslations translation
            for (int j=0;j<maxTranslations;j++)
            {
                targetTranslation.push_back(sourceFound->second[translationIndex[j]]);
                //cout<<phraseTableTProb[sourceFound->first][translationIndex[j]]<<" ";
                }
            }            
        //cout<<'\n';
        phraseTable[sourceFound->first]=targetTranslation;
        }
    }



/*
2. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
*/
vector<string> phraseTranslationTable::getClusterNames()
{
        vector<string> clusterNames;
        if (numCluster==0)
        {
            cerr<<"There are no source phrases in this phrase translation table.\n";
            exit(1);
            }
        else
        {
            for (sourceTargetMap::const_iterator sourceFound=phraseTable.begin();sourceFound!=phraseTable.end();sourceFound++)
            {
                clusterNames.push_back(sourceFound->first);
                }
            }
        return clusterNames;
        }

/*
3. int getNumCluster() --- get the number of clusters (source phrases)
*/
int phraseTranslationTable::getNumCluster()
{return numCluster;}

/*
4. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
*/
int phraseTranslationTable::getNumPhrasePair()
{return numPhrasePair;}


/*
5. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
*/
vector<string> phraseTranslationTable::getTargetTranslation(string sourcePhrase)
{
    vector<string> targetTranslation;
    //1. Find the source phrase
    sourceTargetMap::const_iterator sourceFound=phraseTable.find(sourcePhrase);
    if (sourceFound==phraseTable.end())
    {
        //1.1 If not found, return a null vector
        //cerr<<"Warning in phraseTranslationTable.cpp: the source phrase doesn't appear in the phrase table (training set).\n";
        return targetTranslation;
        }
    else
    {
        //2. else go through all translation and return the vector    
        return sourceFound->second;
        }
    }
/*
6. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations
*/
int phraseTranslationTable::getNumberofTargetTranslation(string sourcePhrase)
{
    sourceTargetMap::const_iterator sourceFound=phraseTable.find(sourcePhrase);
    if (sourceFound==phraseTable.end())
    { return 0;}
    else
    { return sourceFound->second.size();}
    }
