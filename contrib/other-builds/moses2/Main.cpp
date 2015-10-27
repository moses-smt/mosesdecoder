#include <iostream>
#include "System.h"
#include "Manager.h"
#include "Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/Parameter.h"

using namespace std;

int main(int argc, char** argv)
{
	cerr << "Starting..." << endl;

	Moses::Parameter params;
	params.LoadParam(argc, argv);
	System system(params);

	string line;
	while (getline(cin, line)) {

		Manager mgr(system, line);
		mgr.Decode();

		const Hypothesis *bestHypo = mgr.GetBestHypothesis();
		bestHypo->OutputToStream(cout);
		cout << endl;
	}

	cerr << "Finished" << endl;
}
