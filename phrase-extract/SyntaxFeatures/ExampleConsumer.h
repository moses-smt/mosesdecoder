#ifndef moses_EXAMPLECONSUMER_H_
#define moses_EXAMPLECONSUMER_H_

#include "PsdLine.h"
#include "RuleTable.h"
#include "SynchronizedInput.h"
#include "InputFileStream.h"
#include "CorpusLine.h"
#include "InputTreeRep.h"
#include <boost/thread.hpp>

using namespace Moses;
using namespace PSD;
using namespace std;

namespace Moses
{

// Class that consumes objects from a queue
class ExampleConsumer
{

public:
// Constructor with id and the queue to use.
ExampleConsumer(int id, SynchronizedInput<std::deque<PSDLine> >* queue, RuleTable* ruleTable, ExtractorConfig* configFile, CorpusLine* corpus, CorpusLine* parse, SynchronizedInput<std::string>* writeQueue);

// The thread function reads data from the queue
void operator () ();

ContextType ReadFactoredLine(const std::string &line, size_t factorCount);

size_t GetSizeOfSentence(const std::string &line);

private:
int m_id; // The id of the consumer
SynchronizedInput<std::deque<PSDLine> >* m_queue; // Thread-safe queue containing psd lines that are retrieved by consumer
RuleTable* m_ruleTable; // Not thread safe rule-table that is only read
VWBufferTrainConsumer m_consumer;
CorpusLine* m_corpus_input;
CorpusLine* m_parse_input;
ExtractorConfig* m_config;
FeatureExtractor m_extractor;
};
}



#endif /* EXAMPLECONSUMER_H_ */
