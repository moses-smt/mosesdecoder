#include "FeatureConsumer.h"
#include "Util.h"
#include <stdexcept>
#include <exception>
#include <string>
#include <boost/iostreams/device/file.hpp>

using namespace std;
using namespace Moses;

namespace PSD
{

VWFileTrainConsumer::VWFileTrainConsumer(const std::string &outputFile)
{
  if (outputFile.size() > 3 && outputFile.substr(outputFile.size() - 3, 3) == ".gz") {
    m_bfos.push(boost::iostreams::gzip_compressor());
  }
	m_bfos.push(boost::iostreams::file_sink(outputFile));
}

void VWFileTrainConsumer::SetNamespace(char ns, bool shared)
{
  if (! m_outputBuffer.empty())
    WriteBuffer();

  if (shared)
    m_outputBuffer.push_back("shared");

  m_outputBuffer.push_back("|" + SPrint(ns));
}

void VWFileTrainConsumer::AddFeature(const std::string &name)
{
  m_outputBuffer.push_back(EscapeSpecialChars(name));
  // cerr << "Written feature: " << EscapeSpecialChars(name) << "\n";
}

void VWFileTrainConsumer::AddFeature(const std::string &name, float value)
{
  m_outputBuffer.push_back(EscapeSpecialChars(name) + ":" + SPrint(value));
}

void VWFileTrainConsumer::FinishExample()
{
  WriteBuffer();
  m_bfos << "\n";
}

void VWFileTrainConsumer::Finish()
{
  //m_os.close();
	close(m_bfos);
}

void VWFileTrainConsumer::Train(const std::string &label, float loss)
{
  m_outputBuffer.push_front(label + ":" + SPrint(loss));
}

float VWFileTrainConsumer::Predict(const std::string &label) 
{
  throw logic_error("Trying to predict during training!");
}

//
// private methods
//

void VWFileTrainConsumer::WriteBuffer()
{
  m_bfos << Join(" ", m_outputBuffer.begin(), m_outputBuffer.end()) << "\n";
  m_outputBuffer.clear();
}

} // namespace PSD
