/***************************************************************************************************************

================================================================================================================
Main Process:
A. Learn the weight matrix for each source clusters
1. Read the word/tag (source/target) dictionaries
2. Read the word class dictionaries
3. Read the phrase pairs extraction table 
4. For each source phrase learning the weight matrix
5. Write the weight matrix file (.txt)
B. Construct the phrase option database (source phrase->target translations -> reordering probabilities)
1. Read the test corpus (source courpus)
2. Derive the phrase option database and the reordering probabilities
3. Write the phrase option database (.txt)
Remark:
*The maximum length of the file name should not exceed 100 (MAXCHAR) characters
*The maximum length of the sentence should not exceed 200 words
================================================================================================================

Input:
1. soucreCorpus (TestFile) --- the test source corpus (.txt file) 
2. sourceCorpus_tr (SourceCorpusFile) --- the name of the training corpus (source) for reading the word/tag dictionary
3. targetCorpus_tr (TargetCorpusFile) --- the name of the training corpus (target) for reading the word/tag dictionary
4. wordClassFile_fr (SourceWordClassFile) --- the word class file for source words (.txt file) 
5. wordClassFile_en (TargetWordClassFile) --- the word class file for target words (.txt file) 
6. extractPhraseTable (phraseTableFile) --- the phrase pair table for reading the phrase pairs
6*. relabelDict --- the feature relabeled dictionary for the training phrase pairs
7. classSetup (classSetup) --- current there is only two choice: 3 class setup and 5 class setup
8. maxNgramSize (maxNgramSize) --- the max length of the ngram features
9. minPrune (minPrune) --- prune ngram features that occur less than (<=) the minimum number of occurance
10. windowSize (windowSize) --- the window size of the environment (for ngram feature extraction)
11. maxPhraseLength (maxPhraseLength) --- extract phrase upto length maxPhraseLength
12. distCut (distCut) --- if the reordering distance is greater than distCut then cut the example
13. maxRound (maxRound) --- maximum iterator of training weight matrix W (see Ni et al., 2009)
14. step (step) --- the step of the perceptron-based structure learning algorithm (see Ni et al., 2009)
15. eTol (eTol) --- the error tolerance for training weight matrix W (see Ni et al., 2009)
16. phraseTranslationTable --- the phrase translation table contains the source->target translations 
                               (recommended using Moses's filtered translation table)
17. filterLabel --- 1 if the phrase translation table is filtered, 0 otherwise.
18. batchLabel ---  1: store all sentence options first then output them at once, use large memory but faster
                    0: collect phrase options for one sentence and output, use less memory but slower
19. maxTranslation --- the maximum number of translation for each source phrase, if 0, use all translations
20. minTrainingExample --- the minimum number of training examples required
Output:
21. fout_weightMatrix (weightMatrixFile) --- the output file for the weight matrix; 
22. fout_phraseOptionDB (phraseOptionFile) --- the phrase option database.
****************************************************************************************************************/

#include <cstdlib>
#include <iostream>
#include "probPredictionFunction.h"
#include <time.h>
#include <cstring>
#define MAXCHAR 100

using namespace std;
using std::strcpy;
using std::strcat;
using std::strcmp;

int main(int argc, char *argv[])
{
    cout<<"*******************************************************************************\n";
    cout<<"**Main Process for training the weight matrix and generate the phrase options**\n";
    cout<<"*******************************************************************************\n\n";
    //1. initialisation
    time_t time_start=time(NULL);; //start time
    time_t time_end; //end time
    
    //*. Initialise other parameters
    int maxNgramSize;             //the max length of the ngram features
    int minPrune;                 //prune the minimum number of occurance
    int zoneConf[2];          //store the window size of the environment (for ngram feature extraction)
    int maxPhraseLength;          //xtract phrase upto length maxPhraseLength
    int classSetup;                 //store the class setup
    int distCut;                  //if the reordering distance is greater than distCut then cut the example
    int maxRound;                //maximum iterator of training W
    float step;                  //the step of the training
    float eTol;                  //the error tolerance
    bool tableFilterLabel;       //1 if the phrase translation table is filtered, 0 otherwise.
    bool batchLabel;             //the batch output label
    int maxTranslations;         //the maximum number of translations
    int minTrainingExample;      //the minimum number of training examples
    
    
    //1.1 Process the arguments
    bool successFlag[19]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //should read all 19 arguments 
    char* wordClassFile_en= new char[MAXCHAR];//="./data/testFile/en.vcb.classes";
    char* wordClassFile_fr= new char[MAXCHAR];//="./data/testFile/fr.vcb.classes";
   // strcpy(tagsCorpus_en,"tags_");
    char* ngramDictFile_fr=new char[MAXCHAR];//="./data/testFile/ngramDict_fr.txt";
   // strcpy(ngramDictFile_fr,"ngramDict_");
    char* ngramDictFile_en=new char[MAXCHAR];//="./data/testFile/ngramDict_en.txt";
  //  strcpy(ngramDictFile_en,"ngramDict_");
    char* tagsNgramDictFile_fr=new char[MAXCHAR]; //="./data/testFile/tagsDict_fr.txt";
  //  strcpy(tagsNgramDictFile_fr,"tagsDict_");
    char* tagsNgramDictFile_en=new char[MAXCHAR];//="./data/testFile/tagsDict_en.txt";
  //  strcpy(tagsNgramDictFile_en,"tagsDict_");
    char* phraseDBFile=new char[MAXCHAR];//="./data/testFile/extractPhraseTrain_POS4.txt"; //the phrase database 
    char* featureRelabelDBFile= new char[MAXCHAR];//="./data/testFile/featureRelabel.txt";                      //the feture relabel file
  //  strcpy(featureRelabelDBFile,"featureRelabel_");
    char* testCorpusFile= new char[MAXCHAR]; //the test file
    char* weightMatrixFileName=new char[MAXCHAR];   //store the weight matrix 
    char* weightMatrixFilePosName=new char[MAXCHAR];//store the start position of each weight cluster
    char* phraseOptionDBFileName=new char[MAXCHAR]; //the output phrase option database
    char* phraseOptionDBPOSName=new char[MAXCHAR]; //the start position of each sentence phrase option 
    char* phraseTranslationTableFileName = new char[MAXCHAR]; //the input phrase translation table
    
    
    
     /*
    ========================================================================================================
    */
    
    //1.2 Deal with each configulation components
    if (argc<2)
    {
        cerr<<"Error in smt_mainProcess_generatePhraseOption.cpp: please input the name of the configulation file.\n";
        exit(1);
        }
    else
    {
        string eachLine;
        ifstream confFile(argv[1]);
        
        while (getline(confFile,eachLine,'\n'))
        {
              size_t strFind = eachLine.find(" =");
              if (strFind!=string::npos)
              {
                  //if it is the configulation line
                  string tempString = eachLine.substr(0,strFind);
                  char* fileName = (char*) tempString.c_str();

                  string tempString2 = eachLine.substr(strFind+2);
                  //for safety, erase all space
                  string tempString3="";
                  for (int kk=0;kk<tempString2.size();kk++)
                  {
                      if (tempString2[kk]!=' ' and tempString2[kk]!='\t')
                      {
                          tempString3+=tempString2[kk];
                          }
                      }
                  char* directoryName = (char*) tempString3.c_str();
                  
                                                                    
                  //configulation 1. source corpus file
                  if (strcmp(fileName,"SourceCorpusFile")==0)
                  {
                      //cout<<"0\n";
                      char* sourceCorpus=directoryName;
                      strcpy(ngramDictFile_fr,sourceCorpus);
                      strcat(ngramDictFile_fr,".ngramDict");
                      strcpy(tagsNgramDictFile_fr,sourceCorpus);
                      strcat(tagsNgramDictFile_fr,".tagsDict");
                      successFlag[0]=1;
                      }
                  
                  //configulation 2. target corpus file
                  else if (strcmp(fileName,"TargetCorpusFile")==0)
                  {
                       //cout<<"1\n";
                       char* targetCorpus=directoryName;
                       strcpy(ngramDictFile_en,targetCorpus);
                       strcat(ngramDictFile_en,".ngramDict");
                       strcpy(tagsNgramDictFile_en,targetCorpus);
                       strcat(tagsNgramDictFile_en,".tagsDict");
                       successFlag[1]=1;
                       }
                       
                  //configulation 3.  The source word class corpus
                  else if (strcmp(fileName,"SourceWordClassFile")==0)
                  {
                       //cout<<"2\n";
                       strcpy(wordClassFile_fr,directoryName);
                       successFlag[2]=1;
                       
                       }   
                       
                  //configulation 4.  The target word class corpus
                  else if (strcmp(fileName,"TargetWordClassFile")==0)
                  {
                       //cout<<"3\n";
                       strcpy(wordClassFile_en,directoryName);
                       successFlag[3]=1;
                       }  
                       
                  //configulation 5. The phrase pairs table
                  else if (strcmp(fileName,"phraseTableFile")==0)
                  {
                       //cout<<"4\n";
                       strcpy(phraseDBFile,directoryName);
                       strcpy(featureRelabelDBFile,phraseDBFile);
                       strcat(featureRelabelDBFile,".featureRelabel");
                       successFlag[4]=1;
                       }  
                  
                  //configulation 6. The input test file
                  else if (strcmp(fileName,"TestFile")==0)
                  {
                       //cout<<"5\n";
                       strcpy(testCorpusFile,directoryName);
                       successFlag[5]=1;
                       }  
                   
                  //configulation 7. The weight matrix file (output)
                  else if (strcmp(fileName,"weightMatrixFile")==0)
                  {
                       //cout<<"6\n";
                       strcpy(weightMatrixFileName,directoryName);
                       strcpy(weightMatrixFilePosName,weightMatrixFileName);
                       strcat(weightMatrixFilePosName,".startPosition");
                       successFlag[6]=1;
                       }  
                       
                  //configulation 8. The sentence phrase option file (output)
                  else if (strcmp(fileName,"phraseOptionFile")==0)
                  {
                       //cout<<"7\n";
                       strcpy(phraseOptionDBFileName,directoryName);
                       
                       strcpy(phraseOptionDBPOSName,phraseOptionDBFileName);
                       strcat(phraseOptionDBPOSName,".startPosition");
                       successFlag[7]=1;
                       }  
                       
                  //configulation 9. maxNgramSize
                  else if (strcmp(fileName,"maxNgramSize")==0)
                  {
                       //cout<<"8\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxNgramSize=intNum;
                       successFlag[8]=1;
                       }  
                  //configulation 10. minPrune
                  else if (strcmp(fileName,"minPrune")==0)
                  {
                       //cout<<"9\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       minPrune=intNum;
                       successFlag[9]=1;
                       }  
                  //configulation 11. window size
                  else if (strcmp(fileName,"windowSize")==0)
                  {
                       //cout<<"10\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       zoneConf[0]=intNum;
                       zoneConf[1]=intNum;
                       successFlag[10]=1;
                       }  
                  //configulation 12. maxPhraseLength
                  else if (strcmp(fileName,"maxPhraseLength")==0)
                  {
                       //cout<<"11\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxPhraseLength=intNum;
                       successFlag[11]=1;
                       }  
                  //configulation 13. classSetup
                  else if (strcmp(fileName,"classSetup")==0)
                  {
                       //cout<<"12\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       classSetup=intNum;
                       successFlag[12]=1;
                       }  
                       
                  //configulation 14. distCut
                  else if (strcmp(fileName,"distCut")==0)
                  {
                       //cout<<"13\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       distCut=intNum;
                       successFlag[13]=1;
                       }  
                  //configulation 15. maxRound
                  else if (strcmp(fileName,"maxRound")==0)
                  {
                       //cout<<"14\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxRound=intNum;
                       successFlag[14]=1;
                       }  
                  //configulation 16. step
                  else if (strcmp(fileName,"step")==0)
                  {
                       //cout<<"15\n";
                       istringstream temp(directoryName);
                       float floatNum;
                       temp>>floatNum;
                       step=floatNum;
                       successFlag[15]=1;
                       }  
                  //configulation 17. eTol
                  else if (strcmp(fileName,"eTol")==0)
                  {
                       //cout<<"16\n";
                       istringstream temp(directoryName);
                       float floatNum;
                       temp>>floatNum;
                       eTol=floatNum;
                       successFlag[16]=1;
                       }  
                  
                  //configulation 18. phraseTranslationTable
                  else if (strcmp(fileName,"phraseTranslationTable")==0)
                  {
                       strcpy(phraseTranslationTableFileName,directoryName);
                       successFlag[17]=1;
                       } 
                    
                  //configulation 19. maxTranslations   
                  else if (strcmp(fileName,"maxTranslations")==0)
                  {
                       //cout<<"16\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxTranslations=intNum;
                       successFlag[18]=1;
                       }  
                   //configulation 20. minTrainingExample   
                  else if (strcmp(fileName,"minTrainingExample")==0)
                  {
                       //cout<<"16\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       minTrainingExample=intNum;
                       successFlag[19]=1;
                       }    
                  //configulation 21. table filter label
                  else if (strcmp(fileName,"tableFilterLabel")==0)
                  {
                       //cout<<"18\n";
                       istringstream temp(directoryName);
                       temp>>tableFilterLabel;
                       successFlag[20]=1;
                       }   
                  
                  //configulation 22. batchLabel
                  else if (strcmp(fileName,"batchOutputLabel")==0)
                  {
                       //cout<<"20\n";
                       istringstream temp(directoryName);
                       temp>>batchLabel;
                       successFlag[21]=1;
                       }   
                       
                  //cout<<fileName<<'\n';
                  //cout<<directoryName<<'\n';
                  //system("PAUSE");
                                                    
                  }
              }
        }
    
    
    //check the arguments is completed or not
    if (!successFlag[0])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the source corpus.\n";
        exit(1);}
    else if (!successFlag[1])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the target corpus.\n";
        exit(1);}
    else if (!successFlag[2])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the source word class corpus.\n";
        exit(1);}
    else if (!successFlag[3])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the target word class corpus.\n";
        exit(1);}
    else if (!successFlag[4])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the name of the phrase pair DB.\n";
        exit(1);}
    else if (!successFlag[5])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the name of the input test file.\n";
        exit(1);}
    else if (!successFlag[6])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the name of the output weight matrix file.\n";
        exit(1);}
    else if (!successFlag[7])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the name of the output sentence phrase options file.\n";
        exit(1);}
    else if (!successFlag[8])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of maxNgramSize.\n";
        exit(1);}
    else if (!successFlag[9])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of minPrune.\n";
        exit(1);}
    else if (!successFlag[10])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of windowSize.\n";
        exit(1);}
    else if (!successFlag[11])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of maxPhraseLength.\n";
        exit(1);}
    else if (!successFlag[12])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of classSetup.\n";
        exit(1);}
    else if (!successFlag[13])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of distCut.\n";
        exit(1);}
    else if (!successFlag[14])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of maxRound.\n";
        exit(1);}
    else if (!successFlag[15])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of step size.\n";
        exit(1);}
    else if (!successFlag[16])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of eTol.\n";
        exit(1);}
    else if (!successFlag[17])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the phrase translation table.\n";
        exit(1);}
     else if (!successFlag[18])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the maximum number of translations for each source phrase.\n";
        exit(1);}
      else if (!successFlag[19])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: There is no definition of the minimum number of training examples for each source phrase (to training W).\n";
        exit(1);}
    else if (!successFlag[20])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: missing the filter label (tableFilterLabel) for the phrase translation table.\n";
        exit(1);}
    else if (!successFlag[21])
       {cerr<<"Error in smt_mainProcess_generatePhraseOption: missing the batch output label (batchOutputLabel) for outputing the sentence phrase options.\n";
        exit(1);}
    //else check the open state of input files
    else
    {
        //1. source corpus (word/word class) dictionaries
        ifstream tempFile1(ngramDictFile_fr);
        if (!tempFile1.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the source word dictionary.\n";
            exit(1);
            }
        else
        {tempFile1.close();}
        
        ifstream tempFile11(tagsNgramDictFile_fr);
        if (!tempFile11.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the source tag dictionary.\n";
            exit(1);
            }
        else
        {tempFile11.close();}
        
        //2. target corpus (word/word class) dictionaries
        ifstream tempFile2(ngramDictFile_en);
        if (!tempFile2.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the target word dictionary.\n";
            exit(1);
            }
        else
        {tempFile2.close();}
        
        ifstream tempFile22(tagsNgramDictFile_en);
        if (!tempFile22.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the target tag dictionary.\n";
            exit(1);
            }
        else
        {tempFile22.close();}
        
        
        //3. the source word class file
        ifstream tempFile3(wordClassFile_fr);
        if (!tempFile3.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the source word class file.\n";
            exit(1);
            }
        else
        {tempFile3.close();}
        
        //4. the target word class file
        ifstream tempFile4(wordClassFile_en);
        if (!tempFile4.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the target word class file.\n";
            exit(1);
            }
        else
        {tempFile4.close();}
        
        //5. the phrase database
        ifstream tempFile5(phraseDBFile);
        if (!tempFile5.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the phrase pair table file.\n";
            exit(1);
            }
        else
        {tempFile5.close();}
        
            
        //6. the test file 
        ifstream tempFile6(testCorpusFile);
        if (!tempFile6.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the test corpus.\n";
            exit(1);
            }
        else
        {tempFile6.close();}
            
        //6. the weight matrix file name
        if (strcmp(weightMatrixFileName,"")==0)
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: the name of the weight matrix file is empty.\n";
            exit(1);
            }
            
        //7. the phrase option file name
        if (strcmp(phraseOptionDBFileName,"")==0)
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: the name of the phrase option file is empty.\n";
            exit(1);
            }
            
        //8. class setup
        if (classSetup!=3 and classSetup!=5)
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption.cpp: wrong class setup (can only be 3 or 5)!\n";
            exit(1);
            }
            
        //9. the phrase translation table
        ifstream tempFile7(phraseTranslationTableFileName);
        if (!tempFile7.is_open())
        {
            cerr<<"Error in smt_mainProcess_generatePhraseOption: can't open the phrase translation table file.\n";
            exit(1);
            }
        else
        {tempFile7.close();}
            
        cout<<"Configulation......\n";
        cout<<"----------------------------------------------------------------\n";
        cout<<"For phrase pairs and feature extraction:\n";       
        cout<<"maxNgramSize = "<<maxNgramSize<<'\n'; 
        cout<<"windowSize = "<<zoneConf[0]<<'\n';
        cout<<"maxPhraseLength = "<<maxPhraseLength<<'\n';
        cout<<"maxTranslations = "<<maxTranslations<<'\n';
        cout<<"translation table filter label = "<<tableFilterLabel<<'\n';
        cout<<"\nFor distance phrase reordering structure learning model:\n";
        cout<<"classSetup = "<<classSetup<<'\n';
        cout<<"minPrune = "<<minPrune<<'\n';
        cout<<"minTrainingExample = "<<minTrainingExample<<'\n';
        cout<<"distCut = "<<distCut<<'\n';
        cout<<"maxRound = "<<maxRound<<'\n';
        cout<<"step = "<<step<<'\n';
        cout<<"eTol = "<<eTol<<'\n';
        cout<<"\nFor outputing the sentence phrase options:\n";
        cout<<"batchOutputLabel = "<<batchLabel<<'\n';
        cout<<"----------------------------------------------------------------\n\n";
        //system("PAUSE");
        }
    /*
    ========================================================================================================
    */
    
    
    //for time test
    time_t time_prev; //start time
    time_t time_next; //end time
    
    //********************************************************************************************************
    //2.read the phrase pair extraction table
    time_prev=time(NULL);
    cout<<"Step 1. Read the phrase pair extraction table (might take a bit long time, please be patient).\n";
    sourceReorderingTable* trainingPhraseTable = new sourceReorderingTable(phraseDBFile,classSetup,distCut);
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    
    
    //********************************************************************************************************
    //3.train the weight clusters
    
    time_prev=time(NULL);
    cout<<"Step 2. train the weight clusters.\n";
    vector<string> clusterNames = trainingPhraseTable->getClusterNames();
    int numClusters = trainingPhraseTable->getNumCluster();
    int processClusters=0;                               //store the number of clusters processed
    weightMatrixW* weightMatrix = new weightMatrixW();
    ifstream phraseTableFile(phraseDBFile,ios::binary); //re-open the phraseDBFile to get the features
    ofstream weightMatrixFile(weightMatrixFileName,ios::out); //output the weight matrix
    
    for (int i=0; i<numClusters; i++)
    {
        //3.1 For each cluster
        string sourcePhrase=clusterNames[i];
        int numberExample=trainingPhraseTable->getClusterMember(sourcePhrase);
        
        if (numberExample>=minTrainingExample)
        {
            processClusters++;
            //3.2 Get the training examples
            vector<vector<int> > trainingTable=trainingPhraseTable->getExamples(sourcePhrase,phraseTableFile);
        
            //3.3 Train the weight cluster
            weightClusterW weightCluster(sourcePhrase, classSetup);
            weightCluster.structureLearningW(trainingTable, maxRound, step, eTol);
        
            //3.4 write the weight cluster and update the weight matrix
            unsigned long long startPos=weightCluster.writeWeightCluster(weightMatrixFile);
            weightMatrix->insertWeightCluster(sourcePhrase,startPos);
            }
        
        
        //3.5 Notice
        if ((i+1)%100==0)
        {
            cout<<".";
            if ((i+1)%1000==0)
               cout<<'\n';
            }
           
        }
    cout<<"\nThe number of clusters been trained: "<<processClusters<<".\n";
        
    //3.5 Output the weight matrix 
    
    weightMatrix->writeWeightMatrix(weightMatrixFilePosName);
    weightMatrixFile.close();
    phraseTableFile.close();
    delete trainingPhraseTable;
    
    time_next=time(NULL);
    cout<<'\n';
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    
    /*
    If the weight matrix has already trained, then can comment all the procedures in 3. but use the following*/
    //------------------------------------------------------------------------
    //weightMatrixW* weightMatrix = new weightMatrixW(weightMatrixFilePosName);
    //-------------------------------------------------------------------------
    
    
    //********************************************************************************************************
    
    
    
    //********************************************************************************************************
    //4. Read the word class file
    time_prev=time(NULL);
    cout<<"Step 3. Read the word class file for the source/target corpus.\n";
    //4.1 For source text
    wordClassDict* wordClassDict_fr=new wordClassDict(wordClassFile_fr);
    //4.2 For target text
    wordClassDict* wordClassDict_en=new wordClassDict(wordClassFile_en);
    time_next=time(NULL);
    if (!wordClassDict_fr->checkReadFileStatus() or !wordClassDict_en->checkReadFileStatus())
    {
        cerr<<"Error in smt_mainProcess_generatePhraseOption.cpp: can't create the word classes dictionary.\n";
        exit(1);
        }
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    //********************************************************************************************************
    //5.read the ngram feature dictinary for source/target (word/tags) corpus
    time_prev=time(NULL);
    cout<<"Step 4. Read the ngram feature dictionary for the source/target (words/tags) corpus.\n";
    //5.1 the source words dictionary
    cout<<"4.1 Read the source words dictionary.\n";
    phraseNgramDict* ngramDict_fr= new phraseNgramDict(ngramDictFile_fr);
   
    //5.2 the target words dictionary
    cout<<"4.2 Read the target words dictionary.\n";
    phraseNgramDict* ngramDict_en= new phraseNgramDict(ngramDictFile_en);
    
    //5.3 the source tags dictionary
    cout<<"4.3 Read the source tags dictionary.\n";
    phraseNgramDict* tagsDict_fr=new phraseNgramDict(tagsNgramDictFile_fr);
   
    //5.4 the source tags dictionary
    cout<<"4.4 Read the target tags dictionary.\n";
    phraseNgramDict* tagsDict_en=new phraseNgramDict(tagsNgramDictFile_en);
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    
    //********************************************************************************************************
    //6. read the feature relabel dictionary
    time_prev=time(NULL);
    cout<<"Step 5. Read the feature relabeled dictionary.\n";
    relabelFeature* relabelDict = new relabelFeature(featureRelabelDBFile);
    if (relabelDict->getNumFeature()==0)
       {cerr<<"Error in smt_mainProcess_generatePhraseOption.cpp: no relabel feature dictionary is found.\n";
        exit(1);}
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    
     //********************************************************************************************************
    //7. read the test corpus and construct the source->target->features and source-><sentenceIndex,position>->startPos database
    time_prev=time(NULL);
    cout<<"Step 6. Extract all phrase options from the test corpus.\n";
    cout<<"[Format: sentence index -> <start/end position of source phrase> -> <target translations> -> reordering probs]\n";
    
    //7.1 Read the phrase translation table
    phraseTranslationTable* trainingTranslationTable_pointer;
    
    if (tableFilterLabel)
       {if (maxTranslations==0)
            trainingTranslationTable_pointer = new phraseTranslationTable(phraseTranslationTableFileName);
        else
            trainingTranslationTable_pointer = new phraseTranslationTable(phraseTranslationTableFileName, maxTranslations);
        }
    else
    {
        corpusPhraseDB* testPhraseDB = new corpusPhraseDB(testCorpusFile,maxPhraseLength);
        if (maxTranslations==0)
            trainingTranslationTable_pointer = new phraseTranslationTable(phraseTranslationTableFileName, testPhraseDB);
        else
            trainingTranslationTable_pointer = new phraseTranslationTable(phraseTranslationTableFileName, testPhraseDB, maxTranslations);
        }
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time for reading phrase pairs from phrase table: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    
    //7.2 If using the batch output style
    if (batchLabel)
    {
        //7.2.1 Collect the phrase options
        time_next=time(NULL);
        sentencePhraseOption phraseOption = smt_collectPhraseOptions(testCorpusFile, ngramDict_fr, ngramDict_en, tagsDict_fr, tagsDict_en, wordClassDict_fr, wordClassDict_en, maxPhraseLength, maxNgramSize, zoneConf, relabelDict, trainingTranslationTable_pointer, weightMatrixFileName, weightMatrix, classSetup);
    
        //7.2.2 Write down the phrase options (for each source test sentence)
        //phraseOption.outputPhraseOption(phraseOptionDBFileName);
    
        ifstream testCorpus(testCorpusFile,ios::binary);
        ofstream phraseOptionDB(phraseOptionDBFileName,ios::out);
        ofstream phraseOptionDBPOS(phraseOptionDBPOSName,ios::out);
        int countSentence=0;
        string eachSentence;
        //for each test sentence
        while (getline(testCorpus,eachSentence,'\n'))
        {
              //7.2.3 Get the start position of the sentence options
              unsigned long long tempStartPos=phraseOptionDB.tellp();     //record the start position of this sentence option
              phraseOptionDBPOS<<tempStartPos<<'\n';
             //7.2.3 construct the sentence array
             sentenceArray* sentence = new sentenceArray(eachSentence);
             //7.2.3 output the phrase options for this test sentence
             phraseOption.outputPhraseOption(phraseOptionDB,countSentence,sentence,trainingTranslationTable_pointer);
             countSentence++;
             delete sentence;
             if (countSentence%500==0)
                cout<<"Have outputed phrase options for "<<countSentence<<" sentences.\n";
                }
         cout<<"All together outputed phrase options for "<<countSentence<<" sentences.\n";
         testCorpus.close();
         phraseOptionDB.close();
         phraseOptionDBPOS.close();
         }
    //7.3 else collect the phrase options for each source test sentence and output the phrase options
    else
    {
        smt_collectPhraseOptions(testCorpusFile, ngramDict_fr, ngramDict_en, tagsDict_fr, tagsDict_en, wordClassDict_fr, wordClassDict_en, maxPhraseLength, maxNgramSize, zoneConf, relabelDict, trainingTranslationTable_pointer, weightMatrixFileName, weightMatrix, classSetup, phraseOptionDBFileName);
        }
    
    
    
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    time_end=time(NULL);
    cout<<"\nThe total processed time: "<<time_end-time_start<<" seconds.\n";
    //system("PAUSE");
    return EXIT_SUCCESS;
}
