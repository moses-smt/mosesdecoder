/**
 * Extract features and score statistics from nvest file, optionally merging with
 * those from the previous iteration.
 * Developed during the 2nd MT marathon.
 **/

#include <iostream>
#include <string>
#include <vector>

#include <getopt.h>

#include "Data.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "Timer.h"
#include "Util.h"

using namespace std;

void usage() {
  cerr<<"usage: extractor [options])"<<endl;
  cerr<<"[--sctype|-s] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scconfig|-c] configuration string passed to scorer"<<endl;
  cerr<<"\tThis is of the form NAME1:VAL1,NAME2:VAL2 etc "<<endl;
  cerr<<"[--reference|-r] comma separated list of reference files"<<endl;
  cerr<<"[--binary|-b] use binary output format (default to text )"<<endl;
  cerr<<"[--nbest|-n] the nbest file"<<endl;
  cerr<<"[--scfile|-S] the scorer data output file"<<endl;
  cerr<<"[--ffile|-F] the feature data output file"<<endl;
cerr<<"[--prev-ffile|-E] comma separated list of previous feature data" <<endl;
  cerr<<"[--prev-scfile|-R] comma separated list of previous scorer data"<<endl;
  cerr<<"[-v] verbose level"<<endl;
  cerr<<"[--help|-h] print this message and exit"<<endl;
  exit(1);
}


static struct option long_options[] =
  {
    {"sctype",required_argument,0,'s'},
    {"scconfig",required_argument,0,'c'},
    {"reference",required_argument,0,'r'},
    {"binary",no_argument,0,'b'},
    {"nbest",required_argument,0,'n'},
    {"scfile",required_argument,0,'S'},
    {"ffile",required_argument,0,'F'},
    {"prev-scfile",required_argument,0,'R'},
    {"prev-ffile",required_argument,0,'E'},
    {"verbose",required_argument,0,'v'},
    {"help",no_argument,0,'h'},
    {0, 0, 0, 0}
  };
int option_index;

int main(int argc, char** argv) {
    //defaults
    string scorerType("BLEU");
    string scorerConfig("");
    string referenceFile("");
    string nbestFile("");
    string scoreDataFile("statscore.data");
    string featureDataFile("features.data");
    string prevScoreDataFile("");
    string prevFeatureDataFile("");
    bool binmode = false;
    int verbosity = 0;
    int c;
    while ((c=getopt_long (argc,argv, "s:r:n:S:F:R:E:v:hb", long_options, &option_index)) != -1) {
        switch(c) {
            case 's':
                scorerType = string(optarg);
                break;
            case 'c':
                scorerConfig = string(optarg);
                break;
            case 'r':
                referenceFile = string(optarg);
                break;
            case 'b':
                binmode = true;
                break;
            case 'n':
                nbestFile = string(optarg);
                break;
            case 'S':
                scoreDataFile = string(optarg);
                break;
            case 'F':
                featureDataFile = string(optarg);
                break;
            case 'E':
                prevFeatureDataFile = string(optarg);
                break;
            case 'R':
                prevScoreDataFile = string(optarg);
                break;
            case 'v':
                verbosity = atoi(optarg);
                break;
            default:
                usage();
        }
    }
    try {

//check whether score statistics file is specified
    if (scoreDataFile.length() == 0){
	throw runtime_error("Error: output score statistics file is not specified");
    }

//check wheter feature file is specified
    if (featureDataFile.length() == 0){
        throw runtime_error("Error: output feature file is not specified");
    }

//check whether reference file is specified when nbest is specified
    if ((nbestFile.length() > 0 && referenceFile.length() == 0)){
        throw runtime_error("Error: reference file is not specified; you can not score the nbest");

    }
 
    vector<string> nbestFiles;
    if (nbestFile.length() > 0){
        std::string substring;
        while (!nbestFile.empty()){
                getNextPound(nbestFile, substring, ",");
                nbestFiles.push_back(substring);
        }
    }

    vector<string> referenceFiles;
    if (referenceFile.length() > 0){
			std::string substring;
			while (!referenceFile.empty()){
				getNextPound(referenceFile, substring, ",");
				referenceFiles.push_back(substring);
			}
    }

    vector<string> prevScoreDataFiles;
    if (prevScoreDataFile.length() > 0){
			std::string substring;
			while (!prevScoreDataFile.empty()){
				getNextPound(prevScoreDataFile, substring, ",");
				prevScoreDataFiles.push_back(substring);
			}
    }

    vector<string> prevFeatureDataFiles;
    if (prevFeatureDataFile.length() > 0){
        std::string substring;
        while (!prevFeatureDataFile.empty()){
                getNextPound(prevFeatureDataFile, substring, ",");
                prevFeatureDataFiles.push_back(substring);
        }
    }

    if (prevScoreDataFiles.size() != prevFeatureDataFiles.size()){
			throw runtime_error("Error: there is a different number of previous score and feature files");
    }

		
		if (binmode) cerr << "Binary write mode is selected" << endl;
		else cerr << "Binary write mode is NOT selected" << endl;
			
		TRACE_ERR("Scorer type: " << scorerType << endl);
		ScorerFactory sfactory;
		Scorer* scorer = sfactory.getScorer(scorerType,scorerConfig);
				
		Timer timer;
		timer.start("Starting...");

		//load references        
		if (referenceFiles.size() > 0)
			scorer->setReferenceFiles(referenceFiles);

		Data data(*scorer);
		
	//load old data
		for (size_t i=0;i < prevScoreDataFiles.size(); i++){
			data.load(prevFeatureDataFiles.at(i), prevScoreDataFiles.at(i));
		}

	//computing score statistics of each nbest file
		for (size_t i=0;i < nbestFiles.size(); i++){
			data.loadnbest(nbestFiles.at(i));
		}
								
		if (binmode)
			cerr << "Binary write mode is selected" << endl;
		else
			cerr << "Binary write mode is NOT selected" << endl;
			
		data.save(featureDataFile, scoreDataFile, binmode);
		timer.stop("Stopping...");
		return EXIT_SUCCESS;
    } catch (const exception& e) {
			cerr << "Exception: " << e.what() << endl;
			return EXIT_FAILURE;
    }

}
