#ifndef moses_EXAMPLEWRITER_H_
#define moses_EXAMPLEWRITER_H_

#include "PsdLine.h"
#include "SynchronizedInput.h"
#include <boost/thread.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace Moses;
using namespace std;

namespace Moses {

// Class that takes objects from a queue and writes them in a file
class ExampleWriter
{

public:

// Constructor with id, the queue to use and input file to read in queue. We assume that there is only one producer
ExampleWriter(int id, SynchronizedInput<std::string>* queue, boost::iostreams::filtering_ostream* bfos);

// The thread function fills the queue with data
void operator () ();

private:
int m_id; // The id of the producer
SynchronizedInput<std::string>* m_queue; // The queue to use
boost::iostreams::filtering_ostream* m_bfos;

};
}

#endif /* EXAMPLEWRITER_H_ */
