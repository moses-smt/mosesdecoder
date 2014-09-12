#include "ExampleWriter.h"

ExampleWriter::ExampleWriter(int id, SynchronizedInput<std::string>* queue, boost::iostreams::filtering_ostream* bfos):
	m_id(id),
	m_queue(queue),
	m_bfos(bfos)
{}

// The thread function fills the queue with data
void ExampleWriter::operator () ()
{
	std::cerr << "Writer writing queue to file..." << std::endl;
	while(true)
	{
		(*m_bfos) << m_queue->Dequeue();
		(*m_bfos) << endl;
	}
}
