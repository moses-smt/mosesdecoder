#include <iostream>
#include <memory>
#include <boost/pool/pool_alloc.hpp>
#include "System.h"
#include "Phrase.h"
#include "TranslationTask.h"
#include "MemPool.h"
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

	Moses2::ThreadPool pool(system.numThreads, system.cpuAffinityOffset, system.cpuAffinityOffsetIncr);

	long translationId = 0;
	string line;
	while (getline(inStream, line)) {
	    boost::shared_ptr<Moses2::TranslationTask> task(new Moses2::TranslationTask(system, line, translationId));

		pool.Submit(task);
		//task->Run();
		++translationId;
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
	Moses2::MemPool pool;
	Moses2::MemPoolAllocator<int> a(pool);

	boost::unordered_set<int, boost::hash<int>, std::equal_to<int>, Moses2::MemPoolAllocator<int> > s(a);
	s.insert(3);
	s.insert(4);
	s.insert(3);
	s.erase(3);

	boost::pool_allocator<int> alloc;
	std::vector<int, boost::pool_allocator<int> > v(alloc);
	  for (int i = 0; i < 1000; ++i)
	    v.push_back(i);

	  v.clear();
	  boost::singleton_pool<boost::pool_allocator_tag, sizeof(int)>::
	    purge_memory();

	abort();
}
