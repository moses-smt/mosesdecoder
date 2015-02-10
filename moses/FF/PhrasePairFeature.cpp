#include <boost/algorithm/string.hpp>

#include "PhrasePairFeature.h"
#include "moses/AlignmentInfo.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/TranslationOption.h"
#include "moses/InputPath.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

PhrasePairFeature::PhrasePairFeature(const std::string &line)
  :StatelessFeatureFunction(0, line)
{
  std::cerr << "Initializing PhrasePairFeature.." << std::endl;
  ReadParameters();

  if (m_simple == 1) std::cerr << "using simple phrase pairs.. ";
  if (m_sourceContext == 1) std::cerr << "using source context.. ";
  if (m_domainTrigger == 1) std::cerr << "using domain triggers.. ";

  // compile a list of punctuation characters
  if (m_ignorePunctuation) {
    std::cerr << "ignoring punctuation for triggers.. ";
    char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
    for (size_t i=0; i < sizeof(punctuation)-1; ++i)
      m_punctuationHash[punctuation[i]] = 1;
  }
}

void PhrasePairFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "input-factor") {
    m_sourceFactorId = Scan<FactorType>(value);
  } else if (key == "output-factor") {
    m_targetFactorId = Scan<FactorType>(value);
  } else if (key == "unrestricted") {
    m_unrestricted = Scan<bool>(value);
  } else if (key == "simple") {
    m_simple = Scan<bool>(value);
  } else if (key == "source-context") {
    m_sourceContext = Scan<bool>(value);
  } else if (key == "domain-trigger") {
    m_domainTrigger = Scan<bool>(value);
  } else if (key == "ignore-punctuation") {
    m_ignorePunctuation = Scan<bool>(value);
  } else if (key == "ignore-punctuation") {
    m_filePathSource = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void PhrasePairFeature::Load()
{
  if (m_domainTrigger) {
    // domain trigger terms for each input document
    ifstream inFileSource(m_filePathSource.c_str());
    UTIL_THROW_IF2(!inFileSource, "could not open file " << m_filePathSource);

    std::string line;
    while (getline(inFileSource, line)) {
      std::set<std::string> terms;
      vector<string> termVector;
      boost::split(termVector, line, boost::is_any_of("\t "));
      for (size_t i=0; i < termVector.size(); ++i)
        terms.insert(termVector[i]);

      // add term set for current document
      m_vocabDomain.push_back(terms);
    }

    inFileSource.close();
  } else {
    // restricted source word vocabulary
    ifstream inFileSource(m_filePathSource.c_str());
    UTIL_THROW_IF2(!inFileSource, "could not open file " << m_filePathSource);

    std::string line;
    while (getline(inFileSource, line)) {
      m_vocabSource.insert(line);
    }

    inFileSource.close();

    /*  // restricted target word vocabulary
    ifstream inFileTarget(filePathTarget.c_str());
    if (!inFileTarget)
    {
      cerr << "could not open file " << filePathTarget << endl;
      return false;
    }

    while (getline(inFileTarget, line)) {
    m_vocabTarget.insert(line);
    }

    inFileTarget.close();*/

    m_unrestricted = false;
  }
}

void PhrasePairFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  const Phrase& source = inputPath.GetPhrase();
  if (m_simple) {
    ostringstream namestr;
    namestr << "pp_";
    namestr << source.GetWord(0).GetFactor(m_sourceFactorId)->GetString();
    for (size_t i = 1; i < source.GetSize(); ++i) {
      const Factor* sourceFactor = source.GetWord(i).GetFactor(m_sourceFactorId);
      namestr << ",";
      namestr << sourceFactor->GetString();
    }
    namestr << "~";
    namestr << targetPhrase.GetWord(0).GetFactor(m_targetFactorId)->GetString();
    for (size_t i = 1; i < targetPhrase.GetSize(); ++i) {
      const Factor* targetFactor = targetPhrase.GetWord(i).GetFactor(m_targetFactorId);
      namestr << ",";
      namestr << targetFactor->GetString();
    }

    scoreBreakdown.SparsePlusEquals(namestr.str(),1);
  }
  if (m_domainTrigger) {
    const Sentence& input = static_cast<const Sentence&>(input);
    const bool use_topicid = input.GetUseTopicId();
    const bool use_topicid_prob = input.GetUseTopicIdAndProb();

    // compute pair
    ostringstream pair;
    pair << source.GetWord(0).GetFactor(m_sourceFactorId)->GetString();
    for (size_t i = 1; i < source.GetSize(); ++i) {
      const Factor* sourceFactor = source.GetWord(i).GetFactor(m_sourceFactorId);
      pair << ",";
      pair << sourceFactor->GetString();
    }
    pair << "~";
    pair << targetPhrase.GetWord(0).GetFactor(m_targetFactorId)->GetString();
    for (size_t i = 1; i < targetPhrase.GetSize(); ++i) {
      const Factor* targetFactor = targetPhrase.GetWord(i).GetFactor(m_targetFactorId);
      pair << ",";
      pair << targetFactor->GetString();
    }

    if (use_topicid || use_topicid_prob) {
      if(use_topicid) {
        // use topicid as trigger
        const long topicid = input.GetTopicId();
        stringstream feature;
        feature << "pp_";
        if (topicid == -1)
          feature << "unk";
        else
          feature << topicid;

        feature << "_";
        feature << pair.str();
        scoreBreakdown.SparsePlusEquals(feature.str(), 1);
      } else {
        // use topic probabilities
        const vector<string> &topicid_prob = *(input.GetTopicIdAndProb());
        if (atol(topicid_prob[0].c_str()) == -1) {
          stringstream feature;
          feature << "pp_unk_";
          feature << pair.str();
          scoreBreakdown.SparsePlusEquals(feature.str(), 1);
        } else {
          for (size_t i=0; i+1 < topicid_prob.size(); i+=2) {
            stringstream feature;
            feature << "pp_";
            feature << topicid_prob[i];
            feature << "_";
            feature << pair.str();
            scoreBreakdown.SparsePlusEquals(feature.str(), atof((topicid_prob[i+1]).c_str()));
          }
        }
      }
    } else {
      // range over domain trigger words
      const long docid = input.GetDocumentId();
      for (set<string>::const_iterator p = m_vocabDomain[docid].begin(); p != m_vocabDomain[docid].end(); ++p) {
        string sourceTrigger = *p;
        ostringstream namestr;
        namestr << "pp_";
        namestr << sourceTrigger;
        namestr << "_";
        namestr << pair.str();
        scoreBreakdown.SparsePlusEquals(namestr.str(),1);
      }
    }
  }
  if (m_sourceContext) {
    const Sentence& input = static_cast<const Sentence&>(input);

    // range over source words to get context
    for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
      StringPiece sourceTrigger = input.GetWord(contextIndex).GetFactor(m_sourceFactorId)->GetString();
      if (m_ignorePunctuation) {
        // check if trigger is punctuation
        char firstChar = sourceTrigger[0];
        CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
        if(charIterator != m_punctuationHash.end())
          continue;
      }

      bool sourceTriggerExists = false;
      if (!m_unrestricted)
        sourceTriggerExists = FindStringPiece(m_vocabSource, sourceTrigger ) != m_vocabSource.end();

      if (m_unrestricted || sourceTriggerExists) {
        ostringstream namestr;
        namestr << "pp_";
        namestr << sourceTrigger;
        namestr << "~";
        namestr << source.GetWord(0).GetFactor(m_sourceFactorId)->GetString();
        for (size_t i = 1; i < source.GetSize(); ++i) {
          const Factor* sourceFactor = source.GetWord(i).GetFactor(m_sourceFactorId);
          namestr << ",";
          namestr << sourceFactor->GetString();
        }
        namestr << "~";
        namestr << targetPhrase.GetWord(0).GetFactor(m_targetFactorId)->GetString();
        for (size_t i = 1; i < targetPhrase.GetSize(); ++i) {
          const Factor* targetFactor = targetPhrase.GetWord(i).GetFactor(m_targetFactorId);
          namestr << ",";
          namestr << targetFactor->GetString();
        }

        scoreBreakdown.SparsePlusEquals(namestr.str(),1);
      }
    }
  }
}

bool PhrasePairFeature::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_targetFactorId];
  return ret;
}

}
