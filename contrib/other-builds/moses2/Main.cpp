#include <iostream>
#include "System.h"
#include "Phrase.h"
#include "Search/Manager.h"
#include "moses/InputFileStream.h"
#include "moses/Parameter.h"

using namespace std;

extern size_t g_numHypos;

istream &GetInputStream(Moses::Parameter &params)
{
	const Moses::PARAM_VEC *vec = params.GetParam("input-file");
	if (vec) {
		Moses::InputFileStream *stream = new Moses::InputFileStream(vec->at(0));
		return *stream;
	}
	else {
		return cin;
	}
}

int main(int argc, char** argv)
{
	cerr << "Starting..." << endl;

	Moses::Parameter params;
	params.LoadParam(argc, argv);
	System system(params);

	istream &inStream = GetInputStream(params);

	string line;
	while (getline(inStream, line)) {

		Manager mgr(system, line);
		mgr.Decode();

		const Hypothesis *bestHypo = mgr.GetBestHypothesis();
		if (bestHypo) {
			bestHypo->OutputToStream(cout);
			cerr << "BEST TRANSLATION: " << *bestHypo;
		}
		else {
			cerr << "NO TRANSLATION";
		}
		cout << endl;
		cerr << endl;
	}

	if (inStream != cin) {
		delete &inStream;
	}

	cerr << "g_numHypos=" << g_numHypos << endl;
	cerr << "Finished" << endl;
}
