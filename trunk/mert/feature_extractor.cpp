
using namespace std;

#include <iostream>
#include "Parameter.h"
#include "Timer.h"
#include "Util.h"
#include "FeatureStats.h"
#include "FeatureArray.h"
#include "FeatureData.h"

int main (int argc, char * argv[]) {

	bool binmode=false;

	Timer timer;
	timer.start("Starting...");

	string inputfile=argv[1];
	string outputfile=argv[2];

// to merge statistics of new nbest lists with already existing statistics
// set statsfile to them
	string statsfile;
	if (argc > 3){
		statsfile=argv[3];
	}

	TRACE_ERR("USAGE: feature_extractor inputfile outputfile" << std::endl);

	TRACE_ERR("inputfile: " << inputfile << std::endl);
	TRACE_ERR("outputfile: " << outputfile << std::endl);

	FeatureData data;
	if (!statsfile.empty())	data.load(statsfile);

	std::ifstream inFile(inputfile.c_str(), std::ios::out); // matches a stream with a file. Opens the file

	std::string stringBuf;
       	std::string::size_type loc;

	std::getline(inFile, stringBuf);
	if (stringBuf.empty()){
		TRACE_ERR("inputfile: " << inputfile << " is empty" << std::endl);
		return 0;
	}
	inFile.close();

	if ((loc = stringBuf.find(FEATURES_TXT_BEGIN)) == 0 || (loc = stringBuf.find(FEATURES_BIN_BEGIN)) == 0){
		TRACE_ERR("inputfile: " << inputfile << " contains statistics" << std::endl);
		data.load(inputfile);
	}
	else{
		TRACE_ERR("inputfile: " << inputfile << " does not contain statistics; assume it contains nbest list." << std::endl);
		data.loadnbest(inputfile);
	}

	data.save(outputfile, binmode);

	timer.stop("Stopping...");
	return 0;
}
