#include <iostream>
#include <memory>
#include "System.h"
#include "Phrase.h"
#include "TranslationTask.h"
#include "Search/Manager.h"
#include "legacy/InputFileStream.h"
#include "legacy/Parameter.h"
#include "legacy/ThreadPool.h"
#include "legacy/Timer.h"

using namespace std;

//extern size_t g_numHypos;

istream &GetInputStream(Parameter &params)
{
	const PARAM_VEC *vec = params.GetParam("input-file");
	if (vec && vec->size()) {
		InputFileStream *stream = new InputFileStream(vec->at(0));
		return *stream;
	}
	else {
		return cin;
	}
}

void Temp();

int main(int argc, char** argv)
{
	cerr << "Starting..." << endl;

	//Temp();

	Parameter params;
	params.LoadParam(argc, argv);
	System system(params);

	istream &inStream = GetInputStream(params);

	cerr << "system.numThreads=" << system.numThreads << endl;
	Moses2::Timer timer;
	timer.start();

	ThreadPool pool(system.numThreads);

	string line;
	while (getline(inStream, line)) {
	    boost::shared_ptr<TranslationTask> task(new TranslationTask(system, line));

		pool.Submit(task);
		//task->Run();
	}

	pool.Stop(true);

	if (&inStream != &cin) {
		delete &inStream;
	}


	cerr << "Decoding took " << timer.get_elapsed_time() << endl;
//	cerr << "g_numHypos=" << g_numHypos << endl;
	cerr << "Finished" << endl;
}

////////////////////////////////////////////////////////////////////////////////////////////////


void Temp()
{
	vector<int> v;
	v.push_back(33);

}
