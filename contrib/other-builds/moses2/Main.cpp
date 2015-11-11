#include <iostream>
#include "System.h"
#include "Phrase.h"
#include "TranslationTask.h"
#include "Search/Manager.h"
#include "moses/InputFileStream.h"
#include "legacy/Parameter.h"
#include "moses/ThreadPool.h"

using namespace std;

//extern size_t g_numHypos;

istream &GetInputStream(Parameter &params)
{
	const PARAM_VEC *vec = params.GetParam("input-file");
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

	Parameter params;
	params.LoadParam(argc, argv);
	System system(params);

	istream &inStream = GetInputStream(params);

	cerr << "system.numThreads=" << system.numThreads << endl;

	Moses::ThreadPool pool(system.numThreads);

	string line;
	while (getline(inStream, line)) {
	    boost::shared_ptr<TranslationTask> task(new TranslationTask(system, line));

		pool.Submit(task);
		//task->Run();
	}

	pool.Stop(true);

	if (inStream != cin) {
		delete &inStream;
	}

//	cerr << "g_numHypos=" << g_numHypos << endl;
	cerr << "Finished" << endl;
}
