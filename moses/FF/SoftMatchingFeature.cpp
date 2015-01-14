#include "SoftMatchingFeature.h"
#include "moses/AlignmentInfo.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartHypothesis.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/FactorCollection.h"
#include "moses/Util.h"

namespace Moses
{

SoftMatchingFeature::SoftMatchingFeature(const std::string &line)
  : StatelessFeatureFunction(0, line)
  , m_softMatches(moses_MaxNumNonterminals)
{
  ReadParameters();
}

void SoftMatchingFeature::SetParameter(const std::string& key, const std::string& value)
{
  std::cerr << "setting: " << this->GetScoreProducerDescription() << " - " << key << "\n";
  if (key == "tuneable") {
    m_tuneable = Scan<bool>(value);
  } else if (key == "filterable") { //ignore
  } else if (key == "path") {
    const std::string filePath = value;
    Load(filePath);
  } else {
    UTIL_THROW(util::Exception, "Unknown argument " << key << "=" << value);
  }
}


bool SoftMatchingFeature::Load(const std::string& filePath)
{

  StaticData &staticData = StaticData::InstanceNonConst();

  InputFileStream inStream(filePath);
  std::string line;
  while(getline(inStream, line)) {
    std::vector<std::string> tokens = Tokenize(line);
    UTIL_THROW_IF2(tokens.size() != 2, "Error: wrong format of SoftMatching file: must have two nonterminals per line");

    // no soft matching necessary if LHS and RHS are the same
    if (tokens[0] == tokens[1]) {
      continue;
    }

    Word LHS, RHS;
    LHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), tokens[0], true);
    RHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), tokens[1], true);

    m_softMatches[RHS[0]->GetId()].push_back(LHS);
    GetOrSetFeatureName(RHS, LHS);
  }

  staticData.SetSoftMatches(m_softMatches);

  return true;
}

void SoftMatchingFeature::EvaluateWhenApplied(const ChartHypothesis& hypo,
    ScoreComponentCollection* accumulator) const
{

  const TargetPhrase& target = hypo.GetCurrTargetPhrase();

  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap = target.GetAlignNonTerm().GetNonTermIndexMap();

  // loop over the rule that is being applied
  for (size_t pos = 0; pos < target.GetSize(); ++pos) {
    const Word& word = target.GetWord(pos);

    // for non-terminals, trigger the feature mapping the LHS of the previous hypo to the RHS of this hypo
    if (word.IsNonTerminal()) {
      size_t nonTermInd = nonTermIndexMap[pos];

      const ChartHypothesis* prevHypo = hypo.GetPrevHypo(nonTermInd);
      const Word& prevLHS = prevHypo->GetTargetLHS();

      const std::string &name = GetOrSetFeatureName(word, prevLHS);
      accumulator->PlusEquals(this,name,1);
    }
  }
}

// when loading, or when we notice that non-terminals have been added after loading, we resize vectors
void SoftMatchingFeature::ResizeCache() const
{
  FactorCollection& fc = FactorCollection::Instance();
  size_t numNonTerminals = fc.GetNumNonTerminals();

  m_nameCache.resize(numNonTerminals);
  for (size_t i = 0; i < numNonTerminals; i++) {
    m_nameCache[i].resize(numNonTerminals);
  }
}


const std::string& SoftMatchingFeature::GetOrSetFeatureName(const Word& RHS, const Word& LHS) const
{
  try {
#ifdef WITH_THREADS //try read-only lock
    boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
    const std::string &name = m_nameCache.at(RHS[0]->GetId()).at(LHS[0]->GetId());
    if (!name.empty()) {
      return name;
    }
  } catch (const std::out_of_range& oor) {
#ifdef WITH_THREADS //need to resize cache; write lock
    boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
    ResizeCache();
  }
#ifdef WITH_THREADS //need to update cache; write lock
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
  std::string &name = m_nameCache[RHS[0]->GetId()][LHS[0]->GetId()];
  const std::vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
  std::string LHS_string = LHS.GetString(outputFactorOrder, false);
  std::string RHS_string = RHS.GetString(outputFactorOrder, false);
  name = LHS_string + "->" + RHS_string;
  return name;
}

}

