/*
**********************************************************
CPP file ---------- phraseReorderingTable.cpp
Declaration of class phraseReorderingTable
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
Functions:
1. int createOrientationClass(int dist,int classSetup) --- the create the orientation class
2. map<string, vector<int>> getClusterMember(string sourcePhrase) --- get the target phrases and ngram features for a source phrase
2*. int getClusterMember(string sourcePhrase) --- get the number of examples in this cluster
3. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
4. int getNumCluster() --- get the number of clusters (source phrases)
5. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
6. vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile)  --- get the examples with the ngram features (store in vector)
7. vector<unsigned long long> getPositionIndex() --- get the position index of all phrase pairs
8. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
9. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
10. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations

Special function:
1. phraseReorderingTable() --- constructor,   create empty phraseReorderingTable
2. phraseReorderingTable(char* inputFileName, int classSetup, int distCut) --- contructor, create the phraseReorderingTable
Remark: if the reordering distance exceed distCut, don't use this phrase pair (to avoid some word alignment errors)
***********************************************************
*/

#include "phraseReorderingTable.h" //include definition of class phraseReorderingTable from phraseReorderingTable.h

/*
1. constructor
*/

phraseReorderingTable::phraseReorderingTable()
{
    numCluster=0; //Create an empty phraseReorderingTable
    numPhrasePair=0;
    }
    

phraseReorderingTable::phraseReorderingTable(char* inputFileName, int classSetup, int distCut)
{
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    numOrientation=classSetup;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    unsigned long long startPos=0;                                //store the start position of each line
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
              int dist;               //distance
              int orientationClass;   //store the orientation class
              
              
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase DB file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
                  size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                  if (targetFound==string::npos)
                     {cerr<<"Error in the phrase DB file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                      exit(1);}
                  else
                  {
                      //2.2 Get the target phrase
                      target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      //cout<<target<<'\n';
                      size_t distFound = eachSentence.find(" |||",targetFound+5);
                      if (distFound==string::npos)
                         {cerr<<"Error in the phrase DB file: can't find the reordering distance in line: "<<sentenceNum<<"."<<'\n';
                          exit(1);}
                      else
                      {
                          //2.3 Get the distance and orientation
                          istringstream tempString(eachSentence.substr(targetFound+5,distFound-targetFound-5));
                          tempString>>dist; //Get the distance value
                          //cout<<dist<<'\n';
                          //system("PAUSE");
                          if (abs(dist)<=distCut) //If not cut the item
                          {
                              orientationClass =  createOrientationClass(dist,classSetup);
                              numPhrasePair++; //update the number of phrase pairs
                      
                              //3. Update the phraseReorderingTable
                              mapmap::iterator sourcephraseFound=phraseTable.find(source);
                              
                              //3.1 If not found the source key
                              if (sourcephraseFound==phraseTable.end())
                              {
                                  //3.1.1 update the private components
                                  numCluster++;                        //update the number of source phrases
                                  mapDict targetMap;
                                  vector<int> tempValue;
                                  positionIndex.push_back(startPos+distFound+5); //get the start postion of the features
                                  tempValue.push_back(orientationClass);
                                  tempValue.push_back(sentenceNum);              //get the index in the positionIndex
                                  startPos=startPos+eachSentence.size()+1; //get the new line position
                                  //3.1.2 update the target map
                                  targetMap[target]=tempValue;
                                  //3.1.3 update the source map
                                  phraseTable[source]=targetMap;
                                  
                                  }
                              
                              //3.2 If the source key already exists
                              else {
                                    mapDict targetMap=sourcephraseFound->second;
                                    mapDict::iterator targetphraseFound=targetMap.find(target);
                                    //3.2.1 If the target key doesn't exist
                                    if (targetphraseFound==targetMap.end())
                                    {
                                        vector<int> tempValue;
                                        positionIndex.push_back(startPos+distFound+5);
                                        tempValue.push_back(orientationClass);
                                        tempValue.push_back(sentenceNum);
                                        startPos=startPos+eachSentence.size()+1; //get the new line position
                                        targetMap[target]=tempValue;             //update the target Map
                                        }
                                    //3.2.2 If the target key exists
                                     else
                                     {
                                         positionIndex.push_back(startPos+distFound+5);
                                         targetMap[target].push_back(orientationClass);
                                         targetMap[target].push_back(sentenceNum);
                                         startPos=startPos+eachSentence.size()+1; //get the new line position
                                         }
                                     sourcephraseFound->second=targetMap; //update the phraseReorderingTable
                                    }
                              
                              }
                          else
                          {   //if not update this line, should still update the startPos
                              positionIndex.push_back(startPos); 
                              startPos=startPos+eachSentence.size()+1;
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
        cerr<<"Error in phraseReorderingTable.cpp: Can't open the phrase DB!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    }




/*
2. int createOrientationClass(int dist,int classSetup) --- the create the orientation class

*/
int phraseReorderingTable::createOrientationClass(int dist, int classSetup)
{
    int orientationClass;
    //If three-class setup
    if (classSetup==3)
    {
        if (dist<0)
            orientationClass=0;
        else if (dist==0)
            orientationClass=1;
        else
            orientationClass=2;
        }
    else if (classSetup==5)
    {
         if (dist<=-5)
             orientationClass=0;
         else if (dist>-5 and dist<0)
             orientationClass=1;
         else if (dist==0)
             orientationClass=2;
         else if (dist>0 and dist<5)
             orientationClass=3;
         else
             orientationClass=4;
         }
    else
    {
        cerr<<"Currently there is no class setup: "<<classSetup<<" in our model.\n";
        }
    
  
    return orientationClass; //return the orientation class
    }

/*
3. map<string, vector<int>> getClusterMember(string sourcePhrase) --- get the target phrases and ngram features for a source phrase
*/

mapDict phraseReorderingTable::getClusterMember(string sourcePhrase)
{
        mapDict targetMap;
        mapmap::const_iterator sourcephraseFound = phraseTable.find(sourcePhrase);
        if (sourcephraseFound==phraseTable.end())
            cerr<<"Can't find the source phrase: "<<sourcePhrase<<" in the phrase DB.\n";
        else
            targetMap=sourcephraseFound->second;
        return targetMap; 
        }

/*
4. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
*/

vector<string> phraseReorderingTable::getClusterNames()
{
        vector<string> clusterNames;
        if (numCluster==0)
        {
            cerr<<"There are no source phrases in this phrase reordering table.\n";
            exit(1);
            }
        else
        {
            for (mapmap::const_iterator sourceFound=phraseTable.begin();sourceFound!=phraseTable.end();sourceFound++)
            {
                clusterNames.push_back(sourceFound->first);
                }
            }
        return clusterNames;
        }


/*
5. int getNumCluster() --- get the number of clusters (source phrases)
*/
int phraseReorderingTable::getNumCluster()
{
    return numCluster;    
        }


/*
6. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
*/
int phraseReorderingTable::getNumPhrasePair()
{
    return numPhrasePair;
    }


/*
7. int getNumOrientatin() --- get the class setup
*/
int phraseReorderingTable::getNumOrientatin()
{return numOrientation;}

/*
8. vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile)  --- get the examples with the ngram features (store in vector)
*/
vector<vector<int> > phraseReorderingTable::getExamples(string sourcePhrase, ifstream& inputFile)
{
    //1. Initialisation
    vector<vector<int> > clusterExample;
    mapDict targetMap=phraseTable[sourcePhrase]; //get the example pool
    
    //2. Go through all target phrases
    for (mapDict::const_iterator keyFound=targetMap.begin();keyFound!=targetMap.end();keyFound++)
    {
        vector<int> exampleIndex=keyFound->second;
        for (int i=0; i< exampleIndex.size();i=i+2)
        {
            vector<int> eachExample;
            string featureString;    //store the feature string
            int featureIndex;       //store each feature index
            //2.1 Put in the first item (orientation)
            eachExample.push_back(exampleIndex[i]);
            //2.2 Set the pointer to the feature space
            inputFile.seekg(positionIndex[exampleIndex[i+1]],ios::beg);
            getline(inputFile,featureString);
            istringstream tempString(featureString);
            //2.3 Read the features
            while (tempString>>featureIndex)
                  eachExample.push_back(featureIndex);
            //2.4 put in the example
            clusterExample.push_back(eachExample);
            } 
        }
    //3. return the cluster examples
    return clusterExample;
    }


/*
9. vector<unsigned long long> getPositionIndex() --- get the position index of all phrase pairs
*/
vector<unsigned long long> phraseReorderingTable::getPositionIndex()
{return positionIndex;}


/*
10. vector<string> getTargetTranslation(string sourcePhrase) --- get the target translations
*/

vector<string> phraseReorderingTable::getTargetTranslation(string sourcePhrase)
{
    vector<string> targetTranslation;
    //1. Find the source phrase
    mapmap::const_iterator sourceFound=phraseTable.find(sourcePhrase);
    if (sourceFound==phraseTable.end())
    {
        //1.1 If not found, return a null vector
        //cerr<<"Warning in phraseReorderingTable.cpp: the source phrase doesn't appear in the phrase table (training set).\n";
        return targetTranslation;
        }
    else
    {
        //2. else go through all translation and return the vector
        for (mapDict::const_iterator targetFound=sourceFound->second.begin();targetFound!=sourceFound->second.end();targetFound++)
        {
            targetTranslation.push_back(targetFound->first);
            }
            
        return targetTranslation;
        }
    }


/*
6. int getNumberofTargetTranslation(string sourcePhrase) --- get the number of target translations
*/
int phraseReorderingTable::getNumberofTargetTranslation(string sourcePhrase)
{
    mapmap::const_iterator sourceFound=phraseTable.find(sourcePhrase);
    if (sourceFound==phraseTable.end())
    { return 0;}
    else
    { return sourceFound->second.size();}
    }


/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/

/*
1. constructor
*/

sourceReorderingTable::sourceReorderingTable()
{
    numCluster=0; //Create an empty phraseReorderingTable
    numPhrasePair=0;
    }
    

sourceReorderingTable::sourceReorderingTable(char* inputFileName, int classSetup, int distCut)
{
    //1. initialisation
    numCluster=0;
    numPhrasePair=0;
    numOrientation=classSetup;
    ifstream inputFile(inputFileName,ios::binary); //set it as binary file
    unsigned long long startPos=0;                 //store the start position of each line
    int sentenceNum=0;                             //get the sentence index
    string eachSentence;                           //store each sentence source ||| target ||| dist ||| features
    
    if (inputFile.is_open())
    {
        //2. for each sentence
        while (getline(inputFile,eachSentence))
        {
              string source;           //source phrase
              string target;          //target phrase
              int dist;               //distance
              int orientationClass;   //store the orientation class
              
              
              size_t sourceFound = eachSentence.find(" |||"); //find the separationg between the key and the values
              if (sourceFound==string::npos)
                  {cerr<<"Error in the phrase DB file: can't find the source phrase in line: "<<sentenceNum<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Get the source phrase
                  source=eachSentence.substr(0,sourceFound); //get the source phrase
                  //cout<<source<<'\n';
                  
                  size_t targetFound = eachSentence.find(" |||",sourceFound+5);
                  if (targetFound==string::npos)
                     {cerr<<"Error in the phrase DB file: can't find the target phrase in line: "<<sentenceNum<<"."<<'\n';
                      exit(1);}
                  else
                  {
                      //2.2 Get the target phrase
                      target=eachSentence.substr(sourceFound+5,targetFound-sourceFound-5); //get the target phrase
                      //cout<<target<<'\n';
                      size_t distFound = eachSentence.find(" |||",targetFound+5);
                      if (distFound==string::npos)
                         {cerr<<"Error in the phrase DB file: can't find the reordering distance in line: "<<sentenceNum<<"."<<'\n';
                          exit(1);}
                      else
                      {
                          //2.3 Get the distance and orientation
                          istringstream tempString(eachSentence.substr(targetFound+5,distFound-targetFound-5));
                          tempString>>dist; //Get the distance value
                          //cout<<dist<<'\n';
                          //system("PAUSE");
                          if (abs(dist)<=distCut) //If not cut the item
                          {
                              orientationClass =  createOrientationClass(dist,classSetup);
                              numPhrasePair++; //update the number of phrase pairs
                      
                              //3. Update the phraseReorderingTable
                              mapDict::iterator sourcephraseFound=phraseTable.find(source);
                              
                              //3.1 If not found the source key
                              if (sourcephraseFound==phraseTable.end())
                              {
                                  //3.1.1 update the private components
                                  numCluster++;                        //update the number of source phrases
                                  vector<int> tempValue;
                                  positionIndex.push_back(startPos+distFound+5);
                                  tempValue.push_back(orientationClass);
                                  tempValue.push_back(sentenceNum);
                                  startPos=startPos+eachSentence.size()+1; //get the new line position
                                  //3.1.2 update the source map
                                  phraseTable[source]=tempValue;
                                  
                                  }
                              
                              //3.2 If the source key already exists
                              else {
                                    positionIndex.push_back(startPos+distFound+5);
                                    phraseTable[source].push_back(orientationClass);
                                    phraseTable[source].push_back(sentenceNum);
                                    startPos=startPos+eachSentence.size()+1; //get the new line position
                                    }
                              
                              }
                          else
                          {   //if not update this line, should still update the startPos
                              positionIndex.push_back(startPos);
                              startPos=startPos+eachSentence.size()+1;
                               
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
        cerr<<"Can't open the phrase DB!\n";
        exit(1);
        }
    cout<<"All together processed "<<sentenceNum<<" phrase pairs.\n";
    inputFile.close();
    }




/*
2. int createOrientationClass(int dist,int classSetup) --- the create the orientation class

*/
int sourceReorderingTable::createOrientationClass(int dist, int classSetup)
{
    int orientationClass;
    //If three-class setup
    if (classSetup==3)
    {
        if (dist<0)
            orientationClass=0;
        else if (dist==0)
            orientationClass=1;
        else
            orientationClass=2;
        }
    else if (classSetup==5)
    {
         if (dist<=-5)
             orientationClass=0;
         else if (dist>-5 and dist<0)
             orientationClass=1;
         else if (dist==0)
             orientationClass=2;
         else if (dist>0 and dist<5)
             orientationClass=3;
         else
             orientationClass=4;
         }
    else
    {
        cerr<<"Currently there is no class setup: "<<classSetup<<" in our model.\n";
        }
    
  
    return orientationClass; //return the orientation class
    }

/*
3. int getClusterMember(string sourcePhrase) --- get the number of examples in this cluster
*/

int sourceReorderingTable::getClusterMember(string sourcePhrase)
{
        int numberExample=0;
        mapDict::const_iterator sourcephraseFound = phraseTable.find(sourcePhrase);
        if (sourcephraseFound==phraseTable.end())
            cerr<<"Warning in phraseReorderingTable.cpp: Can't find the source phrase: "<<sourcePhrase<<" in the phrase DB.\n";
        else
            numberExample = sourcephraseFound->second.size()/2;
        return numberExample; 
        }

/*
4. vector<string> getClusterNames() --- get all source phrases in this phrase reordering table
*/

vector<string> sourceReorderingTable::getClusterNames()
{
        vector<string> clusterNames;
        if (numCluster==0)
        {
            cerr<<"There are no source phrases in this phrase reordering table.\n";
            exit(1);
            }
        else
        {
            for (mapDict::const_iterator sourceFound=phraseTable.begin();sourceFound!=phraseTable.end();sourceFound++)
            {
                clusterNames.push_back(sourceFound->first);
                }
            }
        return clusterNames;
        }


/*
5. int getNumCluster() --- get the number of clusters (source phrases)
*/
int sourceReorderingTable::getNumCluster()
{
    return numCluster;    
        }


/*
6. int getNumPhrasePair() --- get the number of phrase pairs in this phraseTable
*/
int sourceReorderingTable::getNumPhrasePair()
{
    return numPhrasePair;
    }


/*
7. int getNumOrientatin() --- get the class setup
*/
int sourceReorderingTable::getNumOrientatin()
{return numOrientation;}

/*
8. vector<vector<int> > getExamples(string sourcePhrase, ifstream& inputFile)  --- get the examples with the ngram features (store in vector)
*/

vector<vector<int> > sourceReorderingTable::getExamples(string sourcePhrase, ifstream& inputFile)
{
    //1. Initialisation
    vector<vector<int> > clusterExample;
    
    //2. Go through all target phrases
    vector<int> exampleIndex=phraseTable[sourcePhrase]; //get the example pool
    
    for (int i=0; i< exampleIndex.size();i=i+2)
    {
        vector<int> eachExample;
        string featureString;    //store the feature string
        int featureIndex;       //store each feature index
        //2.1 Put in the first item (orientation)
        eachExample.push_back(exampleIndex[i]);
        //2.2 Set the pointer to the feature space
        inputFile.seekg(positionIndex[exampleIndex[i+1]],ios::beg);
        getline(inputFile,featureString);
        istringstream tempString(featureString);
        //2.3 Read the features
        while (tempString>>featureIndex)
            eachExample.push_back(featureIndex);
        //2.4 put in the example
        clusterExample.push_back(eachExample);
        } 
    //3. return the cluster examples
    return clusterExample;
    }


/*
9. vector<unsigned long long> getPositionIndex() --- get the position index of all phrase pairs
*/
vector<unsigned long long> sourceReorderingTable::getPositionIndex()
{return positionIndex;}
