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
  cerr<<"[--reference|-r] comma separated list of reference files (default reference.txt)"<<endl;
  cerr<<"[--binary|-b] use binary output format (defaults to text )"<<endl;
  cerr<<"[--nbest|-n] the nbest file (default nbest.txt)"<<endl;
  cerr<<"[--scfile|-S] the scorer data output file (default score.data)"<<endl;
  cerr<<"[--ffile|-F] the feature data output file data file (default feature.data)"<<endl;
  cerr<<"[--prev-ffile|-E] the previous feature data output file data file (default None)"<<endl;
  cerr<<"[--prev-scfile|-R] the previous scorer data output file (default None)"<<endl;
  cerr<<"[-v] verbose level"<<endl;
  cerr<<"[--help|-h] print this message and exit"<<endl;
  exit(1);
}


static struct option long_options[] =
  {
    {"sctype",required_argument,0,'s'},
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
    string referenceFile("reference.txt");
    string nbestFile("nbest.txt");
    string scoreDataFile("score.data");
    string featureDataFile("feature.data");
    string prevScoreDataFile;
    string prevFeatureDataFile;
    bool binmode = false;
    int verbosity = 0;
    int c;
    while ((c=getopt_long (argc,argv, "s:r:n:S:F:R:E:v:hb", long_options, &option_index)) != -1) {
        switch(c) {
            case 's':
                scorerType = string(optarg);
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

    if ((prevScoreDataFile.length() > 0 && prevFeatureDataFile.length() == 0)
        || (prevScoreDataFile.length() == 0 && prevFeatureDataFile.length() > 0)) {
        cerr << "Error: either previous score and feature files are both specified, or neither" << endl;
        return EXIT_FAILURE;
    }

    TRACE_ERR("Score statistics output: " << scoreDataFile << endl);
    TRACE_ERR("Features output: " << featureDataFile << endl);

    
    if (binmode) {
        cerr << "Warning: binary mode not yet implemented" << endl;
        binmode = false;
    }

    vector<string> referenceFiles;
    size_t pos = 0;
    while (pos != string::npos && pos < referenceFile.length()) {
        size_t end = referenceFile.find(",",pos);
        if (end == string::npos) {
            referenceFiles.push_back(referenceFile.substr(pos));
            pos = end;
        } else {
            referenceFiles.push_back(referenceFile.substr(pos,end-pos));
            pos = end+1;
        }
        TRACE_ERR("Reference file: " << referenceFiles.back() << endl);
    }

    try {
        TRACE_ERR("Scorer type: " << scorerType << endl);
			  ScorerFactory sfactory;
        Scorer* scorer = sfactory.getScorer(scorerType);
				
        Timer timer;
        timer.start("Starting...");
        
        scorer->setReferenceFiles(referenceFiles);
        Data data(*scorer);
        
				if (prevScoreDataFile.length() > 0) {
            //load old data
            data.load(prevFeatureDataFile, prevScoreDataFile);
        }

				data.loadnbest(nbestFile);
								
        data.save(featureDataFile, scoreDataFile, binmode);
        timer.stop("Stopping...");
        return EXIT_SUCCESS;
    } catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }

}
