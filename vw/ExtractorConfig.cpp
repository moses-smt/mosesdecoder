#include "ExtractorConfig.h"
#include "Util.h"

#include <exception>
#include <stdexcept>
#include <algorithm>
#include <set>

using namespace std;
using namespace boost::bimaps;
using namespace Moses;

namespace Classifier
{

void ExtractorConfig::Load(const string &configFile)
{
  try {
    IniReader reader(configFile);
    m_sourceInternal  = reader.Get<bool>("features.source-internal", false);
    m_sourceExternal  = reader.Get<bool>("features.source-external", false);
    m_targetInternal  = reader.Get<bool>("features.target-internal", false);
    m_sourceIndicator = reader.Get<bool>("features.source-indicator", false);
    m_targetIndicator = reader.Get<bool>("features.target-indicator", false);
    m_sourceTargetIndicator = reader.Get<bool>("features.source-target-indicator", false);
    m_STSE = reader.Get<bool>("features.source-target-source-external", false);
    m_paired          = reader.Get<bool>("features.paired", false);
    m_bagOfWords      = reader.Get<bool>("features.bag-of-words", false);
    m_mostFrequent    = reader.Get<bool>("features.most-frequent", false);
    m_binnedScores    = reader.Get<bool>("features.binned-scores", false);
    m_sourceTopic     = reader.Get<bool>("features.source-topic", false);
    m_phraseFactor    = reader.Get<bool>("features.phrase-factor", false);
    m_windowSize      = reader.Get<size_t>("features.window-size", 0);  

    m_factors = Scan<size_t>(Tokenize(reader.Get<string>("features.factors", ""), ","));
    m_scoreIndexes = Scan<size_t>(Tokenize(reader.Get<string>("features.scores", ""), ","));
    m_scoreBins = Scan<float>(Tokenize(reader.Get<string>("features.score-bins", ""), ","));

    m_vwOptsTrain = reader.Get<string>("vw-options.train", "");
    m_vwOptsPredict = reader.Get<string>("vw-options.predict", "");

    m_normalization = reader.Get<string>("decoder.normalization", "");

    m_isLoaded = true;
  } catch (const runtime_error &err) {
    cerr << "Error loading file " << configFile << ": " << err.what();
    m_isLoaded = false;
  }
}

} // namespace Classifier
