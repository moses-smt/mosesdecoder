
using namespace std;

#include <iostream>
#include "Parameter.h"
#include "Timer.h"
#include "Util.h"

#include "Scorer.h"
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
        if (parameter->isSet("help")) {
            parameter->Explain();
            return EXIT_SUCCESS;
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
		if (Scan<bool>(parameter->GetParam("OutputBinaryMode")[0])){
			binmode = true;
			TRACE_ERR("binary output mode is not yet implemented" << std::endl);
			binmode = false;
		}
		

        vector<string> references;
        const vector<string> &tmpreferences = parameter->GetParam("Reference");
        if (tmpreferences.size() == 0) {
            cerr << "Error: No reference files specified" << endl;
            return EXIT_FAILURE;
        }
        for(size_t i=0; i< tmpreferences.size(); i++)  {
                references.push_back(Scan<string>(tmpreferences[i]));
        }


	Timer timer;
	timer.start("Starting...");
    Scorer* scorer = 0;
    const vector<string>& scorerType = parameter->GetParam("Score");
    ScorerFactory sfactory;
    if (scorerType.size() ==  0) {
        scorer = sfactory.getScorer(sfactory.getTypes()[0]);
    } else {
        scorer = sfactory.getScorer(scorerType[0]);
    }

    // Check consistency of reference
	if (references.size() == 0) {
		TRACE_ERR("Error: You must specify atleast one reference file" << std::endl);
		return EXIT_FAILURE;
     }
     if (NbestFile.size() == 0) { 
        TRACE_ERR("Error: No nbest file specified" << std::endl);
        return EXIT_FAILURE;
     }
    
    scorer->setReferenceFiles(references);

	Data data(*scorer);


// Check consistency of files
	if ((!InputFeatureFile.empty() && InputScoreFile.empty()) ||
	    (InputFeatureFile.empty() && !InputScoreFile.empty()))
    {
		TRACE_ERR("You must define both InputFeatureFile and InputScoreFile (or neither of two)" << std::endl);
        return EXIT_FAILURE;
    }

// Load statistics
	if (!InputFeatureFile.empty() && !InputScoreFile.empty())
		data.load(InputFeatureFile, InputScoreFile);

//Load nbestfile
	data.loadnbest(NbestFile);

	data.save(OutputFeatureFile, OutputScoreFile, binmode);

	timer.stop("Stopping...");
	return 0;
}
