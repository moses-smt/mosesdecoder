#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "moses/Util.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "psd/FeatureExtractor.h"
#include "psd/FeatureConsumer.h"
#include "RuleTable.h"
#include "InputTreeRep.h"
#include "PsdLine.h"
#include "CorpusLine.h"
#include "SynchronizedInput.h"
#include "ExampleConsumer.h"
#include "ExampleProducer.h"
#include "ExampleWriter.h"

using namespace std;
using namespace Moses;
using namespace PSD;

void WritePhraseIndex(const TargetIndexType &index, const string &outFile)
{
  OutputFileStream out(outFile);
  if (! out.good()) {
    cerr << "error: Failed to open " << outFile << endl;
    CHECK(false);
  }
  TargetIndexType::right_map::const_iterator it; // keys are sorted in the map
  for (it = index.right.begin(); it != index.right.end(); it++)
    out << it->second << "\n";
  out.Close();
}

inline int makeSpanInterval(int span)
{
    switch(span)
    {
        case 1: return 1;
        case 2: return 2;
        case 3: return 3;
        case 4: return 4;
        case 5: return 4;
        case 6: return 4;
        case 7: return 7;
        case 8: return 7;
        case 9: return 7;
        case 10: return 7;
        default: return 8;
    }

}

int main(int argc, char**argv)
{

  std::cerr << "Beginning Extraction of Syntactic Features for LHS of rules..." << std::endl;

  if (argc != 9) {
    cerr << "error: wrong arguments" << endl;
    cerr << "Usage: extract-psd psd-file parsed-file phrase-table extractor-config output-train output-index nbr-threads" << endl;
    exit(1);
  }

  //load psd file
  InputFileStream psdFile(argv[1]);
  if (! psdFile.good()) {
      cerr << "error: Failed to open " << argv[1] << endl;
      exit(1);
    }

  CorpusLine corpus(argv[2]);
  CorpusLine parse(argv[3]);

  //Load rule table. All threads read from same rule-table
  std::cout << "Loading rule table..." << std::endl;
  RuleTable rtable(argv[4]);
  std::cout << "Rule table loaded" << std::endl;

  //Load config file
  ExtractorConfig config;
  config.Load(argv[5]);

  const string outputFileName = argv[6];
  boost::iostreams::filtering_ostream outputFile;

  //Open output stream
  if (outputFileName.size() > 3 && outputFileName.substr(outputFileName.size() - 3, 3) == ".gz") {
	  outputFile.push(boost::iostreams::gzip_compressor());
   	  }
  	  outputFile.push(boost::iostreams::file_sink(outputFileName));

  //Write index file
  WritePhraseIndex(rtable.GetTargetIndex(), argv[7]);

  size_t nrThreads = Moses::Scan<size_t>(argv[8]);

  //we need at least 2 threads : 1 producer and 1 consumer
  if (nrThreads < 3)
  {
	  std::cerr << "Feature extraction needs at least 3 threads" << std::endl;
	  exit(1);
  }

  //All threads to create training examples except producer and writer
  int nrConsumers = nrThreads - 2;

  // The shared queue
  SynchronizedInput<std::deque<PSDLine> > exampleQueue;
  SynchronizedInput<std::string> writeQueue;

  // Ask the number of producers
  cout<<"Beginning threading : number of threads : " << nrThreads << endl;

  // Create producers : we use only 1 producer but this number can be increased
  boost::thread_group producers;
  int producer_id = 1;
  ExampleProducer p(producer_id, &exampleQueue, &psdFile);
  producers.create_thread(p);

  // Create consumers
  boost::thread_group consumers;
  for (int i=0; i<nrConsumers; i++)
  {
	ExampleConsumer c(i, &exampleQueue, &rtable, &config , &corpus, &parse, &writeQueue);
	consumers.create_thread(c);
 }

  // Create writers : we use only 1 producer but this number can be increased
  boost::thread_group writers;
  int writer_id = 1;
  ExampleWriter w(writer_id, &writeQueue, &outputFile);
  writers.create_thread(w);

  // flush FeatureConsumer
  producers.interrupt_all(); producers.join_all();
  consumers.interrupt_all(); consumers.join_all();
  writers.interrupt_all(); writers.join_all();

  close(outputFile);
}
