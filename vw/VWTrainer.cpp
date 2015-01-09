#include "Util.h"
#include "Classifier.h"
#include <boost/iostreams/device/file.hpp>

using namespace std;
using namespace Moses;

namespace Discriminative
{

VWTrainer::VWTrainer(const std::string &outputFile)
{
  if (outputFile.size() > 3 && outputFile.substr(outputFile.size() - 3, 3) == ".gz") {
    m_bfos.push(boost::iostreams::gzip_compressor());
  }
  m_bfos.push(boost::iostreams::file_sink(outputFile));
  m_isFirstSource = m_isFirstTarget = m_isFirstExample = true;
}

VWTrainer::~VWTrainer()
{
  m_bfos << "\n";
  close(m_bfos);
}

void VWTrainer::AddLabelIndependentFeature(const StringPiece &name, float value)
{
  if (m_isFirstSource) {
    if (m_isFirstExample) {
      m_isFirstExample = false;
    } else {
      // finish previous example
      m_bfos << "\n";
    }

    m_isFirstSource = false;
    if (! m_outputBuffer.empty())
      WriteBuffer();

    m_outputBuffer.push_back("shared |s");
  }

  AddFeature(name, value);
}

void VWTrainer::AddLabelDependentFeature(const StringPiece &name, float value)
{
  if (m_isFirstTarget) {
    m_isFirstTarget = false;
    if (! m_outputBuffer.empty())
      WriteBuffer();

    m_outputBuffer.push_back("|t");
  }

  AddFeature(name, value);
}

void VWTrainer::Train(const StringPiece &label, float loss)
{
  m_outputBuffer.push_front(label.as_string() + ":" + SPrint(loss));
  m_isFirstSource = true;
  m_isFirstTarget = true;
  WriteBuffer();
}

float VWTrainer::Predict(const StringPiece &label)
{
  throw logic_error("Trying to predict during training!");
}

void VWTrainer::AddFeature(const StringPiece &name, float value)
{
  m_outputBuffer.push_back(EscapeSpecialChars(name.as_string()) + ":" + SPrint(value));
}

void VWTrainer::WriteBuffer()
{
  m_bfos << Join(" ", m_outputBuffer.begin(), m_outputBuffer.end()) << "\n";
  m_outputBuffer.clear();
}

std::string VWTrainer::EscapeSpecialChars(const std::string &str)
{
  string out;
  out = Replace(str, "|", "_PIPE_");
  out = Replace(out, ":", "_COLON_");
  out = Replace(out, " ", "_");
  return out;
}

} // namespace Discriminative
