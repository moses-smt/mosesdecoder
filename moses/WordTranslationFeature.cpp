#include <sstream>
#include <boost/algorithm/string.hpp>
#include "WordTranslationFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"
#include <boost/algorithm/string.hpp>

namespace Moses {

using namespace std;

bool WordTranslationFeature::Load(const std::string &filePathSource, const std::string &filePathTarget) 
{
  if (m_domainTrigger) {
    // domain trigger terms for each input document
    ifstream inFileSource(filePathSource.c_str());
    if (!inFileSource){
      cerr << "could not open file " << filePathSource << endl;
      return false;
    }
    
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
  }
  else {
    // restricted source word vocabulary
    ifstream inFileSource(filePathSource.c_str());
    if (!inFileSource)
      {
	cerr << "could not open file " << filePathSource << endl;
	return false;
      }
    
    std::string line;
    while (getline(inFileSource, line)) {
      m_vocabSource.insert(line);
    }
    
    inFileSource.close();
    
    // restricted target word vocabulary
    ifstream inFileTarget(filePathTarget.c_str());
    if (!inFileTarget)
      {
	cerr << "could not open file " << filePathTarget << endl;
	return false;
      }
    
    while (getline(inFileTarget, line)) {
      m_vocabTarget.insert(line);
    }
    
    inFileTarget.close();
    
    m_unrestricted = false;
  }
  return true;
}

void WordTranslationFeature::Evaluate
                     (const PhraseBasedFeatureContext& context,
                      ScoreComponentCollection* accumulator) const
{
  const Sentence& input = static_cast<const Sentence&>(context.GetSource());
  const TargetPhrase& targetPhrase = context.GetTargetPhrase();
  const AlignmentInfo &alignment = targetPhrase.GetAlignTerm();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignment.begin(); alignmentPoint != alignment.end(); alignmentPoint++) {
    const Phrase& sourcePhrase = targetPhrase.GetSourcePhrase();
    int sourceIndex = alignmentPoint->first;
    int targetIndex = alignmentPoint->second;
    Word ws = sourcePhrase.GetWord(sourceIndex);
    if (m_factorTypeSource == 0 && ws.IsNonTerminal()) continue;
    Word wt = targetPhrase.GetWord(targetIndex);
    if (m_factorTypeSource == 0 && wt.IsNonTerminal()) continue;
    string sourceWord = ws.GetFactor(m_factorTypeSource)->GetString();
    string targetWord = wt.GetFactor(m_factorTypeTarget)->GetString();
    if (m_ignorePunctuation) {
      // check if source or target are punctuation
      char firstChar = sourceWord.at(0);
      CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
      	continue;
      firstChar = targetWord.at(0);
      charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
    }

    if (!m_unrestricted) {
      if (m_vocabSource.find(sourceWord) == m_vocabSource.end())
	sourceWord = "OTHER";
      if (m_vocabTarget.find(targetWord) == m_vocabTarget.end())
	targetWord = "OTHER";
    }

    if (m_simple) {
      // construct feature name
      stringstream featureName;
      featureName << "wt_";
      featureName << sourceWord;
      featureName << "~";
      featureName << targetWord;
      accumulator->SparsePlusEquals(featureName.str(), 1);
    }
    if (m_domainTrigger && !m_sourceContext) {  
      const bool use_topicid = input.GetUseTopicId();
      const bool use_topicid_prob = input.GetUseTopicIdAndProb();        
      if (use_topicid || use_topicid_prob) {
	if(use_topicid) {
	  // use topicid as trigger
	  const long topicid = input.GetTopicId();
	  stringstream feature;
	  feature << "wt_";
	  if (topicid == -1) 
	    feature << "unk";
	  else 
	    feature << topicid;	  

	  feature << "_";
	  feature << sourceWord;
	  feature << "~";
	  feature << targetWord;
	  accumulator->SparsePlusEquals(feature.str(), 1);
	}
	else {
	  // use topic probabilities
	  const vector<string> &topicid_prob = *(input.GetTopicIdAndProb());
	  if (atol(topicid_prob[0].c_str()) == -1) {
	    stringstream feature;
	    feature << "wt_unk_";
	    feature << sourceWord;
	    feature << "~";
	    feature << targetWord;
	    accumulator->SparsePlusEquals(feature.str(), 1);
	  }
	  else {
	    for (size_t i=0; i+1 < topicid_prob.size(); i+=2) {
	      stringstream feature;
	      feature << "wt_";
	      feature << topicid_prob[i];
	      feature << "_";
	      feature << sourceWord;
	      feature << "~";
	      feature << targetWord;
	      accumulator->SparsePlusEquals(feature.str(), atof((topicid_prob[i+1]).c_str()));
	    }
	  }
	}
      }
      else {
	// range over domain trigger words (keywords)
	const long docid = input.GetDocumentId();
	for (set<string>::const_iterator p = m_vocabDomain[docid].begin(); p != m_vocabDomain[docid].end(); ++p) {
	  string sourceTrigger = *p;
	  stringstream feature;
	  feature << "wt_";
	  feature << sourceTrigger;
	  feature << "_";
	  feature << sourceWord;
	  feature << "~";
	  feature << targetWord;
	  accumulator->SparsePlusEquals(feature.str(), 1);    
	}
      }
    }
    if (m_sourceContext) {
      size_t globalSourceIndex = context.GetTranslationOption().GetStartPos() + sourceIndex;
      if (!m_domainTrigger && globalSourceIndex == 0) {
	// add <s> trigger feature for source
	stringstream feature;
	feature << "wt_";
	feature << "<s>,";
	feature << sourceWord;
	feature << "~";
	feature << targetWord;
	accumulator->SparsePlusEquals(feature.str(), 1);
      }
      
      // range over source words to get context
      for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
	if (contextIndex == globalSourceIndex) continue;
	string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
	if (m_ignorePunctuation) {
	  // check if trigger is punctuation
	  char firstChar = sourceTrigger.at(0);
	  CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
	  if(charIterator != m_punctuationHash.end())
	    continue;
	}
	
	const long docid = input.GetDocumentId();
	bool sourceTriggerExists = false;
	if (m_domainTrigger)
	  sourceTriggerExists = m_vocabDomain[docid].find( sourceTrigger ) != m_vocabDomain[docid].end();
	else if (!m_unrestricted)
	  sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();
	
	if (m_domainTrigger) {
	  if (sourceTriggerExists) {
	    stringstream feature;
	    feature << "wt_";
	    feature << sourceTrigger;
	    feature << "_";
	    feature << sourceWord;
	    feature << "~";
	    feature << targetWord;
	    accumulator->SparsePlusEquals(feature.str(), 1);
	  }
	}
	else if (m_unrestricted || sourceTriggerExists) {
	  stringstream feature;
	  feature << "wt_";
	  if (contextIndex < globalSourceIndex) {
	    feature << sourceTrigger;
	    feature << ",";
	    feature << sourceWord;
	  }
	  else {
	    feature << sourceWord;
	    feature << ",";
	    feature << sourceTrigger;
	  }
	  feature << "~";
	  feature << targetWord;
	  accumulator->SparsePlusEquals(feature.str(), 1);
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

void WordTranslationFeature::EvaluateChart(
            const ChartBasedFeatureContext& context,
    				ScoreComponentCollection* accumulator) const
{
  const TargetPhrase& targetPhrase = context.GetTargetPhrase();
  const AlignmentInfo &alignmentInfo = targetPhrase.GetAlignTerm();

  // process aligned words
  for (AlignmentInfo::const_iterator alignmentPoint = alignmentInfo.begin(); alignmentPoint != alignmentInfo.end(); alignmentPoint++) {
    const Phrase& sourcePhrase = targetPhrase.GetSourcePhrase();
    int sourceIndex = alignmentPoint->first;
    int targetIndex = alignmentPoint->second;
    Word ws = sourcePhrase.GetWord(sourceIndex);
    if (m_factorTypeSource == 0 && ws.IsNonTerminal()) continue;
    Word wt = targetPhrase.GetWord(targetIndex);
    if (m_factorTypeSource == 0 && wt.IsNonTerminal()) continue;
    string sourceWord = ws.GetFactor(m_factorTypeSource)->GetString();
    string targetWord = wt.GetFactor(m_factorTypeTarget)->GetString();
    if (m_ignorePunctuation) {
      // check if source or target are punctuation
      char firstChar = sourceWord.at(0);
      CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
      	continue;
      firstChar = targetWord.at(0);
      charIterator = m_punctuationHash.find( firstChar );
      if(charIterator != m_punctuationHash.end())
        continue;
    }

    if (!m_unrestricted) {
    	if (m_vocabSource.find(sourceWord) == m_vocabSource.end())
    		sourceWord = "OTHER";
    	if (m_vocabTarget.find(targetWord) == m_vocabTarget.end())
    		targetWord = "OTHER";
    }
    
    if (m_simple) {
    	// construct feature name
    	stringstream featureName;
    	featureName << "wt_";
    	//featureName << ((sourceExists||m_unrestricted) ? sourceWord : "OTHER");
    	featureName << sourceWord;
    	featureName << "~";
    	//featureName << ((targetExists||m_unrestricted) ? targetWord : "OTHER");
    	featureName << targetWord;
    	accumulator->SparsePlusEquals(featureName.str(), 1);
    }
  /*  if (m_sourceContext) {
    	size_t globalSourceIndex = cur_hypo.GetCurrSourceRange().GetStartPos() + sourceIndex;
    	if (globalSourceIndex == 0) {
    		// add <s> trigger feature for source
    		stringstream feature;
    		feature << "wt_";
    		feature << "<s>,";
    		feature << sourceWord;
    		feature << "~";
    		feature << targetWord;
    		accumulator->SparsePlusEquals(feature.str(), 1);
    		cerr << feature.str() << endl;
    	}

    	// range over source words to get context
    	for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
    		if (contextIndex == globalSourceIndex) continue;
    		string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_factorTypeSource)->GetString();
    		if (m_ignorePunctuation) {
    			// check if trigger is punctuation
    			char firstChar = sourceTrigger.at(0);
    			CharHash::const_iterator charIterator = m_punctuationHash.find( firstChar );
    			if(charIterator != m_punctuationHash.end())
    				continue;
    		}

    		bool sourceTriggerExists = false;
    		if (!m_unrestricted)
    			sourceTriggerExists = m_vocabSource.find( sourceTrigger ) != m_vocabSource.end();

    		if (m_unrestricted || sourceTriggerExists) {
    			stringstream feature;
    			feature << "wt_";
    			if (contextIndex < globalSourceIndex) {
    				feature << sourceTrigger;
    				feature << ",";
    				feature << sourceWord;
	    		}
    			else {
    				feature << sourceWord;
    				feature << ",";
    				feature << sourceTrigger;
	    		}
    			feature << "~";
    			feature << targetWord;
    			accumulator->SparsePlusEquals(feature.str(), 1);
    			cerr << feature.str() << endl;
    		}
    	}
	}*/
/*    if (m_targetContext) {
    	size_t globalTargetIndex = 0; // TODO
//    	size_t globalTargetIndex = cur_hypo.GetCurrTargetWordsRange().GetStartPos() + targetIndex;
    	if (globalTargetIndex == 0) {
    		// add <s> trigger feature for source
    		stringstream feature;
    		feature << "wt_";
    		feature << sourceWord;
    		feature << "~";
    		feature << "<s>,";
    		feature << targetWord;
    		accumulator->SparsePlusEquals(feature.str(), 1);
    		cerr << feature.str() << endl;
    	}

    	// range over target words (up to current position) to get context
    	for(size_t contextIndex = 0; contextIndex < globalTargetIndex; contextIndex++ ) {
    		Phrase outputPhrase = cur_hypo.GetOutputPhrase();
    		string targetTrigger = outputPhrase.GetWord(contextIndex).GetFactor(m_factorTypeTarget)->GetString();
    		//string targetTrigger = cur_hypo.GetWord(contextIndex).GetFactor(m_factorTypeTarget)->GetString();
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
    			cerr << feature.str() << endl;
    		}
    	}
    }*/
  }

}

}
