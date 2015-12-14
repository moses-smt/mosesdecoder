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

istream &GetInputStream(Moses2::Parameter &params)
{
	const Moses2::PARAM_VEC *vec = params.GetParam("input-file");
	if (vec && vec->size()) {
		Moses2::InputFileStream *stream = new Moses2::InputFileStream(vec->at(0));
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

	Moses2::Parameter params;
	params.LoadParam(argc, argv);
	Moses2::System system(params);

	istream &inStream = GetInputStream(params);

	cerr << "system.numThreads=" << system.numThreads << endl;
	Moses2::Timer timer;
	timer.start();

	Moses2::ThreadPool pool(system.numThreads);

	long translationId = 0;
	string line;
	while (getline(inStream, line)) {
	    boost::shared_ptr<Moses2::TranslationTask> task(new Moses2::TranslationTask(system, line, translationId));

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
