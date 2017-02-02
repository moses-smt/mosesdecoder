#include "Util.h"
#include "Classifier.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/device/file.hpp>

using namespace std;
using namespace boost::algorithm;
using namespace Moses;

namespace Discriminative
{

VWTrainer::VWTrainer(const std::string &outputFile)
{
  if (ends_with(outputFile, ".gz")) {
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

FeatureType VWTrainer::AddLabelIndependentFeature(const StringPiece &name, float value)
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

  return std::make_pair(0, value); // we don't hash features
}

FeatureType VWTrainer::AddLabelDependentFeature(const StringPiece &name, float value)
{
  if (m_isFirstTarget) {
    m_isFirstTarget = false;
    if (! m_outputBuffer.empty())
      WriteBuffer();

    m_outputBuffer.push_back("|t");
  }

  AddFeature(name, value);

  return std::make_pair(0, value); // we don't hash features
}

void VWTrainer::AddLabelIndependentFeatureVector(const FeatureVector &features)
{
  throw logic_error("VW trainer does not support feature IDs.");
}

void VWTrainer::AddLabelDependentFeatureVector(const FeatureVector &features)
{
  throw logic_error("VW trainer does not support feature IDs.");
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

} // namespace Discriminative
