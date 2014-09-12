#ifndef moses_EXAMPLEPRODUCER_H_
#define moses_EXAMPLEPRODUCER_H_

#include "PsdLine.h"
#include "SynchronizedInput.h"
#include "InputFileStream.h"
#include <boost/thread.hpp>

using namespace Moses;
using namespace std;

namespace Moses {

// Class that produces objects and puts them in a queue
class ExampleProducer
{

public:

// Constructor with id, the queue to use and input file to read in queue. We assume that there is only one producer
ExampleProducer(int id, SynchronizedInput<std::deque<PSDLine> >* queue, InputFileStream* psdInputStream);

// The thread function fills the queue with data
void operator () ();

//return number of lines in psd file
int GetReadLines(){return m_readLines;};

private:
int m_id; // The id of the producer
SynchronizedInput<std::deque<PSDLine> >* m_queue; // The queue to use
InputFileStream* m_input;
int m_readLines;

};
}

#endif /* EXAMPLEPRODUCER_H_ */
