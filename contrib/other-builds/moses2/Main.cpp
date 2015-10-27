#include <iostream>
#include "System.h"
#include "Manager.h"
#include "Phrase.h"
#include "moses/InputFileStream.h"
#include "moses/Parameter.h"

using namespace std;

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
		bestHypo->OutputToStream(cout);
		cout << endl;
	}

	if (inStream != cin) {
		delete &inStream;
	}

	cerr << "Finished" << endl;
}
