/*
**********************************************************
Cpp file ---------- wordClassDict.cpp
Declaration of class wordClassDict
Store the word class labels for each word
Components:
1. wordClassDict - map (function), store the word (string) and the word class index (int)
2. readDictCheck - return 0 if the phraseNgramDict can't read a dictionary file (can't find) or 1 otherwise;
3. numWords - the number of words in this corpus
Functions:
1. int getWordClass(string word) --- get the word class index (integer)
2. void createWCFile(char* inputFile,char* outputFile) --- generate word class files for the training/test corpus,
                                                           basing on the dictionary the objects hold.
3. bool checkReadFileStatus() --- check the read of the dictionary
4. int getNumWords()  --- get the vocaburary of the corpus
Special function:
1. wordClassDict(char* fileName) --- constructor,  read the word class dictionary from the file
                                     the word dictionary is provided by MOSES, in fr.vcb.classes (or en.vcb.classes)
***********************************************************
*/


//prevent multiple inclusions of head file
#include "wordClassDict.h" //include definition of class wordClassDict from wordClassDict.h

/*
1. constructor, read the word class dictionary (provided by mkcls or MOSES)
*/
wordClassDict::wordClassDict(char* dictFileName)
{
     numWords=0;
     ifstream inputDict(dictFileName,ios::in); //open the dictionary file
     if (inputDict.is_open())
     {
        readDictCheck=true;                           //can read the file
        string tempWord;                          //store the word
        int    tempWordClass;                     //store the word class
        while (inputDict>>tempWord>>tempWordClass)//for each word -> word class
        {
              wordClassDictionary[tempWord]=tempWordClass; //insert the key
              numWords++;                                 //update the number of words in this corpus
              }
     }
     else
     {
         readDictCheck=false;  //can't read the file
         }
     inputDict.close(); //close the dictionary file
                                   }

/*
2. check the read of the dictionary
*/
bool wordClassDict::checkReadFileStatus()
{
     return readDictCheck;
     }


/*
3. generate word class files for the training/test corpus, basing on the dictionary the objects hold.
*/
void wordClassDict::createWCFile(char* inputFile, char* outputFile)
{
     ifstream sentenceFile(inputFile,ios::in);    //the input word file
     ofstream wordClassFile(outputFile,ios::out); //the output word class file
     string eachSentence;                         //sentence string
     int countSentence=0;                         //count the number of sentence processed
     
     if (!sentenceFile.is_open())
     {
        cerr<<"Can't open the input corpus!\n";
        readDictCheck=false;
                              }
     
     while (getline(sentenceFile,eachSentence,'\n'))
     {
           countSentence++;
           //1. initilization
           istringstream str_eachSentence(eachSentence); //convert it to str-stream
           int sentenceWord=0;         //number of words in the sentences
           char tempWord[100];            //store each word, create a space to avoid potential error
           string sentenceArray[200]; //maximum-number of words (200) declare a sentence array
           
     
           //2. Get each word of the sentence
           while (str_eachSentence>>tempWord)
                {
                    sentenceArray[sentenceWord]=tempWord; //Get the words
                    sentenceWord++;                       //update the number of words
                }
                
           //3. Output the word class file
           for (int i=0; i<sentenceWord; i++)
           {   
               wordDict::iterator keySearch = wordClassDictionary.find(sentenceArray[i]);
               if (keySearch==wordClassDictionary.end())
                  wordClassFile<<-1; //write -1
               else
                  wordClassFile<<wordClassDictionary[sentenceArray[i]];
               if (i<sentenceWord-1)
                  wordClassFile<<" "; //output a space
               }
           wordClassFile<<'\n';
           
           if (countSentence%10000==0)
              cout<<"Have processed "<<countSentence<<" sentences.\n";
           }
     cout<<"All together processed "<<countSentence<<" sentences.\n";
     sentenceFile.close();
     wordClassFile.close(); 
     }

/*
4. int getNumWords()  --- get the vocaburary of the corpus
*/

int wordClassDict::getNumWords()
{return numWords;}


/*
5. getWordClass(string word) --- get the word class index (integer)
*/
int wordClassDict::getWordClass(string word)
{
    wordDict::const_iterator keyFound= wordClassDictionary.find(word);
    if (keyFound==wordClassDictionary.end())
    {
        return -1;
        }
    else
    {
        return keyFound->second;
        }
    
    }
