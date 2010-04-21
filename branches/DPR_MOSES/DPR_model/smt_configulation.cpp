/***************************************************************************************************************

================================================================================================================
Main Process:
Generate the configulation file for the user to fill in.
Contains:
         
         
Remark:
*The maximum length of the file name should not exceed 100 (MAXCHAR) characters
*The maximum length of the sentence should not exceed 200 words
================================================================================================================

Input:
1. confFileName --- the name of the configulation file 
Output:
*. The configulation file (for user to fill in)
****************************************************************************************************************/


#include <cstdlib>
#include <iostream>
#include <fstream>
#define MAXCHAR 100

using namespace std;
using std::ofstream;

int main(int argc, char *argv[])
{
    //1. Initialisation (Get the name of the configulation file)
    char* confFileName=new char[MAXCHAR];
    
    
    if (argc<2)
    {
        cerr<<"Error in smt_configulation.cpp: Please input the name of the configulation file.\n";
        exit(1);
        }
    else
    {
        confFileName=argv[1];
        }
        
    ofstream confFile(confFileName,ios::out);
    
    //********************************************************************************************************
    //2.Output A. The corpus information:
    confFile<<"The information of the input source/target word corpora:\n";
    confFile<<"======================================================================================\n";
    confFile<<"Directory + name of the source corpus  (e.g. ./data/source.fr).\n";
    confFile<<"SourceCorpusFile = \n";
    confFile<<"Directory + name of the target corpus  (e.g. ./data/target.en).\n";
    confFile<<"TargetCorpusFile = \n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
    
    
    //********************************************************************************************************
    //3.Output B. The word class information:
    confFile<<"The information of the input source/target word class dictionaries:\n";
    confFile<<"======================================================================================\n";
    confFile<<"Directory + name of the source word class dictionary (e.g. ./corpus/fr.vcb.classes).\n";
    confFile<<"SourceWordClassFile = \n";
    confFile<<"Directory + name of the target word class dictionary (e.g. ./corpus/en.vcb.classes).\n";
    confFile<<"TargetWordClassFile = \n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
    
/*    //********************************************************************************************************
    //4.Output C. The word class corpus information:
    confFile<<"Please define the name of the (output) source/target word tags corpora.\n";
    confFile<<"======================================================================================\n";
    confFile<<"Directory + name of the source word-tag corpus (e.g. ./data/tagsSource.fr).\n";
    confFile<<"SourceTagCorpusFile = \n";
    confFile<<"Directory + name of the target word-tag corpus (e.g. ./data/tagsTarget.en).\n";
    confFile<<"TargetTagCorpusFile = \n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
 */   
    
    //********************************************************************************************************
    //5.Output D. For extracting the phrase pairs with their reordering distances:
    confFile<<"For extracting the phrase pairs with their reordering distances, please define:\n";
    confFile<<"======================================================================================\n";
    confFile<<"Directory + name of the word alignment file (e.g. ./model/aligned.grow-diag-final-and).\n";
    confFile<<"alignmentFile = \n";
    confFile<<"Directory + name of the (output) phrase pairs table (e.g. ./data/extractPhraseTrainTable).\n";
    confFile<<"phraseTableFile = \n";
    confFile<<"(Optional) Directory + name of the test corpus (if filled, only extract the phrases from the training corpora that appear in this file).\n";
    confFile<<"TestFileName = \n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
    
    //********************************************************************************************************
    //6.Output E. For generating the phrase reordering probabilities with structured learning:
    confFile<<"For generating the phrase reordering probabilities with structured learning, please define:\n";
    confFile<<"======================================================================================\n";
    confFile<<"Directory + name of the (input) phrase translation table (recommended using filtered Moses's phrase table) (e.g. ./model/phrase-table).\n";
    confFile<<"phraseTranslationTable = \n";
    confFile<<"Extract the top N translations for a source phrase (based on the probabilities by Moses's phrase table). \n";
    confFile<<"maxTranslations = 100 \n";
    confFile<<"If the phrase translation table is already filtered, fill 1 and 0 otherwise.\n";
    confFile<<"tableFilterLabel = 1 \n";
    confFile<<"Directory + name of the (output) weight parameter matrix (e.g. ./data/weightMatrix).\n";
    confFile<<"weightMatrixFile = \n";
    confFile<<"Need to train the weight matrix (If you have trained the weight matrix, then use 0)? 1. Yes, 0. No. \n";
    confFile<<"weightMatrixTrainLabel = 1\n";
    confFile<<"Directory + name of the (output) sentence phrase options table (e.g. ./data/phraseOption).\n";
    confFile<<"phraseOptionFile = \n";
    confFile<<"Directory + name of the test corpus (Used for extracting the phrase options).\n";
    confFile<<"TestFile = \n";
    confFile<<"Please define the style of outputing the sentence phrase options: If 1, store all sentence options first then output them at once, use large memory but faster; if 0, collect phrase options for one sentence and output, use less memory but slower.\n";
    confFile<<"batchOutputLabel = 1 \n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
    
    //********************************************************************************************************
    //7.Output F. for the structured learning framework
    confFile<<"Please fill in the parameters for seting up the structured learning framework:\n";
    confFile<<"======================================================================================\n";
    confFile<<"The maximum length of the phrases extracted (e.g. 7 as used in MOSES).\n";
    confFile<<"maxPhraseLength = 7\n";
    confFile<<"The class setup of the structured learning framework (i.e. 3 or 5).\n";
    confFile<<"classSetup = 3\n";
    confFile<<"Prune the phrase pairs whose reordering distance (d) is greater than a const distance (distCut).\n";
    confFile<<"distCut = 15\n";
    confFile<<"The maximum length of the ngram features in the ngram dictionary (e.g. usually choose 3 or 4).\n";
    confFile<<"maxNgramSize = 4\n";
    confFile<<"The window size around the source phrases (for extracting the ngram features, usually choose 3 or 4).\n";
    confFile<<"windowSize = 3\n";
    confFile<<"Prune the ngram features that occur less than the minimum number of occurance (minPrune).\n";
    confFile<<"minPrune = 1\n";
    confFile<<"Prune the cluster that has examples less than the minimum number of training examples (minTrainingExample).\n";
    confFile<<"minTrainingExample = 10\n";
    confFile<<"The maximum iteration for the perceptron-based structured learning (please refer to Ni et al., 2009) algorithm.\n";
    confFile<<"maxRound = 500\n";
    confFile<<"The step size for the perceptron-based structured learning (please refer to Ni et al., 2009) algorithm.\n";
    confFile<<"step = 0.05\n";
    confFile<<"The error tolerance for the perceptron-based structured learning (please refer to Ni et al., 2009) algorithm.\n";
    confFile<<"eTol = 0.001\n";
    confFile<<"======================================================================================\n";
    confFile<<"\n\n";
    //********************************************************************************************************
    
    
    confFile.close();
    //system("PAUSE");
    return EXIT_SUCCESS;
}
