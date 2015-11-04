#include "RulePairUnlexicalizedSource.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/FactorCollection.h"
#include <sstream>
#include "util/string_stream.hh"

using namespace std;

namespace Moses
{

RulePairUnlexicalizedSource::RulePairUnlexicalizedSource(const std::string &line)
  : StatelessFeatureFunction(1, line)
  , m_glueRules(false)
  , m_nonGlueRules(true)
  , m_glueTargetLHSStr("Q")
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  FactorCollection &factorCollection = FactorCollection::Instance();
  m_glueTargetLHS = factorCollection.AddFactor(m_glueTargetLHSStr, true);
  VERBOSE(1, " Done.");
}

void RulePairUnlexicalizedSource::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "glueRules") {
    m_glueRules = Scan<bool>(value);
  } else if (key == "nonGlueRules") {
    m_nonGlueRules = Scan<bool>(value);
  } else if (key == "glueTargetLHS") {
    m_glueTargetLHSStr = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


void RulePairUnlexicalizedSource::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{
  const Factor* targetPhraseLHS = targetPhrase.GetTargetLHS()[0];
  if ( !m_glueRules && (targetPhraseLHS == m_glueTargetLHS) ) {
    return;
  }
  if ( !m_nonGlueRules && (targetPhraseLHS != m_glueTargetLHS) ) {
    return;
  }

  for (size_t posS=0; posS<source.GetSize(); ++posS) {
    const Word &wordS = source.GetWord(posS);
    if ( !wordS.IsNonTerminal() ) {
      return;
    }
  }

  util::StringStream namestr;

  for (size_t posT=0; posT<targetPhrase.GetSize(); ++posT) {
    const Word &wordT = targetPhrase.GetWord(posT);
    const Factor* factorT = wordT[0];
    if ( wordT.IsNonTerminal() ) {
      namestr << "[";
    }
    namestr << factorT->GetString();
    if ( wordT.IsNonTerminal() ) {
      namestr << "]";
    }
    namestr << "|";
  }

  namestr << targetPhraseLHS->GetString() << "|";

  for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignNonTerm().begin();
       it!=targetPhrase.GetAlignNonTerm().end(); ++it) {
    namestr << "|" << it->first << "-" << it->second;
  }

  scoreBreakdown.PlusEquals(this, namestr.str(), 1);
  if ( targetPhraseLHS != m_glueTargetLHS ) {
    scoreBreakdown.PlusEquals(this, 1);
  }
}

}

