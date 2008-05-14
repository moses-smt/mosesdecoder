
using namespace std;

#include <iostream>
#include "Parameter.h"
#include "Timer.h"
#include "Util.h"
#include "FeatureStats.h"
#include "FeatureArray.h"
#include "FeatureData.h"

int main (int argc, char * argv[]) {

	Timer timer;
	timer.start("Starting...");

	string nbestfile=argv[1];
	string outputfile=argv[2];

	TRACE_ERR("USAGE: feature_extractor nbestfile outputfile" << std::endl);

	TRACE_ERR("nbestfile: " << nbestfile << std::endl);
	TRACE_ERR("outputfile: " << outputfile << std::endl);

	FeatureData data;
	data.loadnbest(nbestfile);
	data.savetxt(outputfile);

	timer.stop("Stopping...");
	return 0;
}
