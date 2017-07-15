#include <sstream>
#include <boost/algorithm/string.hpp>
#include "WordTranslationFeature.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationOption.h"
#include "moses/InputPath.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

WordTranslationFeature::WordTranslationFeature(const std::string &line)
  :StatelessFeatureFunction(0, line)
  ,m_unrestricted(true)
  ,m_simple(true)
  ,m_sourceContext(false)
  ,m_targetContext(false)
  ,m_domainTrigger(false)
  ,m_ignorePunctuation(false)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();

  if (m_simple == 1) VERBOSE(1, " Using simple word translations.");
  if (m_sourceContext == 1) VERBOSE(1, " Using source context.");
  if (m_targetContext == 1) VERBOSE(1, " Using target context.");
  if (m_domainTrigger == 1) VERBOSE(1, " Using domain triggers.");

  // compile a list of punctuation characters
  if (m_ignorePunctuation) {
    VERBOSE(1, " Ignoring punctuation for triggers.");
    char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
    for (size_t i=0; i < sizeof(punctuation)-1; ++i) {
      m_punctuationHash[punctuation[i]] = 1;
    }
  }

  VERBOSE(1, " Done." << std::endl);

  // TODO not sure about this
  /*
  if (weight[0] != 1) {
    AddSparseProducer(wordTranslationFeature);
    VERBOSE(1, "wt sparse producer weight: " << weight[0] << std::endl);
    if (m_mira)
      m_metaFeatureProducer = new MetaFeatureProducer("wt");
  }

  if (m_parameter->GetParam("report-sparse-features").size() > 0) {
    wordTranslationFeature->SetSparseFeatureReporting();
  }
  */

}

void WordTranslationFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "input-factor") {
    m_factorTypeSource = Scan<FactorType>(value);
  } else if (key == "output-factor") {
    m_factorTypeTarget = Scan<FactorType>(value);
  } else if (key == "simple") {
    m_simple = Scan<bool>(value);
  } else if (key == "source-context") {
    m_sourceContext = Scan<bool>(value);
  } else if (key == "target-context") {
    m_targetContext = Scan<bool>(value);
  } else if (key == "ignore-punctuation") {
    m_ignorePunctuation = Scan<bool>(value);
  } else if (key == "domain-trigger") {
    m_domainTrigger = Scan<bool>(value);
  } else if (key == "texttype") {
    //texttype = value; TODO not used
  } else if (key == "source-path") {
    m_filePathSource = value;
  } else if (key == "target-path") {
    m_filePathTarget = value;
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void WordTranslationFeature::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  // load word list for restricted feature set
  if (m_filePathSource.empty()) {
    return;
  } //else if (tokens.size() == 8) {

  FEATUREVERBOSE(1, "Loading word translation word lists from " << m_filePathSource << " and " << m_filePathTarget << std::endl);
  if (m_domainTrigger) {
    // domain trigger terms for each input document
    ifstream inFileSource(m_filePathSource.c_str());
    UTIL_THROW_IF2(!inFileSource, "could not open file " << m_filePathSource);

    std::string line;
    while (getline(inFileSource, line)) {
      m_vocabDomain.resize(m_vocabDomain.size() + 1);
      vector<string> termVector;
      boost::split(termVector, line, boost::is_any_of("\t "));
      for (size_t i=0; i < termVector.size(); ++i)
        m_vocabDomain.back().insert(termVector[i]);
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

    // restricted target word vocabulary
    ifstream inFileTarget(m_filePathTarget.c_str());
    UTIL_THROW_IF2(!inFileTarget, "could not open file " << m_filePathTarget);

    while (getline(inFileTarget, line)) {
      m_vocabTarget.insert(line);
    }

    inFileTarget.close();

    m_unrestricted = false;
  }
}

void WordTranslationFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  const Sentence& sentence = static_cast<const Sentence&>(input);
  const AlignmentInfo &alignment = targetPhrase.GetAlignTerm();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    const Phrase& sourcePhrase = inputPath.GetPhrase();
    int sourceIndex = alignmentPoint->first;
    int targetIndex = alignmentPoint->second;
    Word ws = sourcePhrase.GetWord(sourceIndex);
    if (m_factorTypeSource == 0 && ws.IsNonTerminal()) continue;
    Word wt = targetPhrase.GetWord(targetIndex);
    if (m_factorTypeSource == 0 && wt.IsNonTerminal()) continue;
    StringPiece sourceWord = ws.GetFactor(m_factorTypeSource)->GetString();
    StringPiece targetWord = wt.GetFactor(m_factorTypeTarget)->GetString();
    if (m_ignorePunctuation) {
      // check if source or target are punctuation
      char firstChar = sourceWord[0];
      CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
      firstChar = targetWord[0];
      charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
    }

    if (!m_unrestricted) {
      if (FindStringPiece(m_vocabSource, sourceWord) == m_vocabSource.end())
        sourceWord = "OTHER";
      if (FindStringPiece(m_vocabTarget, targetWord) == m_vocabTarget.end())
        targetWord = "OTHER";
    }

    if (m_simple) {
      // construct feature name
      util::StringStream featureName;
      featureName << m_description << "_";
      featureName << sourceWord;
      featureName << "~";
      featureName << targetWord;
      scoreBreakdown.SparsePlusEquals(featureName.str(), 1);
    }
    if (m_domainTrigger && !m_sourceContext) {
      const bool use_topicid = sentence.GetUseTopicId();
      const bool use_topicid_prob = sentence.GetUseTopicIdAndProb();
      if (use_topicid || use_topicid_prob) {
        if(use_topicid) {
          // use topicid as trigger
          const long topicid = sentence.GetTopicId();
          util::StringStream feature;
          feature << m_description << "_";
          if (topicid == -1)
            feature << "unk";
          else
            feature << topicid;

          feature << "_";
          feature << sourceWord;
          feature << "~";
          feature << targetWord;
          scoreBreakdown.SparsePlusEquals(feature.str(), 1);
        } else {
          // use topic probabilities
          const vector<string> &topicid_prob = *(input.GetTopicIdAndProb());
          if (atol(topicid_prob[0].c_str()) == -1) {
            util::StringStream feature;
            feature << m_description << "_unk_";
            feature << sourceWord;
            feature << "~";
            feature << targetWord;
            scoreBreakdown.SparsePlusEquals(feature.str(), 1);
          } else {
            for (size_t i=0; i+1 < topicid_prob.size(); i+=2) {
              util::StringStream feature;
              feature << m_description << "_";
              feature << topicid_prob[i];
              feature << "_";
              feature << sourceWord;
              feature << "~";
              feature << targetWord;
              scoreBreakdown.SparsePlusEquals(feature.str(), atof((topicid_prob[i+1]).c_str()));
            }
          }
        }
      } else {
        // range over domain trigger words (keywords)
        const long docid = input.GetDocumentId();
        for (boost::unordered_set<std::string>::const_iterator p = m_vocabDomain[docid].begin(); p != m_vocabDomain[docid].end(); ++p) {
          string sourceTrigger = *p;
          util::StringStream feature;
          feature << m_description << "_";
          feature << sourceTrigger;
          feature << "_";
          feature << sourceWord;
          feature << "~";
          feature << targetWord;
          scoreBreakdown.SparsePlusEquals(feature.str(), 1);
        }
      }
    }
    if (m_sourceContext) {
      size_t globalSourceIndex = inputPath.GetWordsRange().GetStartPos() + sourceIndex;
      if (!m_domainTrigger && globalSourceIndex == 0) {
        // add <s> trigger feature for source
        util::StringStream feature;
        feature << m_description << "_";
        feature << "<s>,";
        feature << sourceWord;
        feature << "~";
        feature << targetWord;
        scoreBreakdown.SparsePlusEquals(feature.str(), 1);
      }

      // range over source words to get context
      for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
        if (contextIndex == globalSourceIndex) continue;
        StringPiece sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
        if (m_ignorePunctuation) {
          // check if trigger is punctuation
          char firstChar = sourceTrigger[0];
          CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
          if(charIterator != m_punctuationHash.end())
            continue;
        }

        const long docid = input.GetDocumentId();
        bool sourceTriggerExists = false;
        if (m_domainTrigger)
          sourceTriggerExists = FindStringPiece(m_vocabDomain[docid], sourceTrigger ) != m_vocabDomain[docid].end();
        else if (!m_unrestricted)
          sourceTriggerExists = FindStringPiece(m_vocabSource, sourceTrigger ) != m_vocabSource.end();

        if (m_domainTrigger) {
          if (sourceTriggerExists) {
            util::StringStream feature;
            feature << m_description << "_";
            feature << sourceTrigger;
            feature << "_";
            feature << sourceWord;
            feature << "~";
            feature << targetWord;
            scoreBreakdown.SparsePlusEquals(feature.str(), 1);
          }
        } else if (m_unrestricted || sourceTriggerExists) {
          util::StringStream feature;
          feature << m_description << "_";
          if (contextIndex < globalSourceIndex) {
            feature << sourceTrigger;
            feature << ",";
            feature << sourceWord;
          } else {
            feature << sourceWord;
            feature << ",";
            feature << sourceTrigger;
          }
          feature << "~";
          feature << targetWord;
          scoreBreakdown.SparsePlusEquals(feature.str(), 1);
        }
      }
    }
    if (m_targetContext) {
      throw runtime_error("Can't use target words outside current translation option in a stateless feature");
      /*
      size_t globalTargetIndex = cur_hypo.GetCurrTargetWordsRange().GetStartPos() + targetIndex;
      if (globalTargetIndex == 0) {
      	// add <s> trigger feature for source
      	stringstream feature;
      	feature << "wt_";
      	feature << sourceWord;
      	feature << "~";
      	feature << "<s>,";
      	feature << targetWord;
      	accumulator->SparsePlusEquals(feature.str(), 1);
      }

      // range over target words (up to current position) to get context
      for(size_t contextIndex = 0; contextIndex < globalTargetIndex; contextIndex++ ) {
      	string targetTrigger = cur_hypo.GetWord(contextIndex).GetFactor(m_factorTypeTarget)->GetString();
      	if (m_ignorePunctuation) {
      		// check if trigger is punctuation
      		char firstChar = targetTrigger.at(0);
      		CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      		if(charIterator != m_punctuationHash.end())
      			continue;
      	}

      	bool targetTriggerExists = false;
      	if (!m_unrestricted)
      		targetTriggerExists = m_vocabTarget.find( targetTrigger ) != m_vocabTarget.end();

      	if (m_unrestricted || targetTriggerExists) {
      		stringstream feature;
      		feature << "wt_";
      		feature << sourceWord;
      		feature << "~";
      		feature << targetTrigger;
      		feature << ",";
      		feature << targetWord;
      		accumulator->SparsePlusEquals(feature.str(), 1);
      	}
      }*/
    }
  }
}

bool WordTranslationFeature::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorTypeTarget];
  return ret;
}

}
