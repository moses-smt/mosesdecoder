
using namespace std;

#include <iostream>
#include "Parameter.h"
#include "Timer.h"
#include "Util.h"

#include "Scorer.h"
#include "BleuScorer.h"
#include "Data.h"

int main (int argc, char * argv[]) {

        // parse command line for parameters
        Parameter *parameter = new Parameter();
        if (!parameter->LoadParam(argc, argv))
        {
                parameter->Explain();
                delete parameter;
                return EXIT_FAILURE;            
        }

	std::string NbestFile, InputFeatureFile, OutputFeatureFile, InputScoreFile, OutputScoreFile;

        // Read files
        if(parameter->GetParam("NbestFile").size()) 
                NbestFile = parameter->GetParam("NbestFile")[0];

        if(parameter->GetParam("InputFeatureStatistics").size()) 
                InputFeatureFile = parameter->GetParam("InputFeatureStatistics")[0];

        if(parameter->GetParam("InputScoreStatistics").size()) 
                InputScoreFile = parameter->GetParam("InputScoreStatistics")[0];

        if(parameter->GetParam("OutputFeatureStatistics").size()) 
                OutputFeatureFile = parameter->GetParam("OutputFeatureStatistics")[0];

        if(parameter->GetParam("OutputScoreStatistics").size()) 
                OutputScoreFile = parameter->GetParam("OutputScoreStatistics")[0];

	bool binmode=false;
        if(parameter->GetParam("OutputBinaryMode").size()) 
		if (Scan<bool>(parameter->GetParam("recover-input-path")[0])){
			binmode = true;
			TRACE_ERR("binary output mode is not yet implemented" << std::endl);
			binmode = false;
		}
		

        vector<string> references;
        const vector<string> &tmpreferences = parameter->GetParam("Reference");
        for(size_t i=0; i< tmpreferences.size(); i++) 
                references.push_back(Scan<string>(tmpreferences[i]));


	Timer timer;
	timer.start("Starting...");

        BleuScorer scorer;
        scorer.setReferenceFiles(references);
        ScoreData sd(scorer);

	Data data(scorer);

// Check consistency of reference
	if (references.size() == 0){
		TRACE_ERR("You must specify atleast one reference file" << std::endl);
		return 1;
	}

// Check consistency of files
	if ((!InputFeatureFile.empty() && InputScoreFile.empty()) ||
	    (InputFeatureFile.empty() && !InputScoreFile.empty()))
		TRACE_ERR("You must define both InputFeatureFile and InputScoreFile (or neither of two)" << std::endl);

// Load statistics
	if (!InputFeatureFile.empty() && !InputScoreFile.empty())
		data.load(InputFeatureFile, InputScoreFile);

//Load nbestfile
	data.loadnbest(NbestFile);

	data.save(OutputFeatureFile, OutputScoreFile, binmode);

	timer.stop("Stopping...");
	return 0;
}
