/*
=======================================================================================================================
#Remark: the maximum length of a sentence is set as 200! All sentence that exceed this length will cause errors!
The phrase construction function library, including the following functions:
1. smt_construct_phraseNgramDict --- Construct the dictionary (writing a .txt dictionary file, which can be read by a phraseNgramDict class)
2. smt_construct_wordDict --- Read the word class labels and construct the tags corpus for training/test corpus (be read by a wordClassDict class)
3. --- given the (source/target) sentence and the zone, extract ngram features in these environment (return a vector?)
4. smt_consistPhrasePair --- extract phrase pairs from a sentence pair with its word alignments.
4*. smt_consistPhrasePair --- overload extract phrase pairs (only appear in the test set) from a sentence pair with its word alignments.
=======================================================================================================================
*/

#include "phraseConstructionFunction.h"

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

bool smt_construct_phraseNgramDict(char* inputCorpusFile, char* ngramDictFile, int maxNgram, int minPrune)
{
     //1. Define a dictionary
    phraseNgramDict ngramDict;      //define an empty dictionary
    int countSentence=0;           //count the sentence processed
    //2. read the corpus
    ifstream corpus;
    corpus.open(inputCorpusFile,ios::in);
    string eachSentence;
    
    if (!corpus.is_open())
    {
       cerr<<"Can't open the input corpus file!\n";
       return 0; //error call
    }
    
    //3. for each corpus
    while (getline(corpus,eachSentence,'\n'))//get each sentence
    {
          countSentence++;
          //3.1 initialization
          istringstream str_eachSentence(eachSentence); //convert it to str-stream
          char tempWord[100];            //store each word (assign a space to avoid potential error)
          int numWord=0;              //store the number of words
          string sentenceArray[200]; //maximum-number of words (200) declare a sentence array
          
          //3.2 Get each word of the sentence
          while (str_eachSentence>>tempWord)
                {
                   sentenceArray[numWord]=tempWord; //Get the words
                   numWord++;                       //update the number of words
                }
                   
           //3.3. get each ngram from the sentence array and update the dictionary                         } 
           for (int ngram=1; ngram<=maxNgram; ngram++)
              for (int i=0; i<=numWord-ngram; i++)
                  {
                  string ngramKey;                    //define the ngram key 
                  ngramKey="";                        //empty string key
                  for (int j=i;j<i+ngram;j++)
                     { 
                      ngramKey+=sentenceArray[j];    //Get the key
                      if (j<i+ngram-1)
                         ngramKey+=" ";
                         }
                  
                   ngramDict.insertNgram(ngramKey,ngram); //insert the ngram feature
                   }
           if (countSentence%5000==0)
              cout<<"Have already process "<<countSentence<<" sentences.\n";                           
    }  
    cout<<"All together process "<<countSentence<<" sentences.\n";       
    
    
    //4. finally output the dictionary into a .txt file
    ngramDict.outputNgramDict(ngramDictFile,minPrune);   
    
    if (ngramDict.checkReadFileStatus()) //if read file successful/or 
        return 1;
    else
        {
        cerr<<"Can't not create the dictionary.\n";
        return 0;
        }
     }



/******************************************************************************
A*. Function (overloaded):
Construct the ngram dictionary for the source/target corpus (and tags corpus)
Input:
1.  inputCorpus --- the input corpus file (.txt)
2.  maxNgram --- the maximum length of the ngram features (default 3)
3.  minPrune --- prune the ngram features which occur < minPrune 
4. overloadFlag --- (bool type value, for overloaded function only)
Output:
1.  ngramDictFile --- the output ngram dictionary file (.txt) 
******************************************************************************/

phraseNgramDict smt_construct_phraseNgramDict(char* inputCorpusFile, char* ngramDictFile, int maxNgram, int minPrune, bool overloadFlag)

{
     //1. Define a dictionary
    phraseNgramDict ngramDict;      //define an empty dictionary
    int countSentence=0;           //count the sentence processed
    //2. read the corpus
    ifstream corpus;
    corpus.open(inputCorpusFile,ios::in);
    string eachSentence;
    
    if (!corpus.is_open())
    {
       cerr<<"Can't open the input corpus file!\n";
       return 0; //error call
    }
    
    //3. for each corpus
    while (getline(corpus,eachSentence,'\n'))//get each sentence
    {
          countSentence++;
          //3.1 initialization
          istringstream str_eachSentence(eachSentence); //convert it to str-stream
          char tempWord[100];            //store each word (assign a space to avoid potential error)
          int numWord=0;              //store the number of words
          string sentenceArray[200]; //maximum-number of words (200) declare a sentence array
          
         
          
          //3.2 Get each word of the sentence
          while (str_eachSentence>>tempWord)
                {
                   sentenceArray[numWord]=tempWord; //Get the words
                   numWord++;                       //update the number of words
                }
           
           
              
           //3.3. get each ngram from the sentence array and update the dictionary                         } 
           for (int ngram=1; ngram<=maxNgram; ngram++)
              for (int i=0; i<=numWord-ngram; i++)
                  {
                  string ngramKey;                    //define the ngram key 
                  ngramKey="";                        //empty string key
                  for (int j=i;j<i+ngram;j++)
                     { 
                      ngramKey+=sentenceArray[j];    //Get the key
                      if (j<i+ngram-1)
                         ngramKey+=" ";
                         }
                  
                   ngramDict.insertNgram(ngramKey,ngram); //insert the ngram feature
                   }
                   
           
           if (countSentence%5000==0)
              cout<<"Have already process "<<countSentence<<" sentences.\n";                           
    }  
    cout<<"All together process "<<countSentence<<" sentences.\n";       
    
    
    //4. finally output the dictionary into a .txt file
    ngramDict.outputNgramDict(ngramDictFile,minPrune);  
    
    if (ngramDict.checkReadFileStatus()) //if read file successful/or 
        return ngramDict;
    else
        {
        cerr<<"Can't not create the dictionary.\n";
        return ngramDict;
        }
     }



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

bool smt_construct_wordDict(char* wordClassDictFile, char* inputCorpus, char* tagsCorpus)
{
     //1. Read the word Dictionary
    cout<<"Start reading the word class dictionary.\n";
    wordClassDict wordClassDictionary(wordClassDictFile);
    
    bool readSuccess=wordClassDictionary.checkReadFileStatus();
    if (readSuccess==true)
        cout<<"Read the word class dictionary successfully.\n";
    else
        {
        cerr<<"Fail to read the word class dictionary.\n";
        return 0;
        }
    
    //2. start constructing the word class corpus
    cout<<"Start constructing the word class corpus.\n";
    wordClassDictionary.createWCFile(inputCorpus,tagsCorpus);
    
    if (wordClassDictionary.checkReadFileStatus()) //if read file successful/or 
        return 1;
    else
        {
        cerr<<"Can't not create the tags corpus.\n";
        return 0;
        }
     }


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

wordClassDict smt_construct_wordDict(char* wordClassDictFile, char* inputCorpus, char* tagsCorpus, bool overloadFlag)
{
     //1. Read the word Dictionary
    cout<<"Start reading the word class dictionary.\n";
    wordClassDict wordClassDictionary(wordClassDictFile);
    
    bool readSuccess=wordClassDictionary.checkReadFileStatus();
    if (readSuccess==true)
        cout<<"Read the word class dictionary successfully.\n";
    else
        {
        cerr<<"Fail to read the word class dictionary.\n";
        return 0;
        }
    
    //2. start constructing the word class corpus
    cout<<"Start constructing the word class corpus.\n";
    wordClassDictionary.createWCFile(inputCorpus,tagsCorpus);
    
    if (wordClassDictionary.checkReadFileStatus()) //if read file successful/or 
        return wordClassDictionary;
    else
        {
        cerr<<"Can't not create the tags corpus.\n";
        return wordClassDictionary;
        }
     }


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
1.  ngramFeatures --- the ngram features in the zone (vector<int>)
******************************************************************************/

vector<int> smt_extract_ngramFeature(sentenceArray* sentence, phraseNgramDict* ngramDictionary, int zoneL, int zoneR, int flag,int maxNgramSize)
{
    //1. initialisation
    vector<int> ngramFeatures; //store the ngram features
    
    if (flag==0) //left size FR words extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(10000000*(zoneR-i-1)+featureIndex);
                    }
                }
                
        }
    else if (flag==10) //left size FR tags extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(-10000000*(zoneR-i-1)-featureIndex);
                    }
                }
        }
    else if (flag==1) //right size FR words extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(100000000+10000000*(i-zoneL)+featureIndex);
                    }
                }
        }
    else if (flag==11) //right size FR tags extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(-100000000-10000000*(i-zoneL)-featureIndex);
                    }
                }
        }
    else if (flag==2) //EN words extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(300000000+featureIndex);
                    }
                }
        }
    else if (flag==22) //EN tags extraction
    {
        for (int ngram=1;ngram<=maxNgramSize;ngram++) //for each ngram
            for (int i=zoneL; i<=zoneR-ngram; i++)    //for each feature
            {
                //2.1 get the words
                string tempWord=sentence->getPhraseFromSentence(i,i+ngram-1);
                    
                //2.2 get the feature
                int featureIndex=ngramDictionary->getNgramIndex(tempWord);
                if (featureIndex!=-1)//if the feature index exists
                {
                    ngramFeatures.push_back(400000000+featureIndex);
                    }
                }
        }
    return ngramFeatures;
    
    }


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
fout --- write an output file (ofstream& object, should pass reference)
         source phrase ||| target phrase ||| reordering distance ||| feature index
******************************************************************************/

void smt_consistPhrasePair(sentenceArray* sentenceFR, sentenceArray* sentenceEN, sentenceArray* tagFR, sentenceArray* tagEN, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, alignArray sentenceAlignment, int zoneConf[], int maxPhraseLength, int maxNgramSize, relabelFeature* featureRelabelDB, ofstream& fout)
{
     //1. Initialization
     statePhraseDict statePhr; //store the phrase pair information, [sourcePos_start,sourcePos_end,targtPos_start,targetPos_end]->reordering distance
//     int maxNgramSize=3;       //the max-length of ngram features
     int Nf=sentenceFR->getSentenceLength(); //the length of the FR sentence
     int Ne=sentenceEN->getSentenceLength(); //the length of the EN sentence
     
     /******************Part 1. Extract the consistent phrase *******************************/
     //2. for each source position
     
     for (int i=0; i<Nf; i++)
     {
         //initialisation
         int startF; //the start position of FR phrase (first word that has alignment) 
         int startE; //the start position of EN phrase
         int endE;   //the end position of EN phrase
         int coverCheck=0; //check whether all EN words are covered (no-gap allowed)
         int minF;         //left most FR word aligned to EN words between startE and endE
         int maxF;         //right most FR word aligned to EN words between startE and endE
         int dist;            //the reordering distance
         vector<int> getWordAlignmentFRtoEN; //to read the word alignment FR to EN
         vector<int> getWordAlignmentENtoFR; //to read the word alignment EN to FR
         vector<int> nonAlign_en; //a vector store the non-aligned EN words
         
         //2.1 if the first item is not null alignment
         if (sentenceAlignment.checkFRtoEN_alignment(i))
             startF=i;
         else
         //2.2 else search the first item at the closest position
             {
                 startF=i+1;
                 while (startF<Nf)
                 {
                       if (sentenceAlignment.checkFRtoEN_alignment(startF))
                           break;
                       else
                           startF++;
                       }
                 }
                 
         //2.3 if the start position is the end of the sentence
         if (startF==Nf)
            continue;
            
         
         //3. get the position of the EN phrase (startE and endE)
         getWordAlignmentFRtoEN=sentenceAlignment.getFRtoEN_alignment(startF);
         startE=getWordAlignmentFRtoEN.front(); //start position of EN
         endE=getWordAlignmentFRtoEN.back();    //end position of EN
         
         
         
         //4. check the null-aligned EN phrase, if it occur between startE and endE
         
         for (int ss=startE; ss< Ne; ss++)
         {
             if (!sentenceAlignment.checkENtoFR_alignment(ss))
                 nonAlign_en.push_back(ss);
                 
             }
         int s=0;

         
         while (s<nonAlign_en.size())
         {
               
               if (nonAlign_en[s]<=endE)
                   coverCheck++;  //for null aligned word, it can align to any word
               else
                   break;
               s++;
               }
         
         //need test: remove all non-aligned words before endE
         nonAlign_en.erase(nonAlign_en.begin(),nonAlign_en.begin()+s);
         
         
         //5. go through all words aligned to startF
         intintDict flagE; //the lattic flag, for two FR words aligned to one EN word, only count coverCheck once
         for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
         {
             flagE[getWordAlignmentFRtoEN[k]]=1; //the phrase cover this EN word
             coverCheck++;                 //update the word covered
             }
             
         //6. check the left most FR word and right most FR word aligned to EN words between startE and endE
         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(startE);
         minF=getWordAlignmentENtoFR.front(); //left most word
         maxF=getWordAlignmentENtoFR.back();  //right most word
         
         for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
         {
             //6.1 get the EN word alignment to startF
             getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(getWordAlignmentFRtoEN[k]);
             if (getWordAlignmentENtoFR.front()<minF)
                minF=getWordAlignmentENtoFR.front(); //re-locate the left most FR word
             if (getWordAlignmentENtoFR.back()>maxF)
                maxF=getWordAlignmentENtoFR.back();  //re-locate the right most FR word
             }
         
         //7. if it satisfies the condition: consistent phrase pairs
         if (coverCheck==(endE-startE+1) and maxF<=startF and minF>=i)
         {
             //7.1 get the reordering distance and store the consistent phrase
             int prevE=startE-1; //previously translated phrase
             //If it is the start of the sentence (see Figure)
             while (prevE>=0)
             {
                   if (sentenceAlignment.checkENtoFR_alignment(prevE))
                      break;
                   prevE=prevE-1;
                   }
             
             
             if (prevE<0)
                 dist=-startF; //the begining of the EN sentence
             else
             {   //else get the reordering distance
                 //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                 getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                 dist=getWordAlignmentENtoFR.back()-startF+1;
                 }
                 
             vector<int> tempInfo;
             tempInfo.push_back(i);       //start position of FR phrase
             tempInfo.push_back(startF); //end position of FR phrase
             tempInfo.push_back(startE); //start position of EN phrase
             tempInfo.push_back(endE);   //end position of EN phrase
             statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
             
             //7.2 further check null-alignment words near by and extend the phrases
             if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
             {
                 tempInfo.clear();
                 tempInfo.push_back(i);       //start position of FR phrase
                 tempInfo.push_back(startF); //end position of FR phrase
                 tempInfo.push_back(startE); //start position of EN phrase
                 tempInfo.push_back(endE+1);   //end position of EN phrase
                 statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                 }
              
              if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
             {
                 tempInfo.clear();
                 tempInfo.push_back(i);       //start position of FR phrase
                 tempInfo.push_back(startF); //end position of FR phrase
                 tempInfo.push_back(startE-1); //start position of EN phrase
                 tempInfo.push_back(endE);   //end position of EN phrase
                 statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                 }    
             
             }
             
         //8. All other phrases can't be extended
         if (minF<i)
            continue;
         
         
         //9. base on the start position of FR phrase, extract the phrase pairs that have more than 1 FR word
         for (int j=startF+1;j<Nf;j++)
         {
             if (sentenceAlignment.checkFRtoEN_alignment(j))
             {
                 //9.1 if the alignment extend the EN phrase
                 getWordAlignmentFRtoEN=sentenceAlignment.getFRtoEN_alignment(j);
                 if (getWordAlignmentFRtoEN.back()>endE)
                    endE=getWordAlignmentFRtoEN.back(); //extend right boundary
                 if (getWordAlignmentFRtoEN.front()<startE)
                 {
                     int tempE=startE;
                     startE=getWordAlignmentFRtoEN.front(); //extend left boundary
                     for (int ss=startE;ss<=tempE;ss++)
                     {
                         //check the null-alignment words
                         if (!sentenceAlignment.checkENtoFR_alignment(ss))
                            nonAlign_en.push_back(ss);
                         }
                     sort(nonAlign_en.begin(),nonAlign_en.end()); //sort the elements
                     }
                 
                 //9.2 first update the null-alignment words
                 s=0;
                 while (s<nonAlign_en.size())
                 {
                       if (nonAlign_en[s]<=endE)
                          coverCheck+=1;
                       else
                           break;
                       s++;
                       }
                 
                 nonAlign_en.erase(nonAlign_en.begin(),nonAlign_en.begin()+s);
                 
                 //9.3 if two FR words aligned to the same EN word, coverCheck only update once
                 for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
                 {
                     intintDict::const_iterator flag_found=flagE.find(getWordAlignmentFRtoEN[k]);
                     if (flag_found==flagE.end()) //not found the key
                     {
                        flagE[getWordAlignmentFRtoEN[k]]=1;
                        coverCheck+=1;
                        }
                     }
                   
                 //9.4 check the left most word and the right most word  
                 for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
                 {
                     //get the EN word alignment to j
                     getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(getWordAlignmentFRtoEN[k]);
                     if (getWordAlignmentENtoFR.front()<minF)
                         minF=getWordAlignmentENtoFR.front(); //re-locate the left most FR word
                     if (getWordAlignmentENtoFR.back()>maxF)
                         maxF=getWordAlignmentENtoFR.back();  //re-locate the right most FR word
                     }
                     
                 //9.5 if the phrase is consistent
                 
                 if ((j-i+1)<=maxPhraseLength and (endE-startE+1)<=maxPhraseLength and coverCheck==(endE-startE+1) and maxF<=j and minF>=i)
                 {
                     //get the reordering distance and store the consistent phrase
                     int prevE=startE-1;
                     
                     while(prevE>=0)
                     {
                         if (sentenceAlignment.checkENtoFR_alignment(prevE))
                            break;
                         prevE=prevE-1;
                         }
                         
                     if (prevE<0)
                        dist=-startF; //the begining of the EN sentence
                     else
                      {   //else get the reordering distance
                         //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                         dist=getWordAlignmentENtoFR.back()-startF+1;
                         }
                     //get the consistent phrase pair
                     vector<int> tempInfo;
                     tempInfo.push_back(i);       //start position of FR phrase
                     tempInfo.push_back(j); //end position of FR phrase
                     tempInfo.push_back(startE); //start position of EN phrase
                     tempInfo.push_back(endE);   //end position of EN phrase
                     statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                     
                     //further check null-alignment words near by and extend the phrases
                     if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j);     //end position of FR phrase
                         tempInfo.push_back(startE); //start position of EN phrase
                         tempInfo.push_back(endE+1);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }
              
                     if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j); //end position of FR phrase
                         tempInfo.push_back(startE-1); //start position of EN phrase
                         tempInfo.push_back(endE);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }    
                     }
                    
                 }
             else //if the j-th FR word has null-alignment (on the right-most of the phrase)
                  //then can extend it directly
             {
                 if ((j-i+1)<=maxPhraseLength and (endE-startE+1)<=maxPhraseLength and coverCheck==(endE-startE+1) and maxF<=j and minF>=i)
                 {
                     //get the reordering distance and store the consistent phrase
                     int prevE=startE-1;
                     
                     while(prevE>=0)
                     {
                         if (sentenceAlignment.checkENtoFR_alignment(prevE))
                            break;
                         prevE=prevE-1;
                         }
                         
                     if (prevE<0)
                        dist=-startF; //the begining of the EN sentence
                     else
                      {   //else get the reordering distance
                         //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                         dist=getWordAlignmentENtoFR.back()-startF+1;
                         }
                     //get the consistent phrase pair
                     vector<int> tempInfo;
                     tempInfo.push_back(i);       //start position of FR phrase
                     tempInfo.push_back(j); //end position of FR phrase
                     tempInfo.push_back(startE); //start position of EN phrase
                     tempInfo.push_back(endE);   //end position of EN phrase
                     statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                     
                     //further check null-alignment words near by and extend the phrases
                     if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j);     //end position of FR phrase
                         tempInfo.push_back(startE); //start position of EN phrase
                         tempInfo.push_back(endE+1);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }
              
                     if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j); //end position of FR phrase
                         tempInfo.push_back(startE-1); //start position of EN phrase
                         tempInfo.push_back(endE);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }    
                     } 
                  }
             if (minF<i) //early stop
                break;
             
             }
         }
     /******************Part 1. Extract the consistent phrase (end of part 1)*******************************/
     
     
     /******************Part 2. Extract the ngram features for each phrase pair ****************************/
     for (statePhraseDict::const_iterator phraseLoop=statePhr.begin();phraseLoop!=statePhr.end();phraseLoop++)
     {
         //1. Get the phrase and the reordering distance
         string tempFRPhrase=sentenceFR->getPhraseFromSentence(phraseLoop->first[0],phraseLoop->first[1]);
         string tempENPhrase=sentenceEN->getPhraseFromSentence(phraseLoop->first[2],phraseLoop->first[3]);
         
         
         //2. Output the phrase pairs and the reordering distance
         fout<<tempFRPhrase<<" ||| "<<tempENPhrase<<" ||| "<<phraseLoop->second<<" ||| ";
         
         //3. Get the ngram features
         vector<int> featureBlock;
         
         
         
         //3.1 target word features
         vector<int> nGram_en=smt_extract_ngramFeature(sentenceEN, ngramDictEN, phraseLoop->first[2], phraseLoop->first[3]+1, 2, maxNgramSize);
         featureBlock.insert(featureBlock.end(),nGram_en.begin(),nGram_en.end());
         
         
         
         //3.2 target tags features
         vector<int> nTag_en=smt_extract_ngramFeature(tagEN, tagsDictEN, phraseLoop->first[2], phraseLoop->first[3]+1, 22, maxNgramSize);
         featureBlock.insert(featureBlock.end(),nTag_en.begin(),nTag_en.end());
         
         
         
         //3.3 left side source word fetures
         int zoneL=max(0,phraseLoop->first[0]-zoneConf[0]);//the left boundary
         int zoneR=phraseLoop->first[0];                   //the right boundary
         //cout<<zoneL<<'\t'<<zoneR<<'\n';
         vector<int> nGram_fr_l=smt_extract_ngramFeature(sentenceFR, ngramDictFR, zoneL, zoneR, 0, maxNgramSize);
         featureBlock.insert(featureBlock.end(),nGram_fr_l.begin(),nGram_fr_l.end());
        
         //3.3 left side source tags features
         vector<int> nTag_fr_l=smt_extract_ngramFeature(tagFR,tagsDictFR,zoneL,zoneR,10, maxNgramSize);
         featureBlock.insert(featureBlock.end(),nTag_fr_l.begin(),nTag_fr_l.end());
       /*  
         if (phraseLoop->first[0]==0 and phraseLoop->first[1]==3)
         {
            cout<<nGram_en.size()<<'\n';
            cout<<nTag_en.size()<<'\n';
            cout<<zoneL<<'\t'<<zoneR<<'\n';
            
                          }
         */
         //3.4 right side source word features
         zoneL=phraseLoop->first[1]+1;
         zoneR=min(Nf,zoneL+zoneConf[1]); //need tested
         vector<int> nGram_fr_r=smt_extract_ngramFeature(sentenceFR,ngramDictFR,zoneL,zoneR,1,maxNgramSize);
         featureBlock.insert(featureBlock.end(),nGram_fr_r.begin(),nGram_fr_r.end());
         
         //3.5 right side source tag features
         vector<int> nTag_fr_r=smt_extract_ngramFeature(tagFR,tagsDictFR,zoneL,zoneR,11,maxNgramSize);
         featureBlock.insert(featureBlock.end(),nTag_fr_r.begin(),nTag_fr_r.end());
         
         //4. relabel the features
         for (int k=0; k<featureBlock.size();k++)
         {
             int featureRelabel=featureRelabelDB->getRelabeledFeature(featureBlock[k]);
             if (featureRelabel==-100) //if the feature doesn't exist
                 featureRelabel=featureRelabelDB->insertFeature(featureBlock[k]);
             //get the relabeled feature index
             featureBlock[k]=featureRelabel;
             }
             
         //5. output the relabeled features
         for (int k=0; k<featureBlock.size(); k++)
         {
             fout<<featureBlock[k]<<" ";
             }
         fout<<'\n'; //ending process of this phrase pair
         }
         
     /***************Part 2. Extract the ngram features for each phrase pair (end)**************************/
     }





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

void smt_consistPhrasePair(sentenceArray* sentenceFR, sentenceArray* sentenceEN, sentenceArray* tagFR, sentenceArray* tagEN, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, alignArray sentenceAlignment, int zoneConf[], int maxPhraseLength, int maxNgramSize, relabelFeature* featureRelabelDB, ofstream& fout, corpusPhraseDB* testPhraseDB)
{
     //1. Initialization
     statePhraseDict statePhr; //store the phrase pair information, [sourcePos_start,sourcePos_end,targtPos_start,targetPos_end]->reordering distance
//     int maxNgramSize=3;       //the max-length of ngram features
     int Nf=sentenceFR->getSentenceLength(); //the length of FR sentence
     int Ne=sentenceEN->getSentenceLength(); //the length of EN sentence
     
    
     /******************Part 1. Extract the consistent phrase *******************************/
     //2. for each source position
     for (int i=0; i<Nf; i++)
     {
         //initialisation
         int startF; //the start position of FR phrase (first word that has alignment) 
         int startE; //the start position of EN phrase
         int endE;   //the end position of EN phrase
         int coverCheck=0; //check whether all EN words are covered (no-gap allowed)
         int minF;         //left most FR word aligned to EN words between startE and endE
         int maxF;         //right most FR word aligned to EN words between startE and endE
         int dist;            //the reordering distance
         vector<int> getWordAlignmentFRtoEN; //to read the word alignment FR to EN
         vector<int> getWordAlignmentENtoFR; //to read the word alignment EN to FR
         vector<int> nonAlign_en; //a vector store the non-aligned EN words
         
         //2.1 if the first item is not null alignment
         if (sentenceAlignment.checkFRtoEN_alignment(i))
             startF=i;
         else
         //2.2 else search the first item at the closest position
             {
                 startF=i+1;
                 while (startF<Nf)
                 {
                       if (sentenceAlignment.checkFRtoEN_alignment(startF))
                           break;
                       else
                           startF++;
                       }
                 }
                 
         //2.3 if the start position is the end of the sentence
         if (startF==Nf)
            continue;
            
         
         //3. get the position of the EN phrase (startE and endE)
         getWordAlignmentFRtoEN=sentenceAlignment.getFRtoEN_alignment(startF);
         startE=getWordAlignmentFRtoEN.front(); //start position of EN
         endE=getWordAlignmentFRtoEN.back();    //end position of EN
         
         //4. check the null-aligned EN phrase, if it occur between startE and endE
         
         for (int ss=startE; ss< Ne; ss++)
         {
             if (!sentenceAlignment.checkENtoFR_alignment(ss))
                nonAlign_en.push_back(ss);
             }
         int s=0;
         while (s<nonAlign_en.size())
         {
               if (nonAlign_en[s]<=endE)
                   coverCheck++;  //for null aligned word, it can align to any word
               else
                   break;
               s++;
               }
         //need test: remove all non-aligned words before endE
         nonAlign_en.erase(nonAlign_en.begin(),nonAlign_en.begin()+s);
         
         //5. go through all words aligned to startF
         intintDict flagE; //the lattic flag, for two FR words aligned to one EN word, only count coverCheck once
         for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
         {
             flagE[getWordAlignmentFRtoEN[k]]=1; //the phrase cover this EN word
             coverCheck++;                 //update the word covered
             }
             
         //6. check the left most FR word and right most FR word aligned to EN words between startE and endE
         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(startE);
         minF=getWordAlignmentENtoFR.front(); //left most word
         maxF=getWordAlignmentENtoFR.back();  //right most word
         
         for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
         {
             //6.1 get the EN word alignment to startF
             getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(getWordAlignmentFRtoEN[k]);
             if (getWordAlignmentENtoFR.front()<minF)
                minF=getWordAlignmentENtoFR.front(); //re-locate the left most FR word
             if (getWordAlignmentENtoFR.back()>maxF)
                maxF=getWordAlignmentENtoFR.back();  //re-locate the right most FR word
             }
         
         //7. if it satisfies the condition: consistent phrase pairs
         if (coverCheck==(endE-startE+1) and maxF<=startF and minF>=i)
         {
             //7.1 get the reordering distance and store the consistent phrase
             int prevE=startE-1; //previously translated phrase
             //If it is the start of the sentence (see Figure)
             while (prevE>=0)
             {
                   if (sentenceAlignment.checkENtoFR_alignment(prevE))
                      break;
                   prevE=prevE-1;
                   }
             
             
             if (prevE<0)
                 dist=-startF; //the begining of the EN sentence
             else
             {   //else get the reordering distance
                 //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                 getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                 dist=getWordAlignmentENtoFR.back()-startF+1;
                 }
                 
             vector<int> tempInfo;
             tempInfo.push_back(i);       //start position of FR phrase
             tempInfo.push_back(startF); //end position of FR phrase
             tempInfo.push_back(startE); //start position of EN phrase
             tempInfo.push_back(endE);   //end position of EN phrase
             statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
             
             //7.2 further check null-alignment words near by and extend the phrases
             if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
             {
                 tempInfo.clear();
                 tempInfo.push_back(i);       //start position of FR phrase
                 tempInfo.push_back(startF); //end position of FR phrase
                 tempInfo.push_back(startE); //start position of EN phrase
                 tempInfo.push_back(endE+1);   //end position of EN phrase
                 statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                 }
              
              if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
             {
                 tempInfo.clear();
                 tempInfo.push_back(i);       //start position of FR phrase
                 tempInfo.push_back(startF); //end position of FR phrase
                 tempInfo.push_back(startE-1); //start position of EN phrase
                 tempInfo.push_back(endE);   //end position of EN phrase
                 statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                 }    
             
             }
             
         //8. All other phrases can't be extended
         if (minF<i)
            continue;
         
         
         //9. base on the start position of FR phrase, extract the phrase pairs that have more than 1 FR word
         for (int j=startF+1;j<Nf;j++)
         {
             if (sentenceAlignment.checkFRtoEN_alignment(j))
             {
                 //9.1 if the alignment extend the EN phrase
                 getWordAlignmentFRtoEN=sentenceAlignment.getFRtoEN_alignment(j);
                 if (getWordAlignmentFRtoEN.back()>endE)
                    endE=getWordAlignmentFRtoEN.back(); //extend right boundary
                 if (getWordAlignmentFRtoEN.front()<startE)
                 {
                     int tempE=startE;
                     startE=getWordAlignmentFRtoEN.front(); //extend left boundary
                     for (int ss=startE;ss<=tempE;ss++)
                     {
                         //check the null-alignment words
                         if (!sentenceAlignment.checkENtoFR_alignment(ss))
                            nonAlign_en.push_back(ss);
                         }
                     sort(nonAlign_en.begin(),nonAlign_en.end()); //sort the elements
                     }
                 
                 //9.2 first update the null-alignment words
                 s=0;
                 while (s<nonAlign_en.size())
                 {
                       if (nonAlign_en[s]<=endE)
                          coverCheck+=1;
                       else
                           break;
                       s++;
                       }
                 
                 nonAlign_en.erase(nonAlign_en.begin(),nonAlign_en.begin()+s);
                 
                 //9.3 if two FR words aligned to the same EN word, coverCheck only update once
                 for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
                 {
                     intintDict::const_iterator flag_found=flagE.find(getWordAlignmentFRtoEN[k]);
                     if (flag_found==flagE.end()) //not found the key
                        {
                            flagE[getWordAlignmentFRtoEN[k]]=1;
                            coverCheck+=1;
                        }
                     }
                   
                 //9.4 check the left most word and the right most word  
                 for (int k=0;k<getWordAlignmentFRtoEN.size();k++)
                 {
                     //get the EN word alignment to j
                     getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(getWordAlignmentFRtoEN[k]);
                     if (getWordAlignmentENtoFR.front()<minF)
                         minF=getWordAlignmentENtoFR.front(); //re-locate the left most FR word
                     if (getWordAlignmentENtoFR.back()>maxF)
                         maxF=getWordAlignmentENtoFR.back();  //re-locate the right most FR word
                     }
                     
                 //9.5 if the phrase is consistent
                 
                 if ((j-i+1)<=maxPhraseLength and (endE-startE+1)<=maxPhraseLength and coverCheck==(endE-startE+1) and maxF<=j and minF>=i)
                 {
                     //get the reordering distance and store the consistent phrase
                     int prevE=startE-1;
                     
                     while(prevE>=0)
                     {
                         if (sentenceAlignment.checkENtoFR_alignment(prevE))
                            break;
                         prevE=prevE-1;
                         }
                         
                     if (prevE<0)
                        dist=-startF; //the begining of the EN sentence
                     else
                      {   //else get the reordering distance
                         //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                         dist=getWordAlignmentENtoFR.back()-startF+1;
                         }
                     //get the consistent phrase pair
                     vector<int> tempInfo;
                     tempInfo.push_back(i);       //start position of FR phrase
                     tempInfo.push_back(j); //end position of FR phrase
                     tempInfo.push_back(startE); //start position of EN phrase
                     tempInfo.push_back(endE);   //end position of EN phrase
                     statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                     
                     //further check null-alignment words near by and extend the phrases
                     if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j);     //end position of FR phrase
                         tempInfo.push_back(startE); //start position of EN phrase
                         tempInfo.push_back(endE+1);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }
              
                     if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j); //end position of FR phrase
                         tempInfo.push_back(startE-1); //start position of EN phrase
                         tempInfo.push_back(endE);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }    
                     }
                    
                 }
             else //if the j-th FR word has null-alignment (on the right-most of the phrase)
                  //then can extend it directly
             {
                 if ((j-i+1)<=maxPhraseLength and (endE-startE+1)<=maxPhraseLength and coverCheck==(endE-startE+1) and maxF<=j and minF>=i)
                 {
                     //get the reordering distance and store the consistent phrase
                     int prevE=startE-1;
                     
                     while(prevE>=0)
                     {
                         if (sentenceAlignment.checkENtoFR_alignment(prevE))
                            break;
                         prevE=prevE-1;
                         }
                         
                     if (prevE<0)
                        dist=-startF; //the begining of the EN sentence
                     else
                      {   //else get the reordering distance
                         //d=last source word position of previously translated phrase+1-first source word position of newly translated phrase
                         getWordAlignmentENtoFR=sentenceAlignment.getENtoFR_alignment(prevE);
                         dist=getWordAlignmentENtoFR.back()-startF+1;
                         }
                     //get the consistent phrase pair
                     vector<int> tempInfo;
                     tempInfo.push_back(i);       //start position of FR phrase
                     tempInfo.push_back(j); //end position of FR phrase
                     tempInfo.push_back(startE); //start position of EN phrase
                     tempInfo.push_back(endE);   //end position of EN phrase
                     statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                     
                     //further check null-alignment words near by and extend the phrases
                     if (!sentenceAlignment.checkENtoFR_alignment(endE+1) and (endE+1)<Ne)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j);     //end position of FR phrase
                         tempInfo.push_back(startE); //start position of EN phrase
                         tempInfo.push_back(endE+1);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }
              
                     if (!sentenceAlignment.checkENtoFR_alignment(startE-1) and (startE-1)>=0)
                     {
                         tempInfo.clear();
                         tempInfo.push_back(i);       //start position of FR phrase
                         tempInfo.push_back(j); //end position of FR phrase
                         tempInfo.push_back(startE-1); //start position of EN phrase
                         tempInfo.push_back(endE);   //end position of EN phrase
                         statePhr[tempInfo]=dist;    //add the consistent phrase to the dictionary (no feature yet)
                         }    
                     } 
                  }
             if (minF<i) //early stop
                break;
             
             }
         
         }
     /******************Part 1. Extract the consistent phrase (end of part 1)*******************************/
     
     
     /******************Part 2. Extract the ngram features for each phrase pair ****************************/
     for (statePhraseDict::const_iterator phraseLoop=statePhr.begin();phraseLoop!=statePhr.end();phraseLoop++)
     {
         //1. Get the phrase and the reordering distance
         string tempFRPhrase=sentenceFR->getPhraseFromSentence(phraseLoop->first[0],phraseLoop->first[1]);
         
         string tempENPhrase=sentenceEN->getPhraseFromSentence(phraseLoop->first[2],phraseLoop->first[3]);
         
         //If the key exist, then write down the phrase
         if (testPhraseDB->checkPhraseDB(tempFRPhrase))
         {
            //2. Output the phrase pairs and the reordering distance
            fout<<tempFRPhrase<<" ||| "<<tempENPhrase<<" ||| "<<phraseLoop->second<<" ||| ";
         
            //3. Get the ngram features
            vector<int> featureBlock;
         
            //3.1 target word features
            vector<int> nGram_en=smt_extract_ngramFeature(sentenceEN, ngramDictEN, phraseLoop->first[2], phraseLoop->first[3]+1, 2, maxNgramSize);
            featureBlock.insert(featureBlock.end(),nGram_en.begin(),nGram_en.end());
         
             //3.2 target tags features
             vector<int> nTag_en=smt_extract_ngramFeature(tagEN, tagsDictEN, phraseLoop->first[2], phraseLoop->first[3]+1, 22, maxNgramSize);
             featureBlock.insert(featureBlock.end(),nTag_en.begin(),nTag_en.end());
         
             //3.3 left side source word fetures
             int zoneL=max(0,phraseLoop->first[0]-zoneConf[0]);//the left boundary
             int zoneR=phraseLoop->first[0];                   //the right boundary
             vector<int> nGram_fr_l=smt_extract_ngramFeature(sentenceFR, ngramDictFR, zoneL, zoneR, 0, maxNgramSize);
             featureBlock.insert(featureBlock.end(),nGram_fr_l.begin(),nGram_fr_l.end());
         
             //3.3 left side source tags features
             vector<int> nTag_fr_l=smt_extract_ngramFeature(tagFR,tagsDictFR,zoneL,zoneR,10, maxNgramSize);
             featureBlock.insert(featureBlock.end(),nTag_fr_l.begin(),nTag_fr_l.end());
         
             //3.4 right side source word features
             zoneL=phraseLoop->first[1]+1;
             zoneR=min(Nf,zoneL+zoneConf[1]); //need tested
             vector<int> nGram_fr_r=smt_extract_ngramFeature(sentenceFR,ngramDictFR,zoneL,zoneR,1,maxNgramSize);
             featureBlock.insert(featureBlock.end(),nGram_fr_r.begin(),nGram_fr_r.end());
         
             //3.5 right side source tag features
             vector<int> nTag_fr_r=smt_extract_ngramFeature(tagFR,tagsDictFR,zoneL,zoneR,11,maxNgramSize);
             featureBlock.insert(featureBlock.end(),nTag_fr_r.begin(),nTag_fr_r.end());
         
             //4. relabel the features
             for (int k=0; k<featureBlock.size();k++)
             {
                 int featureRelabel=featureRelabelDB->getRelabeledFeature(featureBlock[k]);
                 if (featureRelabel==-100) //if the feature doesn't exist
                    featureRelabel=featureRelabelDB->insertFeature(featureBlock[k]);
                 //get the relabeled feature index
                 featureBlock[k]=featureRelabel;
                 }
             
             //5. output the relabeled features
             for (int k=0; k<featureBlock.size(); k++)
             {
                 fout<<featureBlock[k]<<" ";
                 }
             fout<<'\n'; //ending process of this phrase pair
             }
         }
         
     /***************Part 2. Extract the ngram features for each phrase pair (end)**************************/
     }





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
void smt_constructPhraseReorderingDB(char* sourceCorpusFile, char* targetCorpusFile, char* wordAlignmentFile, char* tagsSourceFile, char* tagsTargetFile, char* phraseDBFile, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, int zoneConf[], int maxPhraseLength, int maxNgramSize, char* featureRelabelDBFile)
{
     //1. initialisation
     relabelFeature* featureRelabelDB = new relabelFeature(); //store the relabel of the ngram features
     ifstream sourceFile(sourceCorpusFile,ios::in);           //open the source word file
     ifstream targetFile(targetCorpusFile,ios::in);           //open the target word file
     ifstream sourceTag(tagsSourceFile,ios::in);              //open the source tags file
     ifstream targetTag(tagsTargetFile,ios::in);              //open the target tags file
     ifstream sentenceAlignmentFile(wordAlignmentFile,ios::in);//open the word alignment file
     ofstream foutPhrasePair(phraseDBFile,ios::out);           //create the phrase DB file
     
     string sourceSentence;                                    //store source sentence
     string targetSentence;                                     //store target sentence
     string sourceTagSentence;                                 //store source tags
     string targetTagSentence;                                 //store target tags
     string eachSentenceAlignment;                             //store word alignments
     
     //1*. Check whether the corpora is open or not
     if (!sourceFile.is_open())
         {cerr<<"Error: Can't open the source corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!targetFile.is_open())
         {cerr<<"Error: Can't open the target corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!sourceTag.is_open())
         {cerr<<"Error: Can't open the source tags corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!targetTag.is_open())
         {cerr<<"Error: Can't open the target tags corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!sentenceAlignmentFile.is_open())
         {cerr<<"Error: Can't open the sentence alignment files in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     
     
     //2. For each sentence
     int countSen=0;
     
     while (getline(sourceFile,sourceSentence,'\n'))
     {
           countSen++;
           //2.1 Get the source/target (word/tag) and word alignemnts strings
           getline(targetFile,targetSentence,'\n');
           getline(sourceTag,sourceTagSentence,'\n');
           getline(targetTag,targetTagSentence,'\n');
           getline(sentenceAlignmentFile,eachSentenceAlignment,'\n');
           
           //2.2 Create the word alignment class
           alignArray sentenceAlignment(eachSentenceAlignment);
           
           //2.3 create string array pointer to the source word/tag string
           sentenceArray* sentenceFR=new sentenceArray(sourceSentence);
           sentenceArray* tagFR=new sentenceArray(sourceTagSentence);
                 
           //2.4 create string array pointer to the target word/tag string
           sentenceArray* sentenceEN=new sentenceArray(targetSentence);
           sentenceArray* tagEN=new sentenceArray(targetTagSentence);
           
           
           
           //2.5 using smt_consistPhrasePair to output the phrase pairs
           smt_consistPhrasePair(sentenceFR, sentenceEN, tagFR, tagEN, ngramDictFR, ngramDictEN, tagsDictFR, tagsDictEN, sentenceAlignment, zoneConf, maxPhraseLength, maxNgramSize, featureRelabelDB, foutPhrasePair);
          
          
           delete sentenceFR;
           delete tagFR;
           delete sentenceEN;
           delete tagEN;
           if (countSen%10000==0)
              cout<<"Have processed "<<countSen<<" sentences.\n";
           }
     
     cout<<"All together processed "<<countSen<<" sentences.\n";      
     sourceFile.close();
     targetFile.close();
     sourceTag.close();
     targetTag.close();
     sentenceAlignmentFile.close();
     foutPhrasePair.close();
     
     //3. Output the relabel feature database
     featureRelabelDB->writeRelabelFeatures(featureRelabelDBFile);
     
     //4. release memory
     delete featureRelabelDB;
     
     }
     


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
void smt_constructPhraseReorderingDB(char* sourceCorpusFile, char* targetCorpusFile, char* wordAlignmentFile, char* tagsSourceFile, char* tagsTargetFile, char* phraseDBFile, phraseNgramDict* ngramDictFR, phraseNgramDict* ngramDictEN, phraseNgramDict* tagsDictFR, phraseNgramDict* tagsDictEN, int zoneConf[], int maxPhraseLength, int maxNgramSize, char* featureRelabelDBFile, char* testFileName)
{
     //1. initialisation
     relabelFeature* featureRelabelDB = new relabelFeature(); //store the relabel of the ngram features
     ifstream sourceFile(sourceCorpusFile,ios::in);           //open the source word file
     ifstream targetFile(targetCorpusFile,ios::in);           //open the target word file
     ifstream sourceTag(tagsSourceFile,ios::in);              //open the source tags file
     ifstream targetTag(tagsTargetFile,ios::in);              //open the target tags file
     ifstream sentenceAlignmentFile(wordAlignmentFile,ios::in);//open the word alignment file
     ofstream foutPhrasePair(phraseDBFile,ios::out);           //create the phrase DB file
     
     string sourceSentence;                                    //store source sentence
     string targetSentence;                                     //store target sentence
     string sourceTagSentence;                                 //store source tags
     string targetTagSentence;                                 //store target tags
     string eachSentenceAlignment;                             //store word alignments
     
     //1*. Check whether the corpora is open or not
     if (!sourceFile.is_open())
         {cerr<<"Error: Can't open the source corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!targetFile.is_open())
         {cerr<<"Error: Can't open the target corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!sourceTag.is_open())
         {cerr<<"Error: Can't open the source tags corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!targetTag.is_open())
         {cerr<<"Error: Can't open the target tags corpus in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
     if (!sentenceAlignmentFile.is_open())
         {cerr<<"Error: Can't open the sentence alignment files in function: smt_constructPhraseReorderingDB!\n";
         exit (1);}
         
     //1*. Read the test file and create the phrase database
     corpusPhraseDB* testPhraseDB = new corpusPhraseDB(testFileName,maxPhraseLength);
     if (testPhraseDB->getNumPhrase()==0)
     {
         cerr<<"Error: There is no phrase extracted from the test corpus!\n";
         exit(1);
         }
     
     //2. For each sentence
     int countSen=0;
     while (getline(sourceFile,sourceSentence,'\n'))
     {
           
           countSen++;
           //2.1 Get the source/target (word/tag) and word alignemnts strings
           getline(targetFile,targetSentence,'\n');
           getline(sourceTag,sourceTagSentence,'\n');
           getline(targetTag,targetTagSentence,'\n');
           getline(sentenceAlignmentFile,eachSentenceAlignment,'\n');
           
           //2.2 Create the word alignment class
           alignArray sentenceAlignment(eachSentenceAlignment);
           
           //2.3 create string array pointer to the source word/tag string
           sentenceArray* sentenceFR=new sentenceArray(sourceSentence);
           sentenceArray* tagFR=new sentenceArray(sourceTagSentence);
                 
           //2.4 create string array pointer to the target word/tag string
           sentenceArray* sentenceEN=new sentenceArray(targetSentence);
           sentenceArray* tagEN=new sentenceArray(targetTagSentence);
           
           
           
           //2.5 using smt_consistPhrasePair to output the phrase pairs
           smt_consistPhrasePair(sentenceFR, sentenceEN, tagFR, tagEN, ngramDictFR, ngramDictEN, tagsDictFR, tagsDictEN, sentenceAlignment, zoneConf, maxPhraseLength, maxNgramSize, featureRelabelDB, foutPhrasePair, testPhraseDB);
           
           delete sentenceFR;
           delete tagFR;
           delete sentenceEN;
           delete tagEN;
           
           if (countSen%10000==0)
              cout<<"Have processed "<<countSen<<" sentences.\n";
           }
     
     cout<<"All together processed "<<countSen<<" sentences.\n";      
     sourceFile.close();
     targetFile.close();
     sourceTag.close();
     targetTag.close();
     sentenceAlignmentFile.close();
     
     //3. Output the relabel feature database
     featureRelabelDB->writeRelabelFeatures(featureRelabelDBFile);
     
     foutPhrasePair.close();
     
     //4. release the memory
     delete featureRelabelDB;
     delete testPhraseDB;
     
     }
     

    
