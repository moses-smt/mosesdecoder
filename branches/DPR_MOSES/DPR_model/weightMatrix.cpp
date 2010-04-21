/*
**********************************************************
Cpp file ---------- weightMatrix.cpp

There are two classes
---------------------------------------------------------------------------------------------------------------
*. weightMatrixW
Components:
1. weightMatrix --- store the source phrase -> start position of the cluster values in a weightMatrix.txt file
2. numCluster --- number of clusters (source phrase) 

Functions:
1. int getNumCluster() --- get the number of clusters
2. void writeWeightMatrix(char* outputFileName) --- write the weight matrix (source phrase ||| start position) in a .txt file
3. void insertWeightCluster(string sourcePhrase, unsigned long long startPos) --- insert the start position of a new weight cluster
4. unsigned long long getWeightClusterPOS(string sourcePhrase) --- get the start position of a weight cluster given a source phrase

Special function:
1. weightMatrixW() --- create an empty dictionary
2. weightMatrixW(char* inputFileName) --- get the source phrase and the start position of the each source clusters
                                          Format: source phrase ||| start position in a .txt file
---------------------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------------------
*. weightClusterW --- (vector<map(int,float)>) store orientation class -> ngram features -> values
Components:
1. weightCluster --- store orientation -> ngram features -> values
2. numOrientation --- the number of orientation classes
3. sourcePhrase --- the cluster name (string)
Functions:
1. int getNumOrientation() --- get the number of orientation classes
2. string getClusterName() --- get the name of the source phrase
3. int writeWeightCluster(ofstream& outputFile) --- write the weight matrix and return the number of characters written
                                                                         Format: source phrase '\n'; ngram features values (each line) '\n';
4. void getWeightCluster(ifstream& inputFile,  int numClass, unsigned long long startPos) --- get the weight cluster from a .txt file
5. void structureLearningW(vector<vector<int> > phraseTable, int maxRound, float step, float eTol) --- learn the weight cluster W
                                                                <vector<vector<int> > store the examples, in each vector<int>
                                                                first one is orientation and the others are ngram features
6. vector<float> structureLearningConfidence(vector<int> featureList) --- return the confifence W^{T}phi(x) for each class
6*. vector<float> structureLearningConfidence(vector<int> sourceFeature, vector<int> targetFeature) --- (overloaded function) return the confifence W^{T}phi(x) for each class
Special function:
1. weightClusterW(string sourcePhrasse, int numClass) --- create an empty weight cluster W with numOrientation classes
2. weightClusterW(ifstream& inputFile,  int numClass, int startPos) --- get the weight cluster from a .txt weight file
---------------------------------------------------------------------------------------------------------------

*Remark:
        There are only two class setup:
        A. three-class setup: d<0; d=0; d>0.
        B. five-class setup: d<=-5; -5<d<0; d=0; 0<d<5; d>=5.
*The user can implement new class setup on his own.
***********************************************************
*/

#include "weightMatrix.h"

/*
1. constructor
*/
weightMatrixW::weightMatrixW()
{
  numCluster=0;
  
}

weightMatrixW::weightMatrixW(char* inputFileName)
{
    //1. open the file
    numCluster=0;
    ifstream inputFile(inputFileName,ios::binary);
    if (inputFile.is_open())
    {
        //2. For each line
        string eachLine;
        int countLine=0;
        while (getline(inputFile,eachLine,'\n'))
        {
              countLine++;
              size_t breakFound = eachLine.find(" |||");
              if (breakFound==string::npos)
                  {cerr<<"Error in reading the weightMatrix index file ('in weightMatrix.cpp'): can't find the source phrase in line: "<<countLine<<"."<<'\n';
                   exit(1);}
              else
              {
                  //2.1 Read the source phrase and the start position
                  numCluster++;
                  string source=eachLine.substr(0,breakFound); //get the source phrase
                  istringstream tempString(eachLine.substr(breakFound+4));
                  unsigned long long startPos;
                  tempString>>startPos;                 //get the start position of the weight cluster
                  
                  //2.2 store them in the weight matrix file
                  weightMatrix[source]=startPos;       
                  }
              
              }
        }
    else
    {
        cerr<<"Error in weightMatrix.cpp: Can't open the weight matrix index file! In 'weightMatrix.cpp' \n";
        exit(1);
        }
        
    inputFile.close();
}


/*
2. int getNumCluster() --- get the number of clusters
*/
int weightMatrixW::getNumCluster()
{
    return numCluster;
    }
    
/*
3. void writeWeightMatrix(char* outputFileName) --- write the weight matrix (source phrase ||| start position) in a .txt file
*/
void weightMatrixW::writeWeightMatrix(char* outputFileName)
{
     //1. initialisation
     ofstream outputFile(outputFileName,ios::out);
     
     //2. for each cluster in weight matrix, output the information
     for (weightMatrixMap::const_iterator key=weightMatrix.begin();key!=weightMatrix.end();key++)
     {
         outputFile<<key->first<<" ||| "<<key->second<<'\n';
         }
         
     outputFile.close();
     }

/*
4. void insertWeightCluster(int startPos) --- insert the start position of a new weight cluster
*/
void weightMatrixW::insertWeightCluster(string sourcePhrase, unsigned long long startPos)
{
     weightMatrixMap::iterator key= weightMatrix.find(sourcePhrase);
     if (key==weightMatrix.end())
        {
            weightMatrix[sourcePhrase]=startPos;
            numCluster++;
            }
     else
     {
         cerr<<"Error in weightMatrix.cpp: The weight matrix for the cluster: "<<sourcePhrase<<" has existed!\n";
         exit(1);
         }
     }
     
/*
5. int getWeightClusterPOS(string sourcePhrase) --- get the start position of a weight cluster given a source phrase
*/
unsigned long long weightMatrixW::getWeightClusterPOS(string sourcePhrase)
{
    weightMatrixMap::iterator key= weightMatrix.find(sourcePhrase);
    if (key==weightMatrix.end())
    {
       //cerr<<"Can't find the weight matrix for the cluster! Return -1!\n";
       return numeric_limits<unsigned long long>::max(); //If no position, return the maximum long
       }
    else
        return key->second;
    }



/*************************************************************************************
-----------------------------------EVIL SEPARATION LINE--------------------------------
**************************************************************************************/

/*
1. constructor
*/

weightClusterW::weightClusterW(string source, int numClass)
{
    //Initialisation
    numOrientation=numClass;
    sourcePhrase=source;
    
    //The class setup
    if (numClass==3)
    {
        for (int i=0;i<numClass; i++)
       {
            weightClusterMap Map0;
            weightCluster.push_back(Map0);
            }
            
       //Create the distance matrix
       for (int i=0; i<numClass; i++)
       {
           for (int j=0; j<numClass; j++)
           {
               if (i==j)
                   distMatrix[i][j]=0;
               else
                   distMatrix[i][j]=1;
               }
           }
        }
    else if (numClass==5)
    {
        for (int i=0;i<numClass; i++)
        {
            weightClusterMap Map0;
            weightCluster.push_back(Map0);
            }
            
        //Create the distance matrix
       for (int i=0; i<numClass; i++)
       {
           for (int j=0; j<numClass; j++)
           {
               if (i==j)
                   distMatrix[i][j]=0;
               else
                   distMatrix[i][j]=1;
                   
               distMatrix[0][1]=0.5;
               distMatrix[1][0]=0.5;
               distMatrix[3][4]=0.5;
               distMatrix[4][3]=0.5;
               }
           }
        }
     else
     {
         cerr<<"Error in 'weightMatrix.cpp': The class setup is not defined.\n";
         exit(1);
         }
    }
    
weightClusterW::weightClusterW(ifstream& inputFile, int numClass, unsigned long long startPos)
{
    //1. initialisation
    numOrientation=numClass;
    string eachLine;
    inputFile.seekg(startPos,ios::beg); //set the offset
    
    //2. Get the information
    getline(inputFile,sourcePhrase,'\n'); //the source phrase information
    
    if (numClass==3)
    {
       //2.1 For each class, construct the weightCluster Map
       for (int i=0;i<numClass; i++)
       {
        getline(inputFile,eachLine,'\n');
        int key;
        float value;
        istringstream tempString(eachLine);
        weightClusterMap Map0;
        while(tempString>>key>>value)
            Map0[key]=value;
        weightCluster.push_back(Map0);
        }
        
        //2.2 create the distance matrix
       for (int i=0; i<numClass; i++)
       {
           for (int j=0; j<numClass; j++)
           {
               if (i==j)
                   distMatrix[i][j]=0;
               else
                   distMatrix[i][j]=1;
               }
           }
        
       }
    else if (numClass==5)
    {
         //2.1 For each class, construct the weightCluster Map
       for (int i=0;i<numClass; i++)
       {
        getline(inputFile,eachLine,'\n');
        int key;
        float value;
        istringstream tempString(eachLine);
        weightClusterMap Map0;
        while(tempString>>key>>value)
            Map0[key]=value;
        weightCluster.push_back(Map0);
        }
        
         //2.2 Create the distance matrix
       for (int i=0; i<numClass; i++)
       {
           for (int j=0; j<numClass; j++)
           {
               if (i==j)
                   distMatrix[i][j]=0;
               else
                   distMatrix[i][j]=1;
                   
               distMatrix[0][1]=0.5;
               distMatrix[1][0]=0.5;
               distMatrix[3][4]=0.5;
               distMatrix[4][3]=0.5;
               }
           }
         }
    else
    {
        cerr<<"Error in 'weightMatrix.cpp': The class setup is not defined.\n";
        exit(1);
        }
    }

/*
2. int getNumOrientation() --- get the number of classes
*/
int weightClusterW::getNumOrientation()
{
    return numOrientation;
    }
    
/*
3. string getClusterName() --- get the cluster name (source phrase)
*/
string weightClusterW::getClusterName()
{
       return sourcePhrase;
       }


/*
4. int writeWeightCluster(ofstream& outputFile) --- write the weight matrix and return the number of characters written
                                                                         Format: source phrase '\n'; ngram features values (each line) '\n';
*/

unsigned long long weightClusterW::writeWeightCluster(ofstream& outFile)
{
    //1. Initialisation
    int streamSize=0;                           //store how many bits of the output stream
    unsigned long long startPos=outFile.tellp();     //record the start position of this cluster
    
    //2. write the weight cluster
    outFile<<sourcePhrase<<'\n';
    for (int i=0;i<numOrientation;i++)
    {
        weightClusterMap Map0=weightCluster[i];
        for (weightClusterMap::const_iterator key=Map0.begin(); key!=Map0.end(); key++)
        {
            outFile<<key->first<<" "<<key->second<<" ";
            }
        outFile<<'\n';
        }
    
    //3. return the start position of the weight vector
    return startPos;
}

/*
5. void getWeightCluster(ifstream& inputFile,  int numClass, int startPos) --- get the weight cluster from a .txt file
*/
void weightClusterW::getWeightCluster(ifstream& inputFile, int numClass, unsigned long long startPos)
{
     //1. initialisation
     numOrientation=numClass;
     string eachLine;
     inputFile.seekg(startPos,ios::beg); //set the offset
     
     //2. Get the information
    getline(inputFile,sourcePhrase,'\n'); //the source phrase information
    
    if (numClass==3)
    {
       //2.1 For each class, construct the weightCluster Map
       for (int i=0;i<numClass; i++)
       {
        getline(inputFile,eachLine,'\n');
        int key;
        float value;
        istringstream tempString(eachLine);
        weightClusterMap Map0;
        while(tempString>>key>>value)
            Map0[key]=value;
        weightCluster.push_back(Map0);
        }
        
    }
    else if (numClass==5)
    {
         //2.2 For each class, construct the weightCluster Map
       for (int i=0;i<numClass; i++)
       {
        getline(inputFile,eachLine,'\n');
        int key;
        float value;
        istringstream tempString(eachLine);
        weightClusterMap Map0;
        while(tempString>>key>>value)
            Map0[key]=value;
        weightCluster.push_back(Map0);
        }
         }
    else
    {
        cerr<<"Error in 'weightMatrix.cpp': The class setup is not defined.\n";
        exit(1);
        }
     
 }
 
 
/*
6. void structureLearningW(vector<vector<int> > phraseTable) --- learn the weight cluster W
Input:
1) phraseTable --- the training examples, each vector<int> is an example, first value is the orientation class
2) maxRound --- the maximum number of iteration
3) step --- the update step size
4) eTol --- the error tolerance for the risk function
Output:
1) weightCluster --- the updated weight cluster

Remark:
The structure learning approach uses the stochastical gradient descent algorithm.
For more details, please refer to Ni's PhD thesis or (Ni, et al., 2009) 
*/

void weightClusterW::structureLearningW(vector<vector<int> > phraseTable, int maxRound, float step, float eTol)
{
     //1. Initialisation
     int N=phraseTable.size();                                 //Number of examples
     eTol=eTol*N;
     /* For test only, the risk function value for each iteration
     vector<float> riskValue(maxRound,0.0);
     */
     
     float JW_pre=-100000.0; //The risk value in the previous iteration
     float JW;               //store risk value in this iteration
     vector<int> stochasticData; //store the data index (then will be shuffled)
     for (int i=0; i<N; i++)
         stochasticData.push_back(i);
     
     //2. For each iteration
     for (int t=0; t<maxRound; t++)
     {
         //2.1 Initialisation
         JW=0;
         int failExample=0;                 //count mis-classified examples
         vector<int> countFail;            //store the mis-classified examples (first: index i; second: pseudo class marginIndex)
         
         //*. Stochastic gradient, random shuffle the data order
         random_shuffle(stochasticData.begin(),stochasticData.end()); 
         for (int i=0; i<N; i++)
         {
             //2.2 For each example 
             int orientationClass=phraseTable[stochasticData[i]][0];
             int feaNum=phraseTable[stochasticData[i]].size()-1; //feature number
             float norm=sqrt(feaNum);                            //norm of the feature
             vector<float> prediction(numOrientation,0.0);       //store the prediction of the classes
             
                         
             //2.3 Get the prediction of the orientations
             for (int kk=1; kk<=feaNum; kk++)
             {
                 int featureIndex=phraseTable[stochasticData[i]][kk];
                 for (int k=0; k<numOrientation; k++ )
                 {
                     //2.3.1 search the key in the weight cluster (class k, feature kk)
                     weightClusterMap::iterator keyFound=weightCluster[k].find(featureIndex);
                     //2.3.2 If found the key, update the prediction
                     if (keyFound!=weightCluster[k].end())
                        prediction[k]+=keyFound->second/norm;
                     }
                 }
                 
             //2.4 Find the pseudo-example c_max
             float predVal=prediction[orientationClass]; //the preiction value for the correct class
             float maxMargin=-99999;
             int marginIndex=-1;
             
             /*Test code
             cout<<orientationClass<<": ";
             for (int sss=0;sss<prediction.size();sss++)
                 cout<<prediction[sss]<<" ";
             cout<<'\n';*/
             
             for (int k=0; k<numOrientation; k++)
             {
                 if (k!=orientationClass)
                 
                 {
                       //2.4.1 set maxMargin=pesudoVal+delta(marginIndex,trueClass)
                       float pseudoVal=prediction[k]+distMatrix[k][orientationClass];
                       //2.4.2 update the maxMargin
                       if (pseudoVal>maxMargin)
                       {
                           maxMargin=pseudoVal;
                           marginIndex=k;
                           }
                       }
                 }
                 
             //2.5 If it is a support vector, update the countFail DB
             if (predVal<maxMargin)
             {
                 JW+=(maxMargin-predVal); //Update the risk value
                 failExample++;           //update the mis-classified data
                 countFail.push_back(stochasticData[i]); //get the example
                 countFail.push_back(marginIndex);       //get the pseudo-class
                 }
                 
             //2.6 If there are more than 5 mis-classified examples, do stochastic gradient
             if (countFail.size()>=10 or i==N-1)
             {
                 for (int cc=0;cc<countFail.size();cc=cc+2)
                 {
                     //2.6.1 Get the example information
                     int ii=countFail[cc];                  //index of the example
                     int pseudo_class=countFail[cc+1];      //pseudo-class
                     int true_class=phraseTable[ii][0];     //the true class
                     int tempFeaNum=phraseTable[ii].size()-1; //feature number
                     float tempNorm=sqrt(tempFeaNum);         //norm of the example
                     
                     
                     
                     //2.6.2 Update the features
                     for (int kk=1;kk<=tempFeaNum;kk++)
                     {
                         //2.6.2.1 Update the position part (true class)
                         int featureIndex=phraseTable[ii][kk];
                         weightClusterMap::iterator keyFound=weightCluster[true_class].find(featureIndex);
                         if (keyFound==weightCluster[true_class].end())
                             weightCluster[true_class][featureIndex]=step/tempFeaNum;
                         else
                             keyFound->second=keyFound->second+step/tempFeaNum;
                             
                         //2.6.2.2 Update the negative part (pseudo class)
                         keyFound=weightCluster[pseudo_class].find(featureIndex);
                         if (keyFound==weightCluster[pseudo_class].end())
                             weightCluster[pseudo_class][featureIndex]=-step/tempFeaNum;
                         else
                             keyFound->second=keyFound->second-step/tempFeaNum;
                         }
                     }
                 countFail.clear(); //clear the countFail content
                 }
             }
         
         /*For test only, update the risk value in each round
         riskValue.push_back(JW);
         */
         
         
         //2.7 exam the early stopping criterion
         if (failExample==0)
             break; //early stor
         else
         {
             /*TEST CODE: display the risk value
             cout<<"Th failExample is: "<<failExample<<" and the risk value in round "<<t<<" is: "<<JW<<'\n';
             */
             float errorTotal=fabs(JW-JW_pre);
             if (errorTotal<eTol)
                 break;
             else
                 JW_pre=JW;
             }  
         }
     }


/*
7. vector<float> structureLearningConfidence(vector<int> featureList) --- return the confifence W^{T}phi(x) for each class
*/

vector<float> weightClusterW::structureLearningConfidence(vector<int> featureList)
{
    //initialisation
    vector<float> prediction(numOrientation,0.0);
    double norm=sqrt(featureList.size());
    
    //Update the prediciton
    for (int kk=0; kk<featureList.size(); kk++)
    {
        int featureIndex = featureList[kk];
        for (int k=0; k<numOrientation; k++)
        {
            weightClusterMap::iterator keyFound=weightCluster[k].find(featureIndex);
            if (keyFound!=weightCluster[k].end())
                prediction[k]+=keyFound->second/norm;
            }
        }
        
    //return the preciction (normalised)
    norm=0;
    for (int k=0; k<numOrientation;k++)
    {
        prediction[k]=exp(prediction[k]);
        norm+=prediction[k];
        }
}

/*
7*. vector<float> structureLearningConfidence(vector<int> sourceFeature, vector<int> targetFeature) --- (overloaded function) return the confifence W^{T}phi(x) for each class
*/
vector<float> weightClusterW::structureLearningConfidence(vector<int> sourceFeature, vector<int> targetFeature)//targetFeatureMapSTR::const_iterator targetFound)//
{
    //initialisation
    vector<float> prediction(numOrientation,0.0);
    double norm=sqrt(sourceFeature.size()+targetFeature.size());
    //double norm=sqrt(sourceFeature.size()+targetFound->second.size());
    
    //Update the prediciton (source feature)
    for (int kk=0; kk<sourceFeature.size(); kk++)
    {
        int featureIndex = sourceFeature[kk];
        for (int k=0; k<numOrientation; k++)
        {
            weightClusterMap::iterator keyFound=weightCluster[k].find(featureIndex);
            if (keyFound!=weightCluster[k].end())
                prediction[k]+=keyFound->second/norm;
            }
        }
        
    for (int kk=0; kk<targetFeature.size(); kk++)
    //for (int kk=0; kk<targetFound->second.size(); kk++)
    {
        int featureIndex = targetFeature[kk];
        //int featureIndex = targetFound->second[kk];
        for (int k=0; k<numOrientation; k++)
        {
            weightClusterMap::iterator keyFound=weightCluster[k].find(featureIndex);
            if (keyFound!=weightCluster[k].end())
                prediction[k]+=keyFound->second/norm;
            }
        }
    
    //return the preciction (normalised)
    norm=0;
    for (int k=0; k<numOrientation;k++)
    {
        prediction[k]=exp(prediction[k]);
        norm+=prediction[k];
        }        
    for (int k=0; k<numOrientation;k++)
        prediction[k]=prediction[k]/norm;
        
    return prediction;
    }

