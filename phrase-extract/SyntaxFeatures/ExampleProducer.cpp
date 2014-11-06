#include <iostream>
#include "ExampleProducer.h"

using namespace Moses;
using namespace std;

// Constructor with id and the queue to use
ExampleProducer::ExampleProducer(int id, SynchronizedInput<std::deque<PSDLine> >* queue, InputFileStream* psdInputStream)
: m_id(id),
  m_queue(queue),
  m_input(psdInputStream),
  m_readLines(0)
{}

void ExampleProducer::operator () ()
{
	std::cerr << "Producer constructing shared queue... " << std::endl;
	int data=0;
	string rawPSDLine;

	std::deque<PSDLine> sourcePhrases;

	//Process first line of psd file
	getline(*m_input, rawPSDLine);
	m_readLines++;
	PSDLine psdLine(rawPSDLine);
	sourcePhrases.push_back(psdLine);

	//examples with the same source side and the same span generate the same training example (in sourcePhrases)
	string srcPhrase = psdLine.GetSrcPhrase();
	int span = (psdLine.GetSrcEnd() - psdLine.GetSrcStart()) + 1;
	size_t sentId = psdLine.GetSentID();

	//read psd lines and put into queue
	while (getline(*m_input, rawPSDLine))
	{
		//read second line
		m_readLines++;
	    PSDLine psdLine(rawPSDLine);
	    //std::cerr << psdLine << std::endl;

	    int currentSpan = (psdLine.GetSrcEnd() - psdLine.GetSrcStart()) + 1;

	    //if we have a new source side, enqueue line already read
	    if (psdLine.GetSrcPhrase() != srcPhrase || currentSpan != span || psdLine.GetSentID() != sentId) {
	    	//put a COPY of the queue into shared queue
	    	m_queue->Enqueue(sourcePhrases);
	    	std::cerr << "LINES : " << sourcePhrases.size() << "FLUSHED" << std::endl;
	    	sourcePhrases.clear();
	    }
	    //enque next line and set new source phrase
	    sourcePhrases.push_back(psdLine);
	    //std::cerr << "LINE IN sourcePhrases" << std::endl;
	    std::cerr << "PRODUCED PSD LINE : " << psdLine.GetSentID() << " " << psdLine.GetSrcPhrase() << " " << psdLine.GetTgtPhrase() << std::endl;

	    //restore source phrase, span and sentenceId
	    srcPhrase = psdLine.GetSrcPhrase();
	    span = (psdLine.GetSrcEnd() - psdLine.GetSrcStart()) + 1;
	    sentId = psdLine.GetSentID();
	}
	//enqueue last lines
	m_queue->Enqueue(sourcePhrases);
	std::cerr << "LINES : " << sourcePhrases.size() << "FLUSHED BEFORE EXIT" << std::endl;
	sourcePhrases.clear();
}

