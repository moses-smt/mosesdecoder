/*
=======================================================================================================================
#Remark: the maximum length of a sentence is set as 200! All sentence that exceed this length will cause errors!
The phrase probability prediction function library, including the following functions:
1. smt_sourceClusterPrediction ---  For each source phrae cluster, read the W cluster and predict the reordering probabilities (normalised) for each instance
2. smt_createSourceCluster --- Given a test corpus, extract all source phrases (appear in the training set)
3. smt_collectPhraseOptions --- Create the phrase options for each test sentence
=======================================================================================================================
*/

#include "probPredictionFunction.h"


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
void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFile, phraseFeaturePositionMap sourceFeaturePosition, targetFeatureMap targetTranslation, sentencePhraseOption* phraseOption)
{
     //1. Go through all instances 
     for (phraseFeaturePositionMap::const_iterator indexFound=sourceFeaturePosition.begin(); indexFound!=sourceFeaturePosition.end(); indexFound++)
     {
         
         vector<int> sourceFeature; //store the source features
         string tempString;         //store the feature string
         int tempFeature;           //store the feature
         unsigned short boundary[2]={indexFound->first.left_boundary,indexFound->first.right_boundary}; //get the boundary
         sourceFeatureFile.seekg(indexFound->second,ios::beg); //get the begining of the feature list
         getline(sourceFeatureFile,tempString,'\n');
         istringstream featureString(tempString);
         
         
         while (featureString>>tempFeature)
               sourceFeature.push_back(tempFeature);
               
         //2. For each instance, go through all translations
         mapTargetProbOption targetProbs; //store the target index -> vector<float> probs
         for (targetFeatureMap::const_iterator targetFound=targetTranslation.begin();targetFound!=targetTranslation.end();targetFound++)
         {
             //2.1 Get the reordering probabilities
             vector<float> probValues=wt->structureLearningConfidence(sourceFeature,targetFound->second);
             //2.2 Update the probabilities
             targetProbs[targetFound->first]=probValues;
             }
             
         //3. Update the sentence phrase options
         phraseOption->createPhraseOption(indexFound->first.sentenceIndex,boundary, targetProbs);
         }
     }

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
//void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFile, phraseFeaturePositionMap sourceFeaturePosition, targetFeatureMapSTR targetTranslation, sentencePhraseOptionSTR* phraseOption)
void smt_sourceClusterPrediction(weightClusterW* wt, ifstream& sourceFeatureFile, phraseFeaturePositionMap sourceFeaturePosition, sourceTargetFeatureMapSTR::const_iterator sourceTargetFound, sentencePhraseOptionSTR* phraseOption)
{
     //1. Go through all instances 
     for (phraseFeaturePositionMap::const_iterator indexFound=sourceFeaturePosition.begin(); indexFound!=sourceFeaturePosition.end(); indexFound++)
     {
         
         vector<int> sourceFeature; //store the source features
         string tempString;         //store the feature string
         int tempFeature;           //store the feature
         unsigned short boundary[2]={indexFound->first.left_boundary,indexFound->first.right_boundary}; //get the boundary
         sourceFeatureFile.seekg(indexFound->second,ios::beg); //get the begining of the feature list
         getline(sourceFeatureFile,tempString,'\n');
         istringstream featureString(tempString);
         
         //Get the number of features (then reserve the memory)
         featureString>>tempFeature; 
         sourceFeature.reserve(tempFeature);
         
         while (featureString>>tempFeature)
               sourceFeature.push_back(tempFeature);
               
         //2. For each instance, go through all translations
         mapTargetProbOptionSTR targetProbs; //store the target string -> vector<float> probs
         for (targetFeatureMapSTR::const_iterator targetFound=sourceTargetFound->second.begin(); targetFound!=sourceTargetFound->second.end(); targetFound++)
         {
             //2.1 Get the reordering probabilities
             vector<float> probValues=wt->structureLearningConfidence(sourceFeature,targetFound->second);
             //2.2 Update the probabilities
             targetProbs[targetFound->first]=probValues;
             }
             
         //3. Update the sentence phrase options
         phraseOption->createPhraseOption(boundary, targetProbs);
         }
     }


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
void smt_createSourceCluster(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseReorderingTable* trainingPhraseTable, char* outputFileName,  sourcePositionMap* sourcePositionDict)
{
     //1. Initialisation
     ifstream inputFile(inputFileName,ios::binary); //test source corpus
     string sourceSentence;                         //store each source sentence
     ofstream outputFile(outputFileName,ios::out);  //output the source phrase ||| sentence_index boundary ||| features
     int countSen=0;                                //the sentence index
     
     //1*. Check whether the corpora is open or not
     if (!inputFile.is_open())
         {cerr<<"Error in probPredictionFunction.cpp: Can't open the source corpus!\n";
         exit (1);}
     
     //2. For each sentence
     while (getline(inputFile,sourceSentence,'\n'))
     {
           //2.1 create string array pointer to the source word/tag string
           sentenceArray* sentenceFR=new sentenceArray(sourceSentence);
           sentenceArray* tagFR=new sentenceArray(sourceSentence, wordDict_fr);
           
           //2.2 Get sentence length
           int Nf = sentenceFR->getSentenceLength();
           
           
           //3 For get each ngram
           for (int ngram=1;ngram<=maxPhraseLength;ngram++)
           {
               for (int i=0; i<=Nf-ngram;i++)
               {
                   //3.1 Get source string
                   string sourceString = sentenceFR->getPhraseFromSentence(i,i+ngram-1);
                   int targetTranslations = trainingPhraseTable->getNumberofTargetTranslation(sourceString);
                   
                   
                   //If the source phrase appeared in the training set
                   if (targetTranslations>0)
                   {
                       //3.2 Get the ngram features
                       vector<int> featureBlock;
                       int zoneL=max(0,i-zoneConf[0]);          //the left boundary
                       int zoneR=i;                             //the right boundary
                       
                       //3.2.1 left side source word fetures
                       vector<int> nGram_fr_l=smt_extract_ngramFeature(sentenceFR, ngramDictFR, zoneL, zoneR, 0, maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nGram_fr_l.begin(),nGram_fr_l.end());
                       //3.2.2 left side source tags features
                       vector<int> nTag_fr_l=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,10, maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nTag_fr_l.begin(),nTag_fr_l.end());
                       //3.2.3 right side source word features
                       zoneL=i+ngram;
                       zoneR=min(Nf,zoneL+zoneConf[1]); 
                       vector<int> nGram_fr_r=smt_extract_ngramFeature(sentenceFR,ngramDictFR,zoneL,zoneR,1,maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nGram_fr_r.begin(),nGram_fr_r.end());
                       //3.2.4 right side source tag features
                       vector<int> nTag_fr_r=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,11,maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nTag_fr_r.begin(),nTag_fr_r.end());
                       
                       
                       //3.2.5 transform the features to the relabeled features
                       for (int j=0; j<featureBlock.size();j++)
                           featureBlock[j]=relabelDict->getRelabeledFeature(featureBlock[j]);
                           
                       
                           
                       //3.2.6 write the source features .txt file
                       outputFile<<sourceString<<" ||| "<<countSen<<" "<<i<<" "<<i+ngram-1<<" ||| ";
                       
                       unsigned long long startPos=outputFile.tellp();     //record the start position of this cluster
                       
                       
                       
                       for (int j=0; j<featureBlock.size();j++)
                           outputFile<<featureBlock[j]<<" ";
                       outputFile<<'\n';
                       
                       
                       //3.3 Update sourcePositionDict
                       //store the sentence index and the boundary of the source phrase in the sentence
                       phraseOptionIndex sentenceInfo(countSen, (unsigned short) i, (unsigned short) i+ngram-1);  
                       
                       sourcePositionMap::iterator sourcePhraseFound=sourcePositionDict->find(sourceString);
                              
               
                       if (sourcePhraseFound==sourcePositionDict->end())
                       {
                           phraseFeaturePositionMap tempPhraseFeaturePosition;
                           tempPhraseFeaturePosition.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                           sourcePositionDict->insert(pair<string,phraseFeaturePositionMap>(sourceString,tempPhraseFeaturePosition));                                                                                                      
                            }
                       else
                       {
                           sourcePhraseFound->second.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                           }
                       }
                   }
               }
               
           //4. The end of the process for each test sentence
           delete sentenceFR; //release the memory
           delete tagFR;
           countSen++; //update the sentence index
           if (countSen%100==0)
           {
               cout<<".";
               if (countSen%1000==0)
                  cout<<"\nHave processed "<<countSen<<" test sentences.\n";
               }
              
           
           }
     cout<<"All together processed "<<countSen<<" test sentence.\n";
     inputFile.close();
     outputFile.close();
     }




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
void smt_createSourceCluster(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* outputFileName,  sourcePositionMap* sourcePositionDict)
{
     //1. Initialisation
     ifstream inputFile(inputFileName,ios::binary); //test source corpus
     string sourceSentence;                         //store each source sentence
     ofstream outputFile(outputFileName,ios::out);  //output the source phrase ||| sentence_index boundary ||| features
     int countSen=0;                                //the sentence index
     
     //1*. Check whether the corpora is open or not
     if (!inputFile.is_open())
         {cerr<<"Error in probPredictionFunction.cpp: Can't open the source corpus!\n";
         exit (1);}
     
     //2. For each sentence
     while (getline(inputFile,sourceSentence,'\n'))
     {
           //2.1 create string array pointer to the source word/tag string
           sentenceArray* sentenceFR=new sentenceArray(sourceSentence);
           sentenceArray* tagFR=new sentenceArray(sourceSentence, wordDict_fr);
           
           //2.2 Get sentence length
           int Nf = sentenceFR->getSentenceLength();
           
           
           //3 For get each ngram
           for (int ngram=1;ngram<=maxPhraseLength;ngram++)
           {
               for (int i=0; i<=Nf-ngram;i++)
               {
                   //3.1 Get source string
                   string sourceString = sentenceFR->getPhraseFromSentence(i,i+ngram-1);
                   int targetTranslations = trainingPhraseTable->getNumberofTargetTranslation(sourceString);
                   
                   
                   //If the source phrase appeared in the training set
                   if (targetTranslations>0)
                   {
                       //3.2 Get the ngram features
                       vector<int> featureBlock;
                       int zoneL=max(0,i-zoneConf[0]);          //the left boundary
                       int zoneR=i;                             //the right boundary
                       
                       //3.2.1 left side source word fetures
                       vector<int> nGram_fr_l=smt_extract_ngramFeature(sentenceFR, ngramDictFR, zoneL, zoneR, 0, maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nGram_fr_l.begin(),nGram_fr_l.end());
                       //3.2.2 left side source tags features
                       vector<int> nTag_fr_l=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,10, maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nTag_fr_l.begin(),nTag_fr_l.end());
                       //3.2.3 right side source word features
                       zoneL=i+ngram;
                       zoneR=min(Nf,zoneL+zoneConf[1]); 
                       vector<int> nGram_fr_r=smt_extract_ngramFeature(sentenceFR,ngramDictFR,zoneL,zoneR,1,maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nGram_fr_r.begin(),nGram_fr_r.end());
                       //3.2.4 right side source tag features
                       vector<int> nTag_fr_r=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,11,maxNgramSize);
                       featureBlock.insert(featureBlock.end(),nTag_fr_r.begin(),nTag_fr_r.end());
                       
                       
                       //3.2.5 transform the features to the relabeled features
                       for (int j=0; j<featureBlock.size();j++)
                           featureBlock[j]=relabelDict->getRelabeledFeature(featureBlock[j]);
                           
                       
                           
                       //3.2.6 write the source features .txt file
                       outputFile<<sourceString<<" ||| "<<countSen<<" "<<i<<" "<<i+ngram-1<<" ||| ";
                       
                       unsigned long long startPos=outputFile.tellp();     //record the start position of this cluster
                       
                       
                       for (int j=0; j<featureBlock.size();j++)
                           outputFile<<featureBlock[j]<<" ";
                       outputFile<<'\n';
                       
                       
                       //3.3 Update sourcePositionDict
                       //store the sentence index and the boundary of the source phrase in the sentence
                       phraseOptionIndex sentenceInfo(countSen, (unsigned short) i, (unsigned short) i+ngram-1);  
                       
                       
                       sourcePositionMap::iterator sourcePhraseFound=sourcePositionDict->find(sourceString);
                              
               
                       if (sourcePhraseFound==sourcePositionDict->end())
                       {
                           phraseFeaturePositionMap tempPhraseFeaturePosition;
                           tempPhraseFeaturePosition.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                           sourcePositionDict->insert(pair<string,phraseFeaturePositionMap>(sourceString,tempPhraseFeaturePosition));                                                                                                      
                            }
                       else
                       {
                           //phraseFeaturePositionMap::const_iterator tempKeyFound=sourcePhraseFound->second.find(sentenceInfo);
                           //if (tempKeyFound!=sourcePhraseFound->second.end())
                           //   cerr<<"Warning in creating source clusters, overlap found in:"<<sentenceInfo.sentenceIndex<<" "<<sentenceInfo.left_boundary<<" "<<sentenceInfo.right_boundary<<"\n.";
                           sourcePhraseFound->second.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                           }
                       }
                   }
               }
               
           //4. The end of the process for each test sentence
           delete sentenceFR; //release the memory
           delete tagFR;
           countSen++; //update the sentence index
           if (countSen%100==0)
           {
               cout<<".";
               if (countSen%1000==0)
                  cout<<"\nHave processed "<<countSen<<" test sentences.\n";
               }
              
           
           }
     cout<<"All together processed "<<countSen<<" test sentence.\n";
     inputFile.close();
     outputFile.close();
     }

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
void smt_createSourceCluster(string sourceSentence, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* outputFileName, sourcePositionMap* sourcePositionDict)
{
     //1. Initialisation
     ofstream outputFile(outputFileName,ios::out);  //output the source phrase ||| sentence_index boundary ||| features
     
     
     //2. For the test sentence
     //2.1 create string array pointer to the source word/tag string
     sentenceArray* sentenceFR=new sentenceArray(sourceSentence);
     sentenceArray* tagFR=new sentenceArray(sourceSentence, wordDict_fr);
     
     //2.2 Get sentence length
     int Nf = sentenceFR->getSentenceLength();
     
      //3 For get each ngram
      for (int ngram=1;ngram<=maxPhraseLength;ngram++)
      {
          for (int i=0; i<=Nf-ngram;i++)
          {
          //3.1 Get source string
          string sourceString = sentenceFR->getPhraseFromSentence(i,i+ngram-1);
          int targetTranslations = trainingPhraseTable->getNumberofTargetTranslation(sourceString);
          //If the source phrase appeared in the training set
          if (targetTranslations>0)
          {
              //3.2 Get the ngram features
              vector<int> featureBlock;
              int zoneL=max(0,i-zoneConf[0]);          //the left boundary
              int zoneR=i;                             //the right boundary
                       
              //3.2.1 left side source word fetures
              vector<int> nGram_fr_l=smt_extract_ngramFeature(sentenceFR, ngramDictFR, zoneL, zoneR, 0, maxNgramSize);
              featureBlock.insert(featureBlock.end(),nGram_fr_l.begin(),nGram_fr_l.end());
              //3.2.2 left side source tags features
              vector<int> nTag_fr_l=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,10, maxNgramSize);
              featureBlock.insert(featureBlock.end(),nTag_fr_l.begin(),nTag_fr_l.end());
              //3.2.3 right side source word features
              zoneL=i+ngram;
              zoneR=min(Nf,zoneL+zoneConf[1]); 
              vector<int> nGram_fr_r=smt_extract_ngramFeature(sentenceFR,ngramDictFR,zoneL,zoneR,1,maxNgramSize);
              featureBlock.insert(featureBlock.end(),nGram_fr_r.begin(),nGram_fr_r.end());
              //3.2.4 right side source tag features
              vector<int> nTag_fr_r=smt_extract_ngramFeature(tagFR,tagsDict_fr,zoneL,zoneR,11,maxNgramSize);
              featureBlock.insert(featureBlock.end(),nTag_fr_r.begin(),nTag_fr_r.end());
                       
              
                   
              //3.2.5 transform the features to the relabeled features
              for (int j=0; j<featureBlock.size();j++)
                  featureBlock[j]=relabelDict->getRelabeledFeature(featureBlock[j]);
                                                                                          
              //3.2.6 write the source features .txt file (source string ||| index[0] index[1] ||| features
              outputFile<<sourceString<<" ||| "<<i<<" "<<i+ngram-1<<" ||| ";    
              unsigned long long startPos=outputFile.tellp();     //record the start position of this cluster
              
              //*. test, output the size of the feature block as well (for reserve of vector)
              outputFile<<featureBlock.size()<<" ";    
              for (int j=0; j<featureBlock.size();j++)
                  outputFile<<featureBlock[j]<<" ";
              outputFile<<'\n';
              
              //3.3 Update sourcePositionDict
              //store the sentence index and the boundary of the source phrase in the sentence
              phraseOptionIndex sentenceInfo(0, (unsigned short) i, (unsigned short) i+ngram-1);  
                       
                       
              sourcePositionMap::iterator sourcePhraseFound=sourcePositionDict->find(sourceString);
                              
               //sourcePositionDict: sourcePhrsae-> <index[0],index[1]> -> feature start pos in the feature .txt file
               if (sourcePhraseFound==sourcePositionDict->end())
               {
                   phraseFeaturePositionMap tempPhraseFeaturePosition;
                   tempPhraseFeaturePosition.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                   sourcePositionDict->insert(pair<string,phraseFeaturePositionMap>(sourceString,tempPhraseFeaturePosition));
                                                                 
                   }
               else
               {
                   sourcePhraseFound->second.insert(pair<phraseOptionIndex, unsigned long long>(sentenceInfo,startPos));
                   }
               }
              }
          }
      outputFile.close();
      //To release the memory
      delete sentenceFR;
      delete tagFR;
      }
                       
                   
     
     
     
     

/******************************************************************************
C. Function:
Create the phrase options for each test sentence
phrase option format: sentenceIndex->[left_boundary,right_boundary]->target translations (index) -> reordering probabilities
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
sentencePhraseOption smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseReorderingTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix)
{
    //1. Initialisation
    sentencePhraseOption phraseOption;           //store the phrase option for each sentence
    sentencePhraseOption* phraseOptionPointer=&phraseOption; //the pointer to the phrase option
    sourceTargetFeatureMap* sourceTargetFeature = new sourceTargetFeatureMap; //store the source->target->feature
    sourcePositionMap* sourcePositionDict = new sourcePositionMap;       //store the source-><sentenceIndex,position>->start pos for source features
    char tempSourceFeatureFileName[]="temp_storeSourceFeature";
    ifstream weightFile(weightFileName,ios::binary); //open the weight matrix file
    int classSetup=trainingPhraseTable->getNumOrientatin();//the class setup
    
    
    //2. Create the sourceTargetFeatureMap
    //2.1 For each source phase
    vector<string> clusterNames = trainingPhraseTable->getClusterNames();
    for (int i=0; i<clusterNames.size();i++)
    {
        string sourceString=clusterNames[i]; //the source string
        targetFeatureMap targetTranslationDict;
        //2.2 Get the target translations
        vector<string> targetTranslations = trainingPhraseTable->getTargetTranslation(sourceString);
        
        //2.*. using unsigned short for the target index, so need to check the number of translations
        int numTargetTranslation = targetTranslations.size();
        unsigned short translationBoundary;
        if (numTargetTranslation>numeric_limits<unsigned short>::max())
           {cerr<<"Warning in probPredictionFunction.cpp: the number of target translations for source phrase '"<<sourceString<<"' exceeds 65535 (max size of unsigned short).\n";
            translationBoundary=numeric_limits<unsigned short>::max();}
        else
            translationBoundary=(unsigned short) numTargetTranslation;
        
        
        for (unsigned short j=0;j<translationBoundary;j++)
        {
            //2.3 For each translation
            string target = targetTranslations[j];
            sentenceArray* sentenceEN=new sentenceArray(target);
            sentenceArray* tagEN=new sentenceArray(target, wordDict_en);
            vector<int> targetFeature;
            int Ne = sentenceEN->getSentenceLength();
            
            //target word features
            vector<int> nGram_en=smt_extract_ngramFeature(sentenceEN, ngramDictEN, 0, Ne, 2, maxNgramSize);
            targetFeature.insert(targetFeature.end(),nGram_en.begin(),nGram_en.end());
         
             //target tags features
             vector<int> nTag_en=smt_extract_ngramFeature(tagEN, tagsDict_en, 0, Ne, 22, maxNgramSize);
             targetFeature.insert(targetFeature.end(),nTag_en.begin(),nTag_en.end());
                               
             //relabel the features
             for (int kk=0;kk<targetFeature.size();kk++)
                 targetFeature[kk]=relabelDict->getRelabeledFeature(targetFeature[kk]);
                                   
             //update the targetTranslationDict
             targetTranslationDict[j]=targetFeature;
             
             //delete the new pointers (when use new, remember to delete after using)
             delete sentenceEN;
             delete tagEN;
             } 
         //2.3 Update the sourceTargetMap
         sourceTargetFeature->insert(pair<string,targetFeatureMap>(sourceString, targetTranslationDict));
        }
    
    //3. Collect the source clusters with the features
    smt_createSourceCluster(inputFileName,ngramDictFR, ngramDictEN, tagsDict_fr, tagsDict_en, wordDict_fr, wordDict_en,maxPhraseLength,maxNgramSize,zoneConf, relabelDict, trainingPhraseTable, tempSourceFeatureFileName,  sourcePositionDict);
    ifstream sourceFeatureFile(tempSourceFeatureFileName,ios::binary); //re-open the source feature file
    
    //4. For each source phrase clusters
    for (sourcePositionMap::const_iterator sourceFound=sourcePositionDict->begin();sourceFound!=sourcePositionDict->end();sourceFound++)
    {
        //4.1 Get the source phrase and find the weight matrix
        string sourcePhrase=sourceFound->first;
        unsigned long long weightStartPos=weightMatrix->getWeightClusterPOS(sourcePhrase);
        
        //4.2 If no such weight matrix
        if (weightStartPos==numeric_limits<unsigned long long>::max())
        {
            cerr<<"Warning in probPredictionFunction.cpp: no weight cluster for the source phrase:"<<sourcePhrase<<".\n";
            }
        else
        {
            //4.3 Get the weight cluster
            sourceTargetFeatureMap::const_iterator targetFound=sourceTargetFeature->find(sourcePhrase);
            weightClusterW* wt = new weightClusterW(weightFile, classSetup, weightStartPos);
            
            //4.4 Update the phrase options that contain the source phrase
            smt_sourceClusterPrediction(wt, sourceFeatureFile, sourceFound->second, targetFound->second, phraseOptionPointer);
            delete wt;
            }
        
        }
    
    //5. the end
    sourceFeatureFile.close();
    weightFile.close();
    cout<<"Remove some temp files.\n";
    if (remove(tempSourceFeatureFileName)==-1)
       cerr<<"Warning in probPredictionFunction.cpp: remove temp files failed.\n";
    return phraseOption;
    
    
    }

/******************************************************************************
C*.(overloaded) Function:
Create the phrase options for each test sentence
phrase option format: sentenceIndex->[left_boundary,right_boundary]->target translations (index) -> reordering probabilities
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
sentencePhraseOption smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix, int classSetup)
{
    //1. Initialisation
    sentencePhraseOption phraseOption;           //store the phrase option for each sentence
    sentencePhraseOption* phraseOptionPointer=&phraseOption; //the pointer to the phrase option
    sourceTargetFeatureMap* sourceTargetFeature = new sourceTargetFeatureMap; //store the source->target->feature
    sourcePositionMap* sourcePositionDict = new sourcePositionMap;       //store the source-><sentenceIndex,position>->start pos for source features
    char tempSourceFeatureFileName[]="temp_storeSourceFeature";
    ifstream weightFile(weightFileName,ios::binary); //open the weight matrix file
    
    
    //2. Create the sourceTargetFeatureMap
    //2.1 For each source phase
    vector<string> clusterNames = trainingPhraseTable->getClusterNames();
    for (int i=0; i<clusterNames.size();i++)
    {
        string sourceString=clusterNames[i]; //the source string
        targetFeatureMap targetTranslationDict;
        //2.2 Get the target translations
        vector<string> targetTranslations = trainingPhraseTable->getTargetTranslation(sourceString);
        //2.*. using unsigned short for the target index, so need to check the number of translations
        int numTargetTranslation = targetTranslations.size();
        unsigned short translationBoundary;
        
        if (numTargetTranslation>numeric_limits<unsigned short>::max())
           {cerr<<"Warning in probPredictionFunction.cpp: the number of target translations for source phrase '"<<sourceString<<"' exceeds 65535 (max size of unsigned short).\n";
            translationBoundary=numeric_limits<unsigned short>::max();}
        else
            translationBoundary=(unsigned short) numTargetTranslation;
            
        
        for (unsigned short j=0;j<translationBoundary;j++)
        {
            //2.3 For each translation
            string target = targetTranslations[j];
            sentenceArray* sentenceEN=new sentenceArray(target);
            sentenceArray* tagEN=new sentenceArray(target, wordDict_en);
            vector<int> targetFeature;
            int Ne = sentenceEN->getSentenceLength();
        
            //target word features
            vector<int> nGram_en=smt_extract_ngramFeature(sentenceEN, ngramDictEN, 0, Ne, 2, maxNgramSize);
            targetFeature.insert(targetFeature.end(),nGram_en.begin(),nGram_en.end());
         
             //target tags features
             vector<int> nTag_en=smt_extract_ngramFeature(tagEN, tagsDict_en, 0, Ne, 22, maxNgramSize);
             targetFeature.insert(targetFeature.end(),nTag_en.begin(),nTag_en.end());
                               
             //relabel the features
             for (int kk=0;kk<targetFeature.size();kk++)
                 targetFeature[kk]=relabelDict->getRelabeledFeature(targetFeature[kk]);
                                   
             //update the targetTranslationDict
             targetTranslationDict[j]=targetFeature;
             
             //delete the new pointers (when use new, remember to delete after using)
             delete sentenceEN;
             delete tagEN;
             } 
         //2.3 Update the sourceTargetMap
         sourceTargetFeature->insert(pair<string,targetFeatureMap>(sourceString, targetTranslationDict));
        }
    
//    cout<<"Finish constructing the source->target map.\n";
    
    //3. Collect the source clusters with the features
    smt_createSourceCluster(inputFileName,ngramDictFR, ngramDictEN, tagsDict_fr, tagsDict_en, wordDict_fr, wordDict_en,maxPhraseLength,maxNgramSize,zoneConf, relabelDict, trainingPhraseTable, tempSourceFeatureFileName,  sourcePositionDict);
    ifstream sourceFeatureFile(tempSourceFeatureFileName,ios::binary); //re-open the source feature file
    
 //   cout<<"Finish constructing the source feature map.\n";
    
    //4. For each source phrase clusters
    int tempCount=0;
    for (sourcePositionMap::const_iterator sourceFound=sourcePositionDict->begin();sourceFound!=sourcePositionDict->end();sourceFound++)
    {
        //4.1 Get the source phrase and find the weight matrix
        string sourcePhrase=sourceFound->first;
        unsigned long long weightStartPos=weightMatrix->getWeightClusterPOS(sourcePhrase);
        
        //4.2 If no such weight matrix
        if (weightStartPos==numeric_limits<unsigned long long>::max())
        {
           // cerr<<"Warning in probPredictionFunction.cpp: no weight cluster for the source phrase:"<<sourcePhrase<<".\n";
            }
        else
        {
            //4.3 Get the weight cluster
            sourceTargetFeatureMap::const_iterator targetFound=sourceTargetFeature->find(sourcePhrase);
            weightClusterW* wt = new weightClusterW(weightFile, classSetup, weightStartPos);
            
            //4.4 Update the phrase options that contain the source phrase
            smt_sourceClusterPrediction(wt, sourceFeatureFile, sourceFound->second, targetFound->second, phraseOptionPointer);
            delete wt;
            }
            
        tempCount++;
        if (tempCount%100==0)
        {
            cout<<'.';
            if (tempCount%2000==0)
            cout<<'\n';
            }
       
        }
    
    //5. the end
    sourceFeatureFile.close();
    weightFile.close();
   cout<<"Remove some temp files.\n";
    if (remove(tempSourceFeatureFileName)==-1)
       cerr<<"Warning in probPredictionFunction.cpp: remove temp files failed.\n";
  
    return phraseOption;
    
    }


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
void smt_collectPhraseOptions(char* inputFileName, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDict_fr, phraseNgramDict* tagsDict_en, wordClassDict* wordDict_fr, wordClassDict* wordDict_en, int maxPhraseLength, int maxNgramSize, int zoneConf[], relabelFeature* relabelDict, phraseTranslationTable* trainingPhraseTable, char* weightFileName, weightMatrixW* weightMatrix, int classSetup, char* outPhraseOptionFileName)
{
    //1. Initialisation
    sourceTargetFeatureMapSTR* sourceTargetFeature = new sourceTargetFeatureMapSTR; //store the source->target->feature
    char tempSourceFeatureFileName[]="temp_storeSourceFeature";
    ifstream weightFile(weightFileName,ios::binary); //open the weight matrix file
    ofstream outPhraseOptionFile(outPhraseOptionFileName,ios::out);//to write the phrase options
    char* phraseOptionDBPOSName=new char[MAXCHAR]; //the start position of each sentence phrase option
    strcpy(phraseOptionDBPOSName,outPhraseOptionFileName);
    strcat(phraseOptionDBPOSName,".startPosition");
    ofstream phraseOptionDBPOS(phraseOptionDBPOSName,ios::out); 
    
    //2. Create the sourceTargetFeatureMap
    //2.1 For each source phase
    vector<string> clusterNames = trainingPhraseTable->getClusterNames();
    for (int i=0; i<clusterNames.size();i++)
    {
        string sourceString=clusterNames[i]; //the source string
        targetFeatureMapSTR targetTranslationDict;
        //2.2 Get the target translations
        vector<string> targetTranslations = trainingPhraseTable->getTargetTranslation(sourceString);
        for (int j=0;j<targetTranslations.size();j++)
        {
            //2.3 For each translation
            string target = targetTranslations[j];
            sentenceArray* sentenceEN=new sentenceArray(target);
            sentenceArray* tagEN=new sentenceArray(target, wordDict_en);
            vector<int> targetFeature;
            int Ne = sentenceEN->getSentenceLength();
            //target word features
            vector<int> nGram_en=smt_extract_ngramFeature(sentenceEN, ngramDictEN, 0, Ne, 2, maxNgramSize);
            targetFeature.insert(targetFeature.end(),nGram_en.begin(),nGram_en.end());
         
             //target tags features
             vector<int> nTag_en=smt_extract_ngramFeature(tagEN, tagsDict_en, 0, Ne, 22, maxNgramSize);
             targetFeature.insert(targetFeature.end(),nTag_en.begin(),nTag_en.end());
                               
             //relabel the features
             for (int kk=0;kk<targetFeature.size();kk++)
                 targetFeature[kk]=relabelDict->getRelabeledFeature(targetFeature[kk]);
                                   
             //update the targetTranslationDict
             targetTranslationDict[target]=targetFeature;
             
             //delete the new pointers (when use new, remember to delete after using)
             delete sentenceEN;
             delete tagEN;
             } 
         //2.3 Update the sourceTargetMap
         sourceTargetFeature->insert(pair<string,targetFeatureMapSTR>(sourceString, targetTranslationDict));
        }
                           
    //3. Extract the phrase options
    ifstream inputFile(inputFileName,ios::binary); //test source corpus
    int countSen=0;                                //the sentence index
    string sourceSentence;                         //store the test source sentence
    
    //for each test sentence 
    while (getline(inputFile,sourceSentence))
    {
          //3.1 initialisation 
          sourcePositionMap* sourcePositionDict = new sourcePositionMap;       //store the source-><sentenceIndex,position>->start pos for source features
          sentencePhraseOptionSTR phraseOption;           //store the phrase option for each sentence
          sentencePhraseOptionSTR* phraseOptionPointer=&phraseOption; //the pointer to the phrase option
          
          //3.2 Collect the source clusters with the features
          smt_createSourceCluster(sourceSentence,ngramDictFR, ngramDictEN, tagsDict_fr, tagsDict_en, wordDict_fr, wordDict_en,maxPhraseLength,maxNgramSize,zoneConf, relabelDict, trainingPhraseTable, tempSourceFeatureFileName, sourcePositionDict);
          ifstream sourceFeatureFile(tempSourceFeatureFileName,ios::binary); //re-open the source feature file
          
          //3.4 For each source phrase clusters
          for (sourcePositionMap::const_iterator sourceFound=sourcePositionDict->begin();sourceFound!=sourcePositionDict->end();sourceFound++)
          {
              //3.1 Get the source phrase and find the weight matrix
              string sourcePhrase=sourceFound->first;
              unsigned long long weightStartPos=weightMatrix->getWeightClusterPOS(sourcePhrase);
        
              //3.2 If no such weight matrix
              if (weightStartPos==numeric_limits<unsigned long long>::max())
              {
                  cerr<<"Warning in probPredictionFunction.cpp: no weight cluster for the source phrase:"<<sourcePhrase<<".\n";
                  }
              else
              {
                  //3.3 Get the weight cluster
                  sourceTargetFeatureMapSTR::const_iterator targetFound=sourceTargetFeature->find(sourcePhrase);
                  weightClusterW* wt = new weightClusterW(weightFile, classSetup, weightStartPos);
            
                  //3.4 Update the phrase options that contain the source phrase
                  //smt_sourceClusterPrediction(wt, sourceFeatureFile, sourceFound->second, targetFound->second, phraseOptionPointer);
                  smt_sourceClusterPrediction(wt, sourceFeatureFile, sourceFound->second, targetFound, phraseOptionPointer);
                  delete wt; //delete the pointer (when use new, remember to delete after using) 
                  }   
              }
              
          //3.5 Output the phrases in the phrase option file (.txt file)
          unsigned long long tempStartPos=outPhraseOptionFile.tellp();     //record the start position of this sentence option
          phraseOptionDBPOS<<tempStartPos<<'\n';
          phraseOptionPointer->outputPhraseOption(outPhraseOptionFile);
          
          
          //4. The end of the process for each test sentence
          delete sourcePositionDict; //(when use new, remember to delete after using)
          
          sourceFeatureFile.close();
          if (remove(tempSourceFeatureFileName)==-1)
             cerr<<"Warning in probPredictionFunction.cpp: remove temp files failed.\n";
          countSen++; //update the sentence index
          if (countSen%100==0)
          {
               cout<<".";
               if (countSen%1000==0)
                  cout<<"\nHave processed "<<countSen<<" test sentences.\n";
               }
          }                      
    

    
    //4. the end
    inputFile.close();
    weightFile.close();
    outPhraseOptionFile.close();
    phraseOptionDBPOS.close();
    }
