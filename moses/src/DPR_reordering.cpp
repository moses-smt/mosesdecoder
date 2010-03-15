/*
**********************************************************
Cpp file ---------- DPR_reordering.cpp
The reordering feature function for MOSES
based on the DPR model proposed in (Ni et al., 2009)

Components:
       vector<unsigned long long> m_dprOptionStartPOS --- store the start pos for each sentence option (to read from the .txt file)
       ifstream sentenceOptionFile --- the stream file storing the sentence options
       int sentenceID --- the sentence ID (indicating which sentence option block is used)
       mapPhraseOption sentencePhraseOption --- sentence phrase option <left bound, right bound> -> target (string) -> probs
             
Functions:
0. Constructor: DPR_reordering(ScoreIndexManager &scoreIndexManager, const std::string &filePath, const std::vector<float>& weights)
       
1. interface functions:
       GetNumScoreComponents() --- return the number of scores the component used (usually 1)
       GetScoreProducerDescription() --- return the name of the reordering model
       GetScoreProducerWeightShortName() --- return the short name of the weight for the score
2. Score producers:
       Evaluate() --- to evaluate the reordering scores and add the score to the score component collection
       EmptyHypothesisState() --- create an empty hypothesis
       
3. Other functions:
       constructSentencePhraseOption() --- Construct sentencePhraseOption using sentenceID
       clearSentencePhraseOption() --- clear the sentence phrase options
**********************************************************
*/

#include "DPR_reordering.h"


namespace Moses
{

/*
1. constructor
*/
DPR_reordering::DPR_reordering(ScoreIndexManager &scoreIndexManager, const string filePath,  const string classString, const vector<float>& weights)
{
    //1. Add the function in the scoreIndexManager
    scoreIndexManager.AddScoreProducer(this);
    //2. Set the weight for this score producer
    const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
    
    //3. Get the class setup
    istringstream tempClassSetup(classString);
    tempClassSetup>>classSetup;
    if (classSetup==3)
    {
        for (int k=0; k<25; k++)
            WDR_cost.push_back(log10(exp(-(float) k)));
        unDetectProb = 0.3333;
        }
    else if (classSetup==5)
        unDetectProb = log10(0.2);
    else
        cerr<<"Error in DPR_reordering: Currently there is no class setup: "<<classSetup<<" in our model.\n";
    
    //4. get the start position of the sentence options
     string fileStartPos = filePath+".startPosition";                     //path of the sentence start position file
     ifstream sentencePOS((char*) fileStartPos.c_str(),ios::binary);
     string eachLine;
     while (getline(sentencePOS,eachLine,'\n'))
     {
            istringstream tempString(eachLine);
            unsigned long long tempValue;
            tempString>>tempValue;
            m_dprOptionStartPOS.push_back(tempValue); //Get the start position of each sentence option DB
            }
            
     //5. Read the first sentence option
     sentenceID=0;
     sentenceOptionFile.open((char*) filePath.c_str(),ios::binary);
     
     if (!sentenceOptionFile.is_open())
        cerr<<"Error in DPR_reordering.cpp: can not open the sentence options file!\n";
     else
         constructSentencePhraseOption(); //construct the first sentencePhraseOption
         
     sentencePOS.close();
}

/*
2. interface functions
*/

//return the number of score components
size_t DPR_reordering::GetNumScoreComponents() const
{
 return 1;
}

//return the description of this feature function  
string DPR_reordering::GetScoreProducerDescription() const
{
       return "Distance_phrase_reordering_probabilities_produders";
       }

//return the weight short name      
string DPR_reordering::GetScoreProducerWeightShortName() const
{
       return "weight-DPR";
       }
       
/*
3. the score producers
*/
const FFState* DPR_reordering::EmptyHypothesisState() const
{
      //Do nothing
      return NULL;
}


//given the hypothesis (and previous hypothesis) computed and add the reordering score  
FFState* DPR_reordering::Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator) const
{	
	//cerr << cur_hypo.GetInput();
	//cerr << cur_hypo.GetInput().GetTranslationId();
	
	//1. Check the sentence phrase option (check the ID starts from 0 or 1?)
	long int currentSentenceID  = cur_hypo.GetInput().GetTranslationId();
	if (sentenceID!=currentSentenceID)
	{
         sentenceID=currentSentenceID;
         constructSentencePhraseOption(); //construct the first sentencePhraseOption
         }
         
    //2. get the information current phrase: left_boundary, right_boundary, target translation
    //                       prev phrase:    right_boundary
    size_t prev_right_boundary;
    size_t curr_left_boundary;
    size_t curr_right_boundary;
    const Hypothesis* prevHypothesis = cur_hypo.GetPrevHypo();
    //check if there is a previous hypo
    if (prevHypothesis->GetId()==0)
        prev_right_boundary=-1;
    else
        prev_right_boundary=prevHypothesis->GetCurrSourceWordsRange().GetEndPos();
    
    const WordsRange currWordsRange = cur_hypo.GetCurrSourceWordsRange();
    curr_left_boundary = currWordsRange.GetStartPos();
    curr_right_boundary = currWordsRange.GetEndPos();
    string targetTranslation = cur_hypo.GetCurrTargetPhrase().ToString();
    
    //3. Get the reordering probability
    float reorderingProb = generateReorderingProb(curr_left_boundary, curr_right_boundary, prev_right_boundary, targetTranslation);
    
    //simple, update the score -1.0
    accumulator->PlusEquals(this,reorderingProb);
    return NULL;
}


/*
4. Other functions
*/

/*
4.1 Clear the content in sentencePhraseOption
*/
void DPR_reordering::clearSentencePhraseOption()
{
     for (mapPhraseOption::iterator iterator = sentencePhraseOption.begin(); iterator!= sentencePhraseOption.end(); iterator++)
     {
         iterator->second.clear(); //clear each map in mapTargetProbOption
         }
     sentencePhraseOption.clear(); //clear the components in sentencePhraseOption
     }
     
/*
4.2 Construct sentencePhraseOption using sentenceID
*/
void DPR_reordering::constructSentencePhraseOption() const
{
     //1. Get the start position of the sentence options
     const_cast<ifstream&>(sentenceOptionFile).seekg(m_dprOptionStartPOS[sentenceID],ios::beg); //set the offset
     string eachSentence;
     getline(const_cast<ifstream&>(sentenceOptionFile) ,eachSentence,'\n');
     
     //2. Search each separation 
     size_t boundaryFound = eachSentence.find(" ::: "); //find the separation between the boundary and the values
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
             {
                 if (classSetup==5)
                    probValue=log10(probValue); //get the log probability
                 tempProbs.push_back(probValue);
                 }
                          
             //2.5 Update the information  
             sentencePhraseOption[boundary][target]=tempProbs;  
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
     }

/*
4.3 generate the reordering probability
*/
float DPR_reordering::generateReorderingProb(size_t boundary_left, size_t boundary_right, size_t prev_boundary_right, string targetPhrase) const
{
      float reorderingProb;
      //1. get the distance reordering
      int reorderDistance = prev_boundary_right+1-boundary_left; //reordering distance
      int reorderOrientation = createOrientationClass(reorderDistance); //reordering orientation
      //2. get the boundary vector
      vector<unsigned short> phrase_boundary;
      phrase_boundary.push_back(boundary_left);
      phrase_boundary.push_back(boundary_right);
      mapPhraseOption::const_iterator boundaryFound = sentencePhraseOption.find(phrase_boundary);
      
      //3.1 If no this source phrase (then return equal probability)
      if (boundaryFound==sentencePhraseOption.end())
      {
          if (classSetup==3)
          {
              reorderingProb = WDR_cost[abs(reorderDistance)]; //using word-based distance reordering
              }
          else if (classSetup==5)
          {
               reorderingProb=unDetectProb;
               }
          }
      else
      {
          mapTargetProbOption::const_iterator targetFound = boundaryFound->second.find(targetPhrase);
          //3.2 if no this target phrase
          if (targetFound == boundaryFound->second.end())
          {
              if (classSetup==3)
              {
                  reorderingProb = WDR_cost[abs(reorderDistance)]; //using word-based distance reordering
                  }
              else if (classSetup==5)
              {
                   reorderingProb=unDetectProb;
                   }
              }
          //3.3 else, get normal reordering probability
          else
          {
              if (classSetup ==3)
              {
                  if (reorderOrientation==1) //special case: monotone
                  {
                      if (targetFound->second[1]>0.5)
                         reorderingProb=0.0;
                      else
                      {
                          float ratio=min(MAXRATIO, 1.0/(3*targetFound->second[1]));
                          reorderingProb=ratio*WDR_cost[1];
                          }
                      }
                  else
                  {
                      float ratio=min(MAXRATIO, 1.0/(3*targetFound->second[reorderOrientation]));
                      reorderingProb=ratio*WDR_cost[abs(reorderDistance)];
                      }
                  }
              else if (classSetup==5)
              {
                   reorderingProb=targetFound->second[reorderOrientation];
                   }
              }
          }
    
      return reorderingProb;   
      }



/*
4.4. int createOrientationClass(int dist,int classSetup) --- the create the orientation class
*/
int DPR_reordering::createOrientationClass(int dist) const
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
        cerr<<"Error in DPR_reordering: Currently there is no class setup: "<<classSetup<<" in our model.\n";
        }
    
  
    return orientationClass; //return the orientation class
    }

DPR_reordering::~DPR_reordering()
{
    sentenceOptionFile.close();
    }

} // namespace
