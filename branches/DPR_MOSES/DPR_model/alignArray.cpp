/*
**********************************************************
Cpp file ---------- alignArray.cpp
Declaration of class alignArray
Store the word alignments for each sentence
Components:
1. align_FRtoEN --- the source to target alignemnt [source word Pos]->[target word Pos] (int, vector<int>)
2. align_ENtoFR --- the target to source alignemnt [target word Pos]->[source word Pos] (int, vector<int>)
Functions:
1. vector<int> getFRtoEN_alignment(int sourcePos) --- given the source position, return the corresponding alignments
                                                      if null, return vector {-1}
2. vector<int> getENtoFR_alignment(int targetPos) --- give the target position, return the corresponding alignements
                                                      if null, return vector {-1}
3. bool checkFRtoEN_alignment(int sourcePos) --- check the word is null alignment or not
4. bool checkENtoFR_alignment(int targetPos) --- check the word is null alignment or not
Special function:
1. alignArray() --- constructor,   create empty alignments
2. alignArray(string alignmentString) --- contructor, create the word alignment array using the word alignment strings
                                          the strings is sourcePos-targetPos, produced by GIZA++
***********************************************************
*/

#include "alignArray.h" //include definition of class alignArray from alignArray.h

/*
1. alignArray constructor
*/
alignArray::alignArray()
{}

alignArray::alignArray(string alignmentString)
{
    //1. initialization
    int iter_prev=0;            //start of the word
    int iter_next=0;            //end of the word (space or '\t' or end of the sentence)
    int pos_fr=-100;            //the source position
    int pos_en=-100;            //the target position
    
    //2. for each word alignment
    while (iter_prev<=alignmentString.size())
    {
          if (alignmentString[iter_next]=='-')
          {
               //2.1 read the source position (string)
               istringstream subString(alignmentString.substr(iter_prev,iter_next-iter_prev));
               subString>>pos_fr; //convert it to integer
               iter_prev=iter_next+1;
               iter_next++;
               }
          else if (alignmentString[iter_next]==' ' or iter_next==alignmentString.size())
          {
               alignDict::iterator pos_found;
               //2.2 read the target position (string)
               istringstream subString(alignmentString.substr(iter_prev,iter_next-iter_prev));
               subString>>pos_en; //convert it to integer
               iter_prev=iter_next+1;
               iter_next++;
               
               //2.3 update the alignment matrix (source side)
               pos_found=align_FRtoEN.find(pos_fr); //try to find key pos_fr
               if (pos_found==align_FRtoEN.end())
               {
                   //2.3.1 if the key doesn't exist
                   vector<int> tempValue;
                   tempValue.push_back(pos_en); //push pos_en
                   align_FRtoEN[pos_fr]=tempValue;
                   }
               else
               {
                   //2.3.2 if the key exist
                   pos_found->second.push_back(pos_en);//push pos_en
                   }
                   
               //2.4 update the alignment matrix
               pos_found=align_ENtoFR.find(pos_en); //try to find key pos_en
               if (pos_found==align_ENtoFR.end())
               {
                   //2.4.1 if the key doesn't exist
                   vector<int> tempValue;
                   tempValue.push_back(pos_fr); //push pos_fr
                   align_ENtoFR[pos_en]=tempValue;
                   }
               else
               {
                   //2.4.2 if the key exist
                   pos_found->second.push_back(pos_fr);//push pos_fr
                   }
               
               }
          else //else move forward the pointer
          {
              iter_next++;
              }
          }
          
    }


/*
2. vector<int> getFRtoEN_alignment(int sourcePos) --- given a source position, return the target aligned position
                                                      return {-1} if the position is aligned to null (can't found the key)
*/

vector<int> alignArray::getFRtoEN_alignment(int sourcePos)
{
    vector<int> targetPos(1,-1);
    alignDict::const_iterator pos_found=align_FRtoEN.find(sourcePos);
    if (pos_found!=align_FRtoEN.end())
    {
        targetPos=pos_found->second; //get the target postion (vector)
        }
        
    return targetPos;
    }


/*
3. vector<int> getENtoFR_alignment(int targetPos) --- given a target position, return the source aligned position
                                                      return {-1} if the position is aligned to null (can't found the key)
*/

vector<int> alignArray::getENtoFR_alignment(int targetPos)
{
    vector<int> sourcePos(1,-1);
    alignDict::const_iterator pos_found=align_ENtoFR.find(targetPos);
    if (pos_found!=align_ENtoFR.end())
    {
        sourcePos=pos_found->second; //get the source postion (vector)
        }
        
    return sourcePos;
    }

/*
4. bool checkFRtoEN_alignment(int sourcePos) --- check the word is null alignment or not
*/

bool alignArray::checkFRtoEN_alignment(int sourcePos)
{
     alignDict::const_iterator pos_found=align_FRtoEN.find(sourcePos);
     if (pos_found==align_FRtoEN.end())
         return false;
     else
         return true;
     
     }

/*
5. bool checkENtoFR_alignment(int targetPos) --- check the word is null alignment or not
*/

bool alignArray::checkENtoFR_alignment(int targetPos)
{
     alignDict::const_iterator pos_found=align_ENtoFR.find(targetPos);
     if (pos_found==align_ENtoFR.end())
         return false;
     else
         return true;
     
     }
