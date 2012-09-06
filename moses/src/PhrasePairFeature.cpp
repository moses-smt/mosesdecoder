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
    return true;
}

  void PhrasePairFeature::InitializeForInput( Sentence const& in )
{
  m_local.reset(new ThreadLocalStorage);
  m_local->input = &in;
}

void PhrasePairFeature::Evaluate(const Hypothesis& cur_hypo, ScoreComponentCollection* accumulator) const {
	const TargetPhrase& target = cur_hypo.GetCurrTargetPhrase();
	const Phrase& source = target.GetSourcePhrase();
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
  return true;
} 
}
