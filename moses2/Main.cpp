#include <iostream>
#include <memory>
#include <boost/pool/pool_alloc.hpp>
#include "Main.h"
#include "System.h"
#include "Phrase.h"
#include "TranslationTask.h"
#include "MemPoolAllocator.h"
#include "server/Server.h"
#include "legacy/InputFileStream.h"
#include "legacy/Parameter.h"
#include "legacy/ThreadPool.h"
#include "legacy/Timer.h"
#include "legacy/Util2.h"
#include "util/usage.hh"

using namespace std;

//extern size_t g_numHypos;

int main(int argc, char** argv)
{
  cerr << "Starting..." << endl;

  Moses2::Timer timer;
  timer.start();
  //Temp();

  Moses2::Parameter params;
  if (!params.LoadParam(argc, argv)) {
    return EXIT_FAILURE;
  }
  Moses2::System system(params);
  timer.check("Loaded");

  if (params.GetParam("show-weights")) {
    return EXIT_SUCCESS;
  }

  //cerr << "system.numThreads=" << system.options.server.numThreads << endl;

  Moses2::ThreadPool pool(system.options.server.numThreads, system.cpuAffinityOffset, system.cpuAffinityOffsetIncr);
  //cerr << "CREATED POOL" << endl;

  if (params.GetParam("server")) {
    std::cerr << "RUN SERVER" << std::endl;
    run_as_server(system);
  } else {
    std::cerr << "RUN BATCH" << std::endl;
    batch_run(params, system, pool);
  }

  cerr << "Decoding took " << timer.get_elapsed_time() << endl;
  //	cerr << "g_numHypos=" << g_numHypos << endl;
  cerr << "Finished" << endl;
  return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////
void run_as_server(Moses2::System &system)
{
  Moses2::Server server(system.options.server, system);
  server.run(system); // actually: don't return. see Server::run()
}

////////////////////////////////////////////////////////////////////////////////////////////////
istream &GetInputStream(Moses2::Parameter &params)
{
  const Moses2::PARAM_VEC *vec = params.GetParam("input-file");
  if (vec && vec->size()) {
    Moses2::InputFileStream *stream = new Moses2::InputFileStream(vec->at(0));
    return *stream;
  } else {
    return cin;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////
void batch_run(Moses2::Parameter &params, Moses2::System &system, Moses2::ThreadPool &pool)
{
  istream &inStream = GetInputStream(params);

  long translationId = 0;
  string line;
  while (getline(inStream, line)) {
    //cerr << "line=" << line << endl;
    boost::shared_ptr<Moses2::TranslationTask> task(new Moses2::TranslationTask(system, line, translationId));

    //cerr << "START pool.Submit()" << endl;
    pool.Submit(task);
    //task->Run();
    ++translationId;
  }

  pool.Stop(true);

  if (&inStream != &cin) {
    delete &inStream;
  }

  //util::PrintUsage(std::cerr);

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
