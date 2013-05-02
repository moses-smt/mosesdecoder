#include <sstream>
#include <boost/algorithm/string.hpp>
#include "WordTranslationFeature.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "TranslationOption.h"
#include "UserMessage.h"
#include "util/string_piece_hash.hh"

namespace Moses {

using namespace std;

WordTranslationFeature::WordTranslationFeature(const std::string &line)
:StatelessFeatureFunction("WordTranslationFeature", FeatureFunction::unlimited, line)
,m_unrestricted(true)
,m_sparseProducerWeight(1)
,m_simple(true)
,m_sourceContext(false)
,m_targetContext(false)
,m_ignorePunctuation(false)
,m_domainTrigger(false)
{
  std::cerr << "Initializing word translation feature.. " << endl;

  string texttype;
  string filenameSource;
  string filenameTarget;

  for (size_t i = 0; i < m_args.size(); ++i) {
    const vector<string> &args = m_args[i];

    if (args[0] == "input-factor") {
      m_factorTypeSource = Scan<FactorType>(args[1]);
    }
    else if (args[0] == "output-factor") {
      m_factorTypeTarget = Scan<FactorType>(args[1]);
    }
    else if (args[0] == "simple") {
      m_simple = Scan<bool>(args[1]);
    }
    else if (args[0] == "source-context") {
      m_sourceContext = Scan<bool>(args[1]);
    }
    else if (args[0] == "target-context") {
      m_targetContext = Scan<bool>(args[1]);
    }
    else if (args[0] == "ignore-punctuation") {
      m_ignorePunctuation = Scan<bool>(args[1]);
    }
    else if (args[0] == "domain-trigger") {
      m_domainTrigger = Scan<bool>(args[1]);
    }
    else if (args[0] == "texttype") {
      texttype = args[1];
    }
    else if (args[0] == "source-path") {
      filenameSource = args[1];
    }
    else if (args[0] == "target-path") {
      filenameTarget = args[1];
    }
    else {
      throw "Unknown argument " + args[0];
    }
  }

  if (m_simple == 1) std::cerr << "using simple word translations.. ";
  if (m_sourceContext == 1) std::cerr << "using source context.. ";
  if (m_targetContext == 1) std::cerr << "using target context.. ";
  if (m_domainTrigger == 1) std::cerr << "using domain triggers.. ";

  // compile a list of punctuation characters
  if (m_ignorePunctuation) {
    std::cerr << "ignoring punctuation for triggers.. ";
    char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
    for (size_t i=0; i < sizeof(punctuation)-1; ++i) {
      m_punctuationHash[punctuation[i]] = 1;
    }
  }

  std::cerr << "done." << std::endl;

  // load word list for restricted feature set
  if (filenameSource != "") {
    cerr << "loading word translation word lists from " << filenameSource << " and " << filenameTarget << endl;
    if (!Load(filenameSource, filenameTarget)) {
      UserMessage::Add("Unable to load word lists for word translation feature from files " + filenameSource + " and " + filenameTarget);
      //return false;
    }
  } //else if (tokens.size() == 8) {

  // TODO not sure about this
  /*
  if (weight[0] != 1) {
    AddSparseProducer(wordTranslationFeature);
    cerr << "wt sparse producer weight: " << weight[0] << endl;
    if (m_mira)
      m_metaFeatureProducer = new MetaFeatureProducer("wt");
  }

  if (m_parameter->GetParam("report-sparse-features").size() > 0) {
    wordTranslationFeature->SetSparseFeatureReporting();
  }
  */

}

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
	  m_vocabDomain.resize(m_vocabDomain.size() + 1);
  	  vector<string> termVector;
	  boost::split(termVector, line, boost::is_any_of("\t "));
	  for (size_t i=0; i < termVector.size(); ++i)
	    m_vocabDomain.back().insert(termVector[i]);
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
	for (boost::unordered_set<std::string>::const_iterator p = m_vocabDomain[docid].begin(); p != m_vocabDomain[docid].end(); ++p) {
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

void WordTranslationFeature::Evaluate(const TargetPhrase &targetPhrase
                      , ScoreComponentCollection &scoreBreakdown
                      , ScoreComponentCollection &estimatedFutureScore) const
{
  CHECK(false);
}

}
