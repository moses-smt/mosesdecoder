#include <iostream>
#include "System.h"
#include "Manager.h"
#include "Phrase.h"
#include "moses/InputFileStream.h"

using namespace std;

int main(int argc, char** argv)
{
	cerr << "Starting..." << endl;

	System system;

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
