#include "FeatureConsumer.h"
#include "moses/Util.h"
#include <stdexcept>
#include <exception>
#include <string>
#include <boost/iostreams/device/file.hpp>

using namespace std;
using namespace Moses;

namespace PSD
{

VWBufferTrainConsumer::VWBufferTrainConsumer(SynchronizedInput<string>* queue):
		m_queue(queue)
{}

void VWBufferTrainConsumer::SetNamespace(char ns, bool shared)
{
	if (!m_outputBuffer.empty())
    {WriteBuffer();}
	//m_outputBuffer.push_back("\n");

  if (shared)
    m_outputBuffer.push_back("shared");
  	m_outputBuffer.push_back("|" + SPrint(ns));
}

void VWBufferTrainConsumer::AddFeature(const std::string &name)
{
  m_outputBuffer.push_back(EscapeSpecialChars(name));
}

void VWBufferTrainConsumer::AddFeature(const std::string &name, float value)
{
  m_outputBuffer.push_back(EscapeSpecialChars(name) + ":" + SPrint(value));
}

//Thread-safe writing of example
void VWBufferTrainConsumer::FinishExample()
{
	if (!m_outputBuffer.empty())
	{WriteBuffer();}
	m_queue->Enqueue(Join("",m_tempBuffer.begin(), m_tempBuffer.end()));
	//cerr << "WRITER : PUT INTO QUEUE " << Join("",m_tempBuffer.begin(), m_tempBuffer.end()) << endl;
	m_tempBuffer.clear();
}

void VWBufferTrainConsumer::Finish()
{
}

void VWBufferTrainConsumer::Train(const std::string &label, float loss)
{
  m_outputBuffer.push_front(label + ":" + SPrint(loss));
}

float VWBufferTrainConsumer::Predict(const std::string &label)
{
  throw logic_error("Trying to predict during training!");
}

//
// private methods
//

void VWBufferTrainConsumer::WriteBuffer()
{
  string exampleLine = Join(" ", m_outputBuffer.begin(), m_outputBuffer.end());
  exampleLine += "\n";
  m_tempBuffer.push_back(exampleLine);
  //std::cerr << "WRITER : LINE PUT INTO TMP BUFFER" << exampleLine << std::endl;
  m_outputBuffer.clear();
  exampleLine.clear();
}


std::string VWBufferTrainConsumer::EscapeSpecialChars(const std::string &str)
{
  string out;
  out = Replace(str, "|", "_PIPE_");
  out = Replace(out, ":", "_COLON_");
  out = Replace(out, " ", "_");
  return out;
}

} // namespace PSD
