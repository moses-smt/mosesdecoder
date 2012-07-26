#include <sstream>
#include "WordTranslationFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
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

  void WordTranslationFeature::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
  m_local->docid = in.GetDocumentId();
  m_local->topicid = in.GetTopicId();
  m_local->topicid_prob = in.GetTopicIdAndProb();
  m_local->use_topicid = in.GetUseTopicId();
  m_local->use_topicid_prob = in.GetUseTopicIdAndProb();
}

void WordTranslationFeature::Evaluate(const Hypothesis& cur_hypo, ScoreComponentCollection* accumulator) const
{
  const Sentence& input = *(m_local->input);
  const long docid = m_local->docid;
  const long topicid = m_local->topicid;
  const vector<string>* topicid_prob = m_local->topicid_prob;
  const bool use_topicid = m_local->use_topicid;
  const bool use_topicid_prob = m_local->use_topicid_prob;
  const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  const AlignmentInfo &alignment = targetPhrase.GetAlignmentInfo();

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
    	//featureName << ((sourceExists||m_unrestricted) ? sourceWord : "OTHER");
    	featureName << sourceWord;
    	featureName << "~";
    	//featureName << ((targetExists||m_unrestricted) ? targetWord : "OTHER");
    	featureName << targetWord;
    	accumulator->SparsePlusEquals(featureName.str(), 1);
    }
    if (m_domainTrigger && !m_sourceContext) {          
      if (use_topicid || use_topicid_prob) {
	// use topicid as trigger
	stringstream feature;
	feature << "wt_";

	if(use_topicid) {
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
	  if (atol((*topicid_prob)[0].c_str()) == -1) {
	    feature << "unk_";
	    feature << sourceWord;
	    feature << "~";
	    feature << targetWord;
	    accumulator->SparsePlusEquals(feature.str(), 1);
	  }
	  else {
	    for (size_t i=0; i < topicid_prob->size(); i+=2) {
	      feature << (*topicid_prob)[i];
	      feature << "_";
	      feature << sourceWord;
	      feature << "~";
	      feature << targetWord;
	      accumulator->SparsePlusEquals(feature.str(), atof((*topicid_prob)[i+1].c_str()));
	    }
	  }
	}

	// general feature
	/*stringstream feature;
	feature << "wt_";
	feature << sourceWord;
	feature << "~";
	feature << targetWord;
	accumulator->SparsePlusEquals(feature.str(), 1);

	if (topicid != -1) {
	  // topic-specific feature
	  stringstream feature;
	  feature << "wt_";
	  feature << topicid;
	  feature << "_";
	  feature << sourceWord;
	  feature << "~";
	  feature << targetWord;
	  accumulator->SparsePlusEquals(feature.str(), 1);
	  }*/
      }
      else {
	// range over domain trigger words
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
    	size_t globalSourceIndex = cur_hypo.GetCurrSourceWordsRange().GetStartPos() + sourceIndex;
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
    	}
    }
  }
}

void WordTranslationFeature::EvaluateChart(const ChartHypothesis& cur_hypo, int featureID,
    																			 ScoreComponentCollection* accumulator) const
{
  //const Sentence& input = *(m_local->input);
  const TargetPhrase& targetPhrase = cur_hypo.GetCurrTargetPhrase();
  const AlignmentInfo &alignmentInfo = targetPhrase.GetAlignmentInfo();
  const AlignmentInfo::CollType &alignment = alignmentInfo.GetTerminalAlignments();

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
