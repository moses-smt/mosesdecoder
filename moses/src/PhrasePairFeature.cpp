#include "AlignmentInfo.h"
#include "PhrasePairFeature.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include <boost/algorithm/string.hpp>

using namespace std;

namespace Moses {

string PhrasePairFeature::GetScoreProducerWeightShortName(unsigned) const
{
  return "pp";
}

size_t PhrasePairFeature::GetNumInputScores() const 
{
  return 0;
}

  bool PhrasePairFeature::Load(const std::string &filePathSource/*, const std::string &filePathTarget*/) 
{
  if (m_domainTrigger) {
    // domain trigger terms for each input document
    ifstream inFileSource(filePathSource.c_str());
    if (!inFileSource)
      {
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
  return true;
}

  void PhrasePairFeature::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
  m_local->docid = in.GetDocumentId();
  m_local->topicid = in.GetTopicId();
  m_local->use_topicid = in.GetUseTopicId();
  m_local->topicid_prob = in.GetTopicIdAndProb();
  m_local->use_topicid_prob = in.GetUseTopicIdAndProb();
}

void PhrasePairFeature::Evaluate(const Hypothesis& cur_hypo, ScoreComponentCollection* accumulator) const {
  const TargetPhrase& target = cur_hypo.GetCurrTargetPhrase();
  const Phrase& source = target.GetSourcePhrase();
  const long docid = m_local->docid;
  const long topicid = m_local->topicid;
  const bool use_topicid = m_local->use_topicid;
  const bool use_topicid_prob = m_local->use_topicid_prob;
/*   const AlignmentInfo& align = cur_hypo.GetAlignmentInfo();
   for (AlignmentInfo::const_iterator i = align.begin(); i != align.end(); ++i) {
    const Factor* sourceFactor = source.GetWord(i->first).GetFactor(m_sourceFactorId);
    const Factor* targetFactor = cur_hypo.GetWord(i->second).GetFactor(m_targetFactorId);
    ostringstream namestr;
    namestr << sourceFactor->GetString();
    namestr << ":";
    namestr << targetFactor->GetString();
    accumulator->PlusEquals(this,namestr.str(),1);
  }*/
   
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
	   namestr << target.GetWord(0).GetFactor(m_targetFactorId)->GetString();
	   for (size_t i = 1; i < target.GetSize(); ++i) {
		   const Factor* targetFactor = target.GetWord(i).GetFactor(m_targetFactorId);
		   namestr << ",";
		   namestr << targetFactor->GetString();
	   }

	   accumulator->SparsePlusEquals(namestr.str(),1);
   }
   if (m_domainTrigger) {
     // compute pair
     ostringstream pair;
     pair << source.GetWord(0).GetFactor(m_sourceFactorId)->GetString();
     for (size_t i = 1; i < source.GetSize(); ++i) {
       const Factor* sourceFactor = source.GetWord(i).GetFactor(m_sourceFactorId);
       pair << ",";
       pair << sourceFactor->GetString();
     }
     pair << "~";
     pair << target.GetWord(0).GetFactor(m_targetFactorId)->GetString();
     for (size_t i = 1; i < target.GetSize(); ++i) {
       const Factor* targetFactor = target.GetWord(i).GetFactor(m_targetFactorId);
       pair << ",";
       pair << targetFactor->GetString();
     }

     if (use_topicid || use_topicid_prob) {
       // use topicid as trigger                                                                                           
       if(use_topicid) {
	 stringstream feature;
	 feature << "pp_";
	 if (topicid == -1)
	   feature << "unk";
	 else
	   feature << topicid;

	 feature << "_";
	 feature << pair.str();
	 accumulator->SparsePlusEquals(feature.str(), 1);
       }
       else {
	 // use topic probabilities                                                                                        
	 const vector<string> &topicid_prob = *(m_local->topicid_prob);
	 if (atol(topicid_prob[0].c_str()) == -1) {
	   stringstream feature;
	   feature << "pp_unk_";
	   feature << pair.str();
	   accumulator->SparsePlusEquals(feature.str(), 1);
	 }
	 else {
	   for (size_t i=0; i+1 < topicid_prob.size(); i+=2) {
	     stringstream feature;
	     feature << "pp_";
	     feature << topicid_prob[i];
	     feature << "_";
	     feature << pair.str();
	     accumulator->SparsePlusEquals(feature.str(), atof((topicid_prob[i+1]).c_str()));
	   }
	 }
       }
     }
     else {
       // range over domain trigger words
       for (set<string>::const_iterator p = m_vocabDomain[docid].begin(); p != m_vocabDomain[docid].end(); ++p) {
	 string sourceTrigger = *p;
	 ostringstream namestr;
	 namestr << "pp_"; 
	 namestr << sourceTrigger;
	 namestr << "_";
	 namestr << pair.str();	
	 accumulator->SparsePlusEquals(namestr.str(),1);
       }
     }
   }
   if (m_sourceContext) {
	   const Sentence& input = *(m_local->input);

	   // range over source words to get context
	   for(size_t contextIndex = 0; contextIndex < input.GetSize(); contextIndex++ ) {
		   string sourceTrigger = input.GetWord(contextIndex).GetFactor(m_sourceFactorId)->GetString();
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
			   namestr << target.GetWord(0).GetFactor(m_targetFactorId)->GetString();
			   for (size_t i = 1; i < target.GetSize(); ++i) {
				   const Factor* targetFactor = target.GetWord(i).GetFactor(m_targetFactorId);
				   namestr << ",";
				   namestr << targetFactor->GetString();
			   }
			   
			   accumulator->SparsePlusEquals(namestr.str(),1);
		   }
	   }
	}
}

bool PhrasePairFeature::ComputeValueInTranslationOption() const {
  return false;
} 
}
