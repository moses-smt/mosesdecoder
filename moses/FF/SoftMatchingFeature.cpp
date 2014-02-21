#include "SoftMatchingFeature.h"
#include "moses/AlignmentInfo.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartHypothesis.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"

namespace Moses
{

SoftMatchingFeature::SoftMatchingFeature(const std::string &line)
  : StatelessFeatureFunction(0, line)
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

      m_soft_matches[LHS].insert(RHS);
      m_soft_matches_reverse[RHS].insert(LHS);
    }

    staticData.Set_Soft_Matches(Get_Soft_Matches());
    staticData.Set_Soft_Matches_Reverse(Get_Soft_Matches_Reverse());

   return true;
}

void SoftMatchingFeature::EvaluateChart(const ChartHypothesis& hypo,
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

      const std::string name = GetFeatureName(prevLHS, word);
      accumulator->PlusEquals(this,name,1);
    }
  }
}

//caching feature names because string conversion is slow
const std::string& SoftMatchingFeature::GetFeatureName(const Word& LHS, const Word& RHS) const
{

  const NonTerminalMapKey key(LHS, RHS);
  {
#ifdef WITH_THREADS //try read-only lock
  boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif // WITH_THREADS
  NonTerminalSoftMatchingMap::const_iterator i = m_soft_matching_cache.find(key);
  if (i != m_soft_matching_cache.end()) return i->second;
  }
#ifdef WITH_THREADS //need to update cache; write lock
  boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif // WITH_THREADS
  const std::vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();

  std::string LHS_string = LHS.GetString(outputFactorOrder, false);
  std::string RHS_string = RHS.GetString(outputFactorOrder, false);

  const std::string name = LHS_string + "->" + RHS_string;

  m_soft_matching_cache[key] = name;
  return m_soft_matching_cache.find(key)->second;
}

}

