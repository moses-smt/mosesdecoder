/***************************************************************************************************************

================================================================================================================
Main Process:
Construct the phrase reordering database for training.
1. Read the word class labels and create the tags corpus for the source/target corpus.
2. Read the source/target (words/tags) corpus and create the ngram feature dictionary.
   Remark: A. the length of ngram feature is control by (int maxNgramSize);
           B. can prune the ngram features that appear less than (int minPrune) times.
3. Extract the phrase pairs with their ngram features store then in a .txt file

Remark:
*The maximum length of the file name should not exceed 100 (MAXCHAR) characters
*The maximum length of the sentence should not exceed 200 words
================================================================================================================

Input:
1. soucreCorpus (SourceCorpusFile) --- the source corpus (.txt file) 
2. targetCorpus (TargetCorpusFile) --- the target corpus (.txt file) 
3. wordAlignmentFile (alignmentFile) --- the word alignment file (.txt file from GIZA++, named aligned.grow-diag-final-and) 
4. wordClassFile_fr (SourceWordClassFile) --- the word class file for source words (.txt file) 
5. wordClassFile_en (TargetWordClassFile) --- the word class file for target words (.txt file) 
6. maxNgramSize --- the max length of the ngram features
7. minPrune --- prune ngram features that occur less than (<=) the minimum number of occurance
8. windowSize --- the window size of the environment (for ngram feature extraction)
9. maxPhraseLength --- extract phrase upto length maxPhraseLength
10*. optinal (testCorpusFile) (TestFileName) --- the source test corpus (.txt file) to filter the phrase DB 
Output:
11. fout_phraseDB (phraseTableFile) --- the output file for phraseDB: source phrase ||| target phrase ||| reordering dist ||| feature index; 
11*. fout_relabelDB --- the relabel dictionary of the ngram features
****************************************************************************************************************/
#include <cstdlib>
#include <iostream>
#include "phraseConstructionFunction.h" //include all functions and classes to construct the phraseDB
#include <time.h>
#include <cstring>
#include <sstream>
#include <fstream>
#define MAXCHAR 100

using namespace std;
using std::istringstream;
using std::ifstream;

int main(int argc, char *argv[])
{
    
    time_t time_start=time(NULL); //start time
    time_t time_end; //end time
    
    
    cout<<"*****************************************************************************\n";
    cout<<"**Main Process for constructing the phrase reordering database for training**\n";
    cout<<"*****************************************************************************\n\n"; 
    //1. Initialisation 
    
    int maxNgramSize;             //the max length of the ngram features
    int minPrune;                 //prune the minimum number of occurance
    int zoneConf[2];              //store the window size of the environment (for ngram feature extraction)
    int maxPhraseLength;          //extract phrase upto length maxPhraseLength
    
    
    
    //1.1 Process the arguments
    bool successFlag[11]={0,0,0,0,0,0,0,0,0,0,0}; //should read all 10 arguments (the final one is optional)
    char* sourceCorpus = new char[MAXCHAR];//="./data/testFile/euroTest.fr";
    char* targetCorpus=new char[MAXCHAR];//="./data/testFile/euroTest.en";
    char* wordClassFile_en= new char[MAXCHAR];//="./data/testFile/en.vcb.classes";
    char* wordClassFile_fr= new char[MAXCHAR];//="./data/testFile/fr.vcb.classes";
    char* wordAlignmentFile= new char[MAXCHAR];//="./data/testFile/alignTest.txt";
    char* tagsCorpus_fr=new char[MAXCHAR];//="./data/testFile/tagsTest.fr";
   // strcpy(tagsCorpus_fr,"tags_");
    char* tagsCorpus_en=new char[MAXCHAR];//="./data/testFile/tagsTest.en";
   // strcpy(tagsCorpus_en,"tags_");
    char* ngramDictFile_fr=new char[MAXCHAR];//="./data/testFile/ngramDict_fr.txt";
   // strcpy(ngramDictFile_fr,"ngramDict_");
    char* ngramDictFile_en=new char[MAXCHAR];//="./data/testFile/ngramDict_en.txt";
  //  strcpy(ngramDictFile_en,"ngramDict_");
    char* tagsNgramDictFile_fr=new char[MAXCHAR]; //="./data/testFile/tagsDict_fr.txt";
  //  strcpy(tagsNgramDictFile_fr,"tagsDict_");
    char* tagsNgramDictFile_en=new char[MAXCHAR];//="./data/testFile/tagsDict_en.txt";
  //  strcpy(tagsNgramDictFile_en,"tagsDict_");
  //  char* wordDictFile_fr=new char[MAXCHAR];//="./data/testFile/wordDict_fr.txt";
  //  strcpy(wordDictFile_fr,"wordDict_");
   // char* wordDictFile_en=new char[MAXCHAR];//="./data/testFile/wordDict_en.txt";
  //  strcpy(wordDictFile_en,"wordDict_");
    char* phraseDBFile=new char[MAXCHAR];//="./data/testFile/extractPhraseTrain_POS4.txt"; //the phrase database 
    char* featureRelabelDBFile= new char[MAXCHAR];//="./data/testFile/featureRelabel.txt";                      //the feture relabel file
  //  strcpy(featureRelabelDBFile,"featureRelabel_");
    char* testCorpusFile= new char[MAXCHAR]; //the test file, only collect the features that appear in the test set
    
    
    
    
    /*
    ========================================================================================================
    */
    
    //1.2 Deal with each configulation components
    if (argc<2)
    {
        cerr<<"Error in smt_mainProcess_construct_phraseReorderingDB.cpp: please input the name of the configulation file.\n";
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
                      strcpy(sourceCorpus,directoryName);
                      strcpy(tagsCorpus_fr,sourceCorpus);
                      strcat(tagsCorpus_fr,".tags");
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
                       strcpy(targetCorpus,directoryName);
                       strcpy(tagsCorpus_en,targetCorpus);
                       strcat(tagsCorpus_en,".tags");
                       strcpy(ngramDictFile_en,targetCorpus);
                       strcat(ngramDictFile_en,".ngramDict");
                       strcpy(tagsNgramDictFile_en,targetCorpus);
                       strcat(tagsNgramDictFile_en,".tagsDict");
                       successFlag[1]=1;
                       }
                  
                  //configulation 3. The word alignment corpus
                  else if (strcmp(fileName,"alignmentFile")==0)
                  {
                       //cout<<"2\n";
                       strcpy(wordAlignmentFile,directoryName);
                       successFlag[2]=1;
                       }
                       
                  //configulation 4.  The source word class corpus
                  else if (strcmp(fileName,"SourceWordClassFile")==0)
                  {
                       //cout<<"3\n";
                       strcpy(wordClassFile_fr,directoryName);
                       successFlag[3]=1;
                       }   
                       
                  //configulation 5.  The target word class corpus
                  else if (strcmp(fileName,"TargetWordClassFile")==0)
                  {
                       //cout<<"4\n";
                       strcpy(wordClassFile_en,directoryName);
                       successFlag[4]=1;
                       }  
                       
                  //configulation 6. The phrase pairs table
                  else if (strcmp(fileName,"phraseTableFile")==0)
                  {
                       //cout<<"5\n";
                       strcpy(phraseDBFile,directoryName);
                       strcpy(featureRelabelDBFile,phraseDBFile);
                       strcat(featureRelabelDBFile,".featureRelabel");
                       successFlag[5]=1;
                       }  
                       
                  //configulation 7. maxNgramSize
                  else if (strcmp(fileName,"maxNgramSize")==0)
                  {
                       //cout<<"6\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxNgramSize=intNum;
                       successFlag[6]=1;
                       }  
                  //configulation 8. minPrune
                  else if (strcmp(fileName,"minPrune")==0)
                  {
                       //cout<<"7\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       minPrune=intNum;
                       successFlag[7]=1;
                       }  
                  //configulation 9. window size
                  else if (strcmp(fileName,"windowSize")==0)
                  {
                       //cout<<"8\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       zoneConf[0]=intNum;
                       zoneConf[1]=intNum;
                       successFlag[8]=1;
                       }  
                  //configulation 10. maxPhraseLength
                  else if (strcmp(fileName,"maxPhraseLength")==0)
                  {
                       //cout<<"9\n";
                       istringstream temp(directoryName);
                       int intNum;
                       temp>>intNum;
                       maxPhraseLength=intNum;
                       successFlag[9]=1;
                       }  
                  //configulation 11. test file name
                  else if (strcmp(fileName,"TestFileName")==0 and strcmp(directoryName,"")!=0)
                  {
                       strcpy(testCorpusFile,directoryName);
                       successFlag[10]=1;
                       }   
                                                                             
                  }
              }
        }
    
    
    //check the arguments is completed or not
    if (!successFlag[0])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the source corpus.\n";
        exit(1);}
    else if (!successFlag[1])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the target corpus.\n";
        exit(1);}
    else if (!successFlag[2])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the word alignment corpus.\n";
        exit(1);}
    else if (!successFlag[3])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the source word class corpus.\n";
        exit(1);}
    else if (!successFlag[4])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the target word class corpus.\n";
        exit(1);}
    else if (!successFlag[5])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of the name of the output phrase DB.\n";
        exit(1);}
    else if (!successFlag[6])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of maxNgramSize.\n";
        exit(1);}
    else if (!successFlag[7])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of minPrune.\n";
        exit(1);}
    else if (!successFlag[8])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of windowSize.\n";
        exit(1);}
    else if (!successFlag[9])
       {cerr<<"Error in smt_construct_phraseReorderingDB: There is no definition of maxPhraseLength.\n";
        exit(1);}
    //else check the open state of input files
    else
    {
        //1. source corpus
        ifstream tempFile1(sourceCorpus);
        if (!tempFile1.is_open())
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: can't open the source word corpus.\n";
            exit(1);
            }
        else
        {tempFile1.close();}
        
        //2. target corpus
        ifstream tempFile2(targetCorpus);
        if (!tempFile2.is_open())
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: can't open the target word corpus.\n";
            exit(1);
            }
        else
        {tempFile2.close();}
        
        //3. word alignment file
        ifstream tempFile3(wordAlignmentFile);
        if (!tempFile3.is_open())
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: can't open the word alignment file.\n";
            exit(1);
            }
        else
        {tempFile3.close();}
        
        //4. the source word class file
        ifstream tempFile4(wordClassFile_fr);
        if (!tempFile4.is_open())
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: can't open the source word class file.\n";
            exit(1);
            }
        else
        {tempFile4.close();}
        
        //5. the target word class file
        ifstream tempFile5(wordClassFile_en);
        if (!tempFile5.is_open())
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: can't open the target word class file.\n";
            exit(1);
            }
        else
        {tempFile5.close();}
        
        //6. the phrase database
        if (strcmp(phraseDBFile,"")==0)
        {
            cerr<<"Error in smt_construct_phraseReorderingDB: the name of the phraseDB file is empty.\n";
            exit(1);
            }
            
        //7. the test file (if filled)
        if (successFlag[10]==1)
        {
            ifstream tempFile6(testCorpusFile);
            if (!tempFile6.is_open())
            {
                cerr<<"Error in smt_construct_phraseReorderingDB: can't open the test corpus (optional).\n";
                exit(1);
                }
            else
            {tempFile6.close();}
            }
            
        cout<<"Configulation......\n";
        cout<<"----------------------------------------------------------------\n";
        cout<<"maxNgramSize = "<<maxNgramSize<<'\n';
        cout<<"minPrune = "<<minPrune<<'\n';
        cout<<"windowSize = "<<zoneConf[0]<<'\n';
        cout<<"maxPhraseLength = "<<maxPhraseLength<<'\n';
        cout<<"----------------------------------------------------------------\n\n";
        }
    
       
    
    /*
    ========================================================================================================
    */
    
    //for time test
    time_t time_prev; //start time
    time_t time_next; //end time
    
    //********************************************************************************************************
    //2. Read the word class file
    time_prev=time(NULL);
    cout<<"Step 1. Read the word class file for the source/target corpus and create the tags source/target corpus.\n";
    //2.1 For source text
    wordClassDict wordClassDict_fr=smt_construct_wordDict(wordClassFile_fr, sourceCorpus, tagsCorpus_fr, 1);
    cout<<'\n';
    //2.2 For target text
    wordClassDict wordClassDict_en=smt_construct_wordDict(wordClassFile_en, targetCorpus, tagsCorpus_en, 1);
    cout<<'\n';
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    
    //********************************************************************************************************
    //3. Create the ngram feature dictinary for source/target (word/tags) corpus
    time_prev=time(NULL);
    cout<<"Step 2. Create the ngram feature dictionary for the source/target (words/tags) corpus.\n";
    //3.1 the source words dictionary
    cout<<"2.1 Create the source words dictionary.\n";
    bool successNgramDict;
    
    successNgramDict=smt_construct_phraseNgramDict(sourceCorpus, ngramDictFile_fr, maxNgramSize, minPrune);
    if (!successNgramDict)
    {
        cerr<<"Error in smt_mainProcess_construct_phraseReorderingDB.cpp: can't write the source word dictionary.\n";
        exit(1);
        }
     phraseNgramDict ngramDict_fr(ngramDictFile_fr);
     phraseNgramDict* ngramDict_fr_pointer=&ngramDict_fr;
       
    //cout<<ngramDict_fr_pointer->getNumFeature()<<'\n';
    cout<<'\n';

    //3.2 the target words dictionary
    cout<<"2.2 Create the target words dictionary.\n";
    
    successNgramDict=smt_construct_phraseNgramDict(targetCorpus, ngramDictFile_en, maxNgramSize, minPrune);
    if (!successNgramDict)
    {
        cerr<<"Error in smt_mainProcess_construct_phraseReorderingDB.cpp: can't write the target word dictionary.\n";
        exit(1);
        }
    phraseNgramDict ngramDict_en(ngramDictFile_en);
    phraseNgramDict* ngramDict_en_pointer=&ngramDict_en;
    cout<<'\n';   
    
    //3.3 the source tags dictionary
    cout<<"2.3 Create the source tags dictionary.\n";
    successNgramDict=smt_construct_phraseNgramDict(tagsCorpus_fr, tagsNgramDictFile_fr, maxNgramSize, minPrune);
    if (!successNgramDict)
    {
        cerr<<"Error in smt_mainProcess_construct_phraseReorderingDB.cpp: can't write the target word dictionary.\n";
        exit(1);
        }
    phraseNgramDict tagsDict_fr(tagsNgramDictFile_fr);
    phraseNgramDict* tagsDict_fr_pointer=&tagsDict_fr;
    cout<<'\n';

    //3.4 the source tags dictionary
    cout<<"2.4 Create the target tags dictionary.\n";
    successNgramDict=smt_construct_phraseNgramDict(tagsCorpus_en, tagsNgramDictFile_en, maxNgramSize, minPrune);
    if (!successNgramDict)
    {
        cerr<<"Error in smt_mainProcess_construct_phraseReorderingDB.cpp: can't write the target word dictionary.\n";
        
        }
    phraseNgramDict tagsDict_en(tagsNgramDictFile_en);
    phraseNgramDict* tagsDict_en_pointer=&tagsDict_en;
        
    cout<<'\n';
    time_next=time(NULL);
    cout<<"----------------------------------------------------------------\n";
    cout<<"Processed time: "<<time_next-time_prev<<" seconds.\n";
    cout<<"----------------------------------------------------------------\n\n";
    //********************************************************************************************************
    
    //********************************************************************************************************
    //4. extract phrase pairs and store them in a .txt file
    time_prev=time(NULL);
    
    if (successFlag[6])
    {
        cout<<"Step 3. extract the phrase pairs with ngram features (only for the phrases appear in the test corpus) from the courpus pairs.\n";
        smt_constructPhraseReorderingDB(sourceCorpus, targetCorpus, wordAlignmentFile, tagsCorpus_fr, tagsCorpus_en, phraseDBFile, ngramDict_fr_pointer, ngramDict_en_pointer, tagsDict_fr_pointer, tagsDict_en_pointer, zoneConf, maxPhraseLength, maxNgramSize, featureRelabelDBFile,testCorpusFile);
        }
    else
    {
        cout<<"Step 3. extract the phrase pairs with ngram features from the courpus pairs.\n";
        smt_constructPhraseReorderingDB(sourceCorpus, targetCorpus, wordAlignmentFile, tagsCorpus_fr, tagsCorpus_en, phraseDBFile, ngramDict_fr_pointer, ngramDict_en_pointer, tagsDict_fr_pointer, tagsDict_en_pointer, zoneConf, maxPhraseLength, maxNgramSize, featureRelabelDBFile);
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
