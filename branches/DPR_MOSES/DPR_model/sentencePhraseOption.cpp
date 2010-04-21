/*
**********************************************************
Cpp file ---------- sentencePhraseOption.cpp
Declaration of class sentencePhraseOption
Store the phrase options (including target phrases and reordering probilities) for each sentence

Components:
1. phraseOption --- sentence phrase option. sentence index -> [position_left,position_right]->target phrase -> [reordering probabilities]
2. numSen --- the number of senences
              
Functions:
1. void createPhraseOption(int sentenceIndex, int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
1*. void createPhraseOption(int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
    Remark(*): only for one sentence so no need to store the sentenceIndex
2. vector<float> getPhraseProbs(int sentenceIndex, int phrase_boundary[], string targetPhrase, int numClass) --- get the phrase reordering probability
3. void outputPhraseOption(char* outputFileName) --- output all phrase options to a .txt file
3* void outputPhraseOption(ofstream& outputFile, int sentenceIndex, sentenceArray* sentence, phraseTranslationTable* trainingPhraseTable) --- output a sentence's phrase options to a .txt file
3* void outputPhraseOption(ofstream& outputFile) --- output all phrase options to a .txt ifle (only for a test sentence) 
4. int getNumSentence() --- get the number of sentences
                            
Special function:
1. sentencePhraseOption() --- constructor, create a phraseOption list
2. sentencePhraseOption(char* inputFileName) --- get the phrase options from a .txt file
***********************************************************
*/


#include "sentencePhraseOption.h"

/*
1. constructor
*/

sentencePhraseOption::sentencePhraseOption()
{
    numSen=0;
    }
    




/*
2. void createPhraseOption(int sentenceIndex, int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
*/

void sentencePhraseOption::createPhraseOption(int sentenceIndex, unsigned short phrase_boundary[], mapTargetProbOption targetProbs)
{
     //1. Whether the sentence is new sentence or not
     mapSentenceOption::iterator sentenceFound=phraseOption.find(sentenceIndex);
     
     //2. If the sentence option doesn't exist
     if (sentenceFound==phraseOption.end())
     {
         numSen++; //update the number of sentence
         vector<unsigned short> boundary;
         mapPhraseOption tempPhraseOption;       //store the phrase options
         
         //2.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //2.2 update the probability information
         tempPhraseOption[boundary]=targetProbs; //get the target probabilities
         phraseOption[sentenceIndex]=tempPhraseOption;
         }
     //3. If the sentence option exists
     else
     {
         vector<unsigned short> boundary;
         //3.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //3.2 update the probability information
         sentenceFound->second[boundary]=targetProbs; //get the target probabilities
         }
     }
     
/*
2*. void createPhraseOption(int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
*/

void sentencePhraseOption::createPhraseOption(unsigned short phrase_boundary[], mapTargetProbOption targetProbs)
{
     //1. Whether the sentence is new sentence or not
     int sentenceIndex=0;
     mapSentenceOption::iterator sentenceFound=phraseOption.find(sentenceIndex);
     
     //2. If the sentence option doesn't exist
     if (sentenceFound==phraseOption.end())
     {
         numSen++; //update the number of sentence
         vector<unsigned short> boundary;
         mapPhraseOption tempPhraseOption;       //store the phrase options
         
         //2.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //2.2 update the probability information
         tempPhraseOption[boundary]=targetProbs; //get the target probabilities
         phraseOption[sentenceIndex]=tempPhraseOption;
         }
     //3. If the sentence option exists
     else
     {
         vector<unsigned short> boundary;
         //3.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //3.2 update the probability information
         sentenceFound->second[boundary]=targetProbs; //get the target probabilities
         }
     }     



/*
4*  void outputPhraseOption(ofstream& outputFile, int sentenceIndex, sentenceArray* sentence, phraseTranslationTable* trainingPhraseTable)
*/
void sentencePhraseOption::outputPhraseOption(ofstream& outFile, int sentenceIndex, sentenceArray* sentence, phraseTranslationTable* trainingPhraseTable)
{
     //int numSource=0;
     //int numTarget=0;
     //vector<int> returnNum;
     mapSentenceOption::const_iterator  sentenceFound = phraseOption.find(sentenceIndex);
     if (sentenceFound==phraseOption.end())
     {
         cerr<<"Error in sentencePhraseOption.cpp: no sentence found for the sentence index: "<<sentenceIndex<<".\n";
         outFile<<"0 0 ::: -1 ||| 0.2 0.2 0.2 0.2 0.2 ;;; \n";
         //exit(1);
         }
     else
     {
         for (mapPhraseOption::const_iterator boundaryFound=sentenceFound->second.begin(); boundaryFound!=sentenceFound->second.end(); boundaryFound++)
         {
             
             //numSource++;
             //1. Output the boundary
             outFile<<boundaryFound->first[0]<<" "<<boundaryFound->first[1]<<" ::: ";
             int countTargetItem=0;
             int stackSize=boundaryFound->second.size();
             //1.1 Get the source phrases
             string sourcePhrase = sentence->getPhraseFromSentence((int) boundaryFound->first[0], (int) boundaryFound->first[1]);
             //1.2 get the target translations
             vector<string> targetTranslations = trainingPhraseTable->getTargetTranslation(sourcePhrase);
             
             for (mapTargetProbOption::const_iterator targetFound=boundaryFound->second.begin();targetFound!=boundaryFound->second.end();targetFound++)
             {
                 //numTarget++;
                 //2. Output the target phrase
                 outFile<<targetTranslations[targetFound->first]<<" ||| ";
                 
                 //3. output the probability values
                 for (int i=0; i< targetFound->second.size(); i++)
                 {
                     outFile<<targetFound->second[i]<<" ";
                     }
                 
                 countTargetItem++;
                 if (countTargetItem<stackSize)
                     outFile<<"||| ";
                 else
                     outFile<<";;; ";
                 }
             }
             
         outFile<<'\n';
         }
         
     //returnNum.push_back(numSource);
     //returnNum.push_back(numTarget);
     //return returnNum;
     }




/*
5. int getNumSentence() --- get the number of sentences
*/
int sentencePhraseOption::getNumSentence()
{return numSen;}


/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/
/*
1. constructor
*/

sentencePhraseOptionSTR::sentencePhraseOptionSTR()
{
    numSen=0;
    }


sentencePhraseOptionSTR::sentencePhraseOptionSTR(char* inputFileName)
{
    //1. initialisation
    ifstream inputFile(inputFileName,ios::binary);
    numSen=0;
    //*. The structure: each line is a sentence
    //boundary ::: target phrase ||| reordering probs;;;
    string eachSentence;
    
    if (inputFile.is_open())
    {
        //2. For each sentence
        while (getline(inputFile,eachSentence))
        {
              mapPhraseOptionSTR tempPhraseOption;       //store the phrase options
              size_t boundaryFound = eachSentence.find(" ::: "); //find the separationg between the boundary and the values
              size_t boundaryFound_end;                          //find the end of the boundary 
              int countBoundaryOption=0;
              while (boundaryFound!=string::npos)
              {
                    //2.1 Get the boundary (create a phraseOption map)
                    vector<unsigned short> boundary;        //store the boundary
                    unsigned short boundary_int;
                    string tempString;                      //store the boundary
                    if (countBoundaryOption==0)                       
                       tempString=eachSentence.substr(0,boundaryFound); //get the boundary string
                    else
                        tempString=eachSentence.substr(boundaryFound_end+5,boundaryFound-boundaryFound_end-5);
                        
                    istringstream boundaryString(tempString);
                    while (boundaryString>>boundary_int)
                          boundary.push_back(boundary_int);
                          
                    //2.2 Get the target string (all target transaltions)
                    boundaryFound_end=eachSentence.find(" ;;; ",boundaryFound+5);
                    string targetString=eachSentence.substr(boundaryFound+5,boundaryFound_end-boundaryFound-5);
                    size_t targetFound=targetString.find(" ||| ");
                    size_t probFound=targetString.find(" ||| ",targetFound+5);
                    size_t probFound_prev;               //store the previous probs position
                    int countPhraseOption=0;
                    while (targetFound!=string::npos)
                    {
                          if (probFound==string::npos)
                             probFound=targetString.size();
                          string target;          //store each target phrase
                          string tempProbString;  //store the probability string
                          vector<float> tempProbs; //store the probabilities
                          float probValue;         //store the probability value
                          
                          //2.3 Get each target string
                          if (countPhraseOption==0)
                             target = targetString.substr(0,targetFound);
                          else
                              target= targetString.substr(probFound_prev+5,targetFound-probFound_prev-5);
                          
                          //2.4 Get the probability vector 
                          tempProbString=targetString.substr(targetFound+5,probFound-targetFound-5);   
                          istringstream probString(tempProbString);
                          while(probString>>probValue)
                              tempProbs.push_back(probValue);
                          
                          //2.5 Update the information  
                          tempPhraseOption[boundary][target]=tempProbs;  
                          countPhraseOption++;
                          probFound_prev=probFound;
                          targetFound=targetString.find(" ||| ",probFound+5);
                          if (targetFound!=string::npos)
                              probFound=targetString.find(" ||| ",targetFound+5);
                          }
                    //3. Get the next boundary
                    boundaryFound_end=boundaryFound;
                    countBoundaryOption++;
                    boundaryFound=eachSentence.find(" ::: ",boundaryFound_end+5); //Get next boundary found
                    }
                    
              //4. Update the phrase option
              phraseOption[numSen]=tempPhraseOption;
              numSen++;
              }
        }
    else
    {
        cerr<<"Error in sentencePhraseOption.cpp:Can't open the phrase option files!\n";
        exit(1);
        }
    
    }

/*
2. void createPhraseOption(int sentenceIndex, int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
*/

void sentencePhraseOptionSTR::createPhraseOption(int sentenceIndex, unsigned short phrase_boundary[], mapTargetProbOptionSTR targetProbs)
{
     //1. Whether the sentence is new sentence or not
     mapSentenceOptionSTR::iterator sentenceFound=phraseOption.find(sentenceIndex);
     
     //2. If the sentence option doesn't exist
     if (sentenceFound==phraseOption.end())
     {
         numSen++; //update the number of sentence
         vector<unsigned short> boundary;
         boundary.reserve(2);
         mapPhraseOptionSTR tempPhraseOption;       //store the phrase options
         
         //2.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //2.2 update the probability information
         tempPhraseOption[boundary]=targetProbs; //get the target probabilities
         phraseOption[sentenceIndex]=tempPhraseOption;
         }
     //3. If the sentence option exists
     else
     {
         vector<unsigned short> boundary;
         boundary.reserve(2);
         //3.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //3.2 update the probability information
         sentenceFound->second[boundary]=targetProbs; //get the target probabilities
         }
     }
     
/*
2*. void createPhraseOption(int phrase_boundary[], mapTargetProbOption targetProbs) --- update the reordering probabilities for a phrase pair
*/

void sentencePhraseOptionSTR::createPhraseOption(unsigned short phrase_boundary[], mapTargetProbOptionSTR targetProbs)
{
     //1. Whether the sentence is new sentence or not
     int sentenceIndex=0;
     mapSentenceOptionSTR::iterator sentenceFound=phraseOption.find(sentenceIndex);
     
     //2. If the sentence option doesn't exist
     if (sentenceFound==phraseOption.end())
     {
         numSen++; //update the number of sentence
         vector<unsigned short> boundary;
         boundary.reserve(2);
         mapPhraseOptionSTR tempPhraseOption;       //store the phrase options
         
         //2.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //2.2 update the probability information
         tempPhraseOption[boundary]=targetProbs; //get the target probabilities
         phraseOption[sentenceIndex]=tempPhraseOption;
         }
     //3. If the sentence option exists
     else
     {
         vector<unsigned short> boundary;
         boundary.reserve(2);
         //3.1 update the boundary information
         boundary.push_back(phrase_boundary[0]);
         boundary.push_back(phrase_boundary[1]);
         
         //3.2 update the probability information
         sentenceFound->second[boundary]=targetProbs; //get the target probabilities
         }
     }     



/*
3. vector<float> getPhraseProbs(int sentenceIndex, int phrase_boundary[], string targetPhrase, int numClass) --- get the phrase reordering probability
*/

vector<float> sentencePhraseOptionSTR::getPhraseProbs(int sentenceIndex, unsigned short phrase_boundary[], string targetPhrase, int numClass)
{
    //1. initialisation
    vector<float> reorderingProbs(numClass,1/numClass);
    mapSentenceOptionSTR::const_iterator sentenceFound=phraseOption.find(sentenceIndex);
    
    if (sentenceFound==phraseOption.end())
    {   
        cerr<<"Warning in sentencePhraseOption.cpp: can't find the sentence index.\n";
        return reorderingProbs;
        }
    else
    {
        //2. If found the sentence, then find the boundary vector
        vector<unsigned short> boundary;
        boundary.push_back(phrase_boundary[0]);
        boundary.push_back(phrase_boundary[1]);
        
        mapPhraseOptionSTR::const_iterator boundaryFound=sentenceFound->second.find(boundary);
        if (boundaryFound==sentenceFound->second.end())
        {
            cerr<<"Warning in sentencePhraseOption.cpp: can't find the boundary phrase.\n";
            return reorderingProbs;
            }
        else
        {
            //3. If found the boundary, then find the target phrase
            mapTargetProbOptionSTR::const_iterator targetFound=boundaryFound->second.find(targetPhrase);
            if (targetFound==boundaryFound->second.end())
            {
                cerr<<"Warning in sentencePhraseOption.cpp: can't find the target phrase.\n";
                return reorderingProbs;
                }
            //4. If found the target phrase, then get the probabilities
            else
            {
                for (int i=0; i<numClass; i++)
                {
                    reorderingProbs[i]=targetFound->second[i];
                    }
                return reorderingProbs;
                }
            }
        }
    }
    
/*
4. void outputPhraseOption(char* outputFileName)  --- output all phrase options to a .txt fileSpecial function:
*/

void sentencePhraseOptionSTR::outputPhraseOption(char* outputFileName)
{
     //*. The structure: each line is a sentence
     //boundary ::: target phrase ||| reordering probs;;;
     ofstream outFile(outputFileName,ios::out);
     
     for (mapSentenceOptionSTR::const_iterator sentenceFound=phraseOption.begin();sentenceFound!=phraseOption.end();sentenceFound++)
     {
         for (mapPhraseOptionSTR::const_iterator boundaryFound=sentenceFound->second.begin(); boundaryFound!=sentenceFound->second.end(); boundaryFound++)
         {
             //1. Output the boundary
             outFile<<boundaryFound->first[0]<<" "<<boundaryFound->first[1]<<" ::: ";
             int countTargetItem=0;
             int stackSize=boundaryFound->second.size();
             for (mapTargetProbOptionSTR::const_iterator targetFound=boundaryFound->second.begin();targetFound!=boundaryFound->second.end();targetFound++)
             {
                 //2. Output the target phrase
                 outFile<<targetFound->first<<" ||| ";
                 
                 //3. output the probability values
                 for (int i=0; i< targetFound->second.size(); i++)
                 {
                     outFile<<targetFound->second[i]<<" ";
                     }
                 
                 countTargetItem++;
                 if (countTargetItem<stackSize)
                     outFile<<"||| ";
                 else
                     outFile<<";;; ";
                 }
             }
             
         outFile<<'\n';
         }
         
     //4. close the output file
     outFile.close();
     }


/*
4*. void outputPhraseOption(ofstream& outputFile)  --- output all phrase options to a .txt fileSpecial function:
*/

void sentencePhraseOptionSTR::outputPhraseOption(ofstream& outFile)
{
     //*. The structure: each line is a sentence
     //boundary ::: target phrase ||| reordering probs;;;
     
     for (mapSentenceOptionSTR::const_iterator sentenceFound=phraseOption.begin();sentenceFound!=phraseOption.end();sentenceFound++)
     {
         for (mapPhraseOptionSTR::const_iterator boundaryFound=sentenceFound->second.begin(); boundaryFound!=sentenceFound->second.end(); boundaryFound++)
         {
             //1. Output the boundary
             outFile<<boundaryFound->first[0]<<" "<<boundaryFound->first[1]<<" ::: ";
             int countTargetItem=0;
             int stackSize=boundaryFound->second.size();
             for (mapTargetProbOptionSTR::const_iterator targetFound=boundaryFound->second.begin();targetFound!=boundaryFound->second.end();targetFound++)
             {
                 //2. Output the target phrase
                 outFile<<targetFound->first<<" ||| ";
                 
                 //3. output the probability values
                 for (int i=0; i< targetFound->second.size(); i++)
                 {
                     outFile<<targetFound->second[i]<<" ";
                     }
                 
                 countTargetItem++;
                 if (countTargetItem<stackSize)
                     outFile<<"||| ";
                 else
                     outFile<<";;; ";
                 }
             }
             
         outFile<<'\n';
         }
     }
