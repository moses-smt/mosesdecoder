#include "MaxentReordering.h"
#include "StaticData.h"

namespace Moses
{
MaxentReordering::MaxentReordering(const std::string &filePath, 
									 const std::vector<float>& weights, 
									 Condition condition, 
									 std::vector< FactorType >& f_factors, 
									 std::vector< FactorType >& e_factors)
  : m_NumScoreComponents(weights.size()), m_MaxContextLength(0) 
{
  std::cerr << "Creating maxent reordering...\n";
  //add ScoreProducer
  const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
  std::cerr << "weights: ";
  for(size_t w = 0; w < weights.size(); ++w){
	  std::cerr << weights[w] << " ";
  }
  std::cerr << "\n";
  m_Condition = DecodeCondition(condition);
  
  // TODO: Condition for maxent: 
  // (a) previous source words, source phrase, target phrase, jump: A 
  // (b) conditions as (a) plus conditioning on previously translated words: B

  for(size_t i = 0; i < m_Condition.size(); ++i){
    switch(m_Condition[i]){
    case E:
      m_FactorsE = e_factors;
	  if(m_FactorsE.empty()){
		//problem
		std::cerr << "Problem e factor mask is unexpectedly empty\n";
      }
      break;
    case F:
      m_FactorsF = f_factors;
	  if(m_FactorsF.empty()){
		//problem
		std::cerr << "Problem f factor mask is unexpectedly empty\n";
      }
      break;
    case C:
      m_FactorsC         = e_factors;
	  m_MaxContextLength = 1;
      if(m_FactorsC.empty()){
		//problem
		std::cerr << "Problem c factor mask is unexpectedly empty\n";
      }
      break;
    default:
      //problem
	  std::cerr << "Unknown conditioning option!\n";
      break;
    }
  }

  m_Table = MaxentReorderingTable::LoadAvailable(filePath, m_FactorsF, m_FactorsE, m_FactorsC);
}

MaxentReordering::~MaxentReordering(){
  if(m_Table){
	delete m_Table;
  }
}
  
std::vector<float> MaxentReordering::CalcScore(Hypothesis* hypothesis) const {
  std::vector<float> score(GetNumScoreComponents(), 0);
  std::vector<float> values_curr, values_next;
  bool nextHypoExists = false;

	// Maxent model has outcomes: RIGHT, RIGHT_PLUS, LEFT, LEFT_PLUS
	// Defined orientation values:
	// RIGHT = 0, RIGHT_PLUS = 1, LEFT = 2, LEFT_PLUS = 3, LEFT_undef = 4, NONE = 5
	// need to get value for current jump.. what jump (in f) does the current phrase perform
	// with respect to the previous or next input phrase? 
	vector<OrientationType> orientations = GetOrientationType(hypothesis); 
	OrientationType orientation_curr = orientations[0];
	OrientationType orientation_next = orientations[1];
	
	assert(orientation_curr != orientation_next);
	assert(orientation_curr <= 5 && (orientation_next == 2 || orientation_next == 3 || orientation_next == 5) );
	IFVERBOSE(2){
		std::cerr << "Maxent orientation type (curr): " << orientation_curr << "\n";
		std::cerr << "Maxent orientation type (next): " << orientation_next << "\n";
	}
	
	// grab data for current hypothesis
	const ScoreComponentCollection &reorderingScoreColl_curr = 
		hypothesis->GetCachedMaxentReorderingScore();
	values_curr = reorderingScoreColl_curr.GetScoresForProducer(this);
	//sanity check: right no. of probabilities? (i.e. 4?)
	assert(values_curr.size() == GetNumOrientationTypes());
	
	// try to grab data for next hypothesis
	
	const Hypothesis *nextHypo = hypothesis->GetNextHypothesis();
	if(nextHypo != NULL){
		const ScoreComponentCollection &reorderingScoreColl_next = 
		  nextHypo->GetCachedMaxentReorderingScore();
		values_next = reorderingScoreColl_next.GetScoresForProducer(this);
		//sanity check: right no. of probabilities? (i.e. 4?)
		assert(values_next.size() == GetNumOrientationTypes());
		nextHypoExists = true;
	}
    
  //add score	
	float value_curr = 0.0, value_next = 0.0;
	IFVERBOSE(2){
		std::cerr << "Curr scores: " << values_curr[0] << " " << values_curr[1] << " " << values_curr[2] << " " << values_curr[3] << "\n";
	}
	if(orientation_curr < 4){
  	value_curr = values_curr[orientation_curr];
	} 
  else if(orientation_curr == 4){ 
  	// optimistic guess: use better one of the values for LEFT and LEFT_PLUS
   	if(values_curr[2] >= values_curr[3]){
   		value_curr = values_curr[2];
   		orientation_curr = 2;
   	}
   	else{
   		value_curr = values_curr[3];
   		orientation_curr = 3;
   	}
  }  
  IFVERBOSE(2){
  	std::cerr << "Maxent value (curr): " << value_curr << "\n";
	}
  if(orientation_curr < 5)
  	score[orientation_curr] = value_curr;
  
  if(nextHypoExists){
  	if(orientation_next != 5){
	  	value_next = values_next[orientation_next];
	  } 
	  IFVERBOSE(2){
	  	std::cerr << "Next scores: " << values_next[0] << " " << values_next[1] << " " << values_next[2] << " " << values_next[3] << "\n";
	  	std::cerr << "Maxent value (next): " << value_next << "\n";
	  }
	  if(orientation_next < 5)
    	score[orientation_next] = value_next;
  }  
  IFVERBOSE(2){
  	for(int i=0; i< score.size(); i++){
	  	std::cerr << "maxent score[" << i << "] = " << score[i] << "\n";
	  }
  	std::cerr << "\n";
  }
  return score;
}

Phrase MaxentReordering::auxGetContext(const Hypothesis* hypothesis) const { 
  const Hypothesis* h = hypothesis;
  Phrase c(Output);
  if(0 == hypothesis->GetId()){
	return c;
  }
  while(0 != hypothesis->GetPrevHypo()->GetId() && c.GetSize() < m_MaxContextLength){
	hypothesis = hypothesis->GetPrevHypo();
	int needed = m_MaxContextLength - c.GetSize();
	const Phrase& p = hypothesis->GetCurrTargetPhrase();
	Phrase tmp(Output);
	if(needed > p.GetSize()){
	  //needed -= p.GetSize();
	  tmp = p;
	} else {
	  WordsRange range(p.GetSize() - needed, p.GetSize()-1);
	  tmp = p.GetSubString(range);
	}
	//new code: new append returns void not this...
	tmp.Append(c); c = tmp;
  }
  return c;
}

std::vector<MaxentReordering::Condition> MaxentReordering::DecodeCondition(MaxentReordering::Condition c){
  std::vector<MaxentReordering::Condition> result;
  switch(c){
  case F:
  case E:
  case C:
    result.push_back(c);
    break;
  case FE:
    result.push_back(F);
    result.push_back(E);
    break;
  case FEC:
    result.push_back(F);
    result.push_back(E);
    result.push_back(C);
    break;
  }
  return result;
}


std::vector< MaxentReordering::OrientationType> MaxentOrientationReordering::GetOrientationType(Hypothesis* currHypothesis) const
{
  const Hypothesis* prevHypothesis = currHypothesis->GetPrevHypo();
  const WordsRange prevSourceWordsRange  = prevHypothesis->GetCurrSourceWordsRange();
  const WordsRange currSourceWordsRange  = currHypothesis->GetCurrSourceWordsRange();
  
//  std::cerr << "\nhypo source: " << currHypothesis->GetSourcePhraseStringRep() <<  "  " << currHypothesis->GetCurrSourceWordsRange() << "\n";
//  std::cerr << "hypo target: " << currHypothesis->GetTargetPhraseStringRep() <<  "  " << currHypothesis->GetCurrTargetWordsRange() << "\n";
    
  // save outcome for evaluation with previous and with next phrase in source sentence
  std::vector< MaxentReordering::OrientationType> orientations;
    
  // If previous phrase in source sentence is TRANSLATED, we move to the right w.r.t. 
  // that phrase; if it is NOT TRANSLATED yet, we know we move to the left w.r.t. that 
  // phrase but we do not know yet how far we move.  
  
  //check if there is a previous hypo 
  if(0 == prevHypothesis->GetId()){
  	// NO HYPOTHESES YET!
  	IFVERBOSE(2){
  		std::cerr << "there are no hypotheses yet..\n";
  	}
    if(0 == currSourceWordsRange.GetStartPos()){
      // translate at source position 0 (monotone -> right movement -> RIGHT)
      IFVERBOSE(2){
      	std::cerr << "jump RIGHT\n"; 
      }
			orientations.push_back(RIGHT);
			orientations.push_back(NONE);
			return orientations;
    } else {
      // start translation from different position 
      // (discontinuous -> left movement -> LEFT or LEFT_PLUS)
      IFVERBOSE(2){
      	std::cerr << "fallback: jump LEFT_undef\n"; 
      }
			orientations.push_back(LEFT_undef);
			orientations.push_back(NONE);
      return orientations;
    }
  } else {
  	// THERE ARE PREVIOUS HYPOTHESES!
  	IFVERBOSE(2){
  		std::cerr << "there are hypotheses already..\n";
  	}
 	  const WordsRange prevWordsRange  = prevHypothesis->GetCurrSourceWordsRange();
    if(prevWordsRange.GetEndPos() == currSourceWordsRange.GetStartPos()-1){
    	// CASE 1: monotone right movement -> RIGHT 
      const Hypothesis *previous = currHypothesis->GetHypoContainingPosition(currSourceWordsRange.GetStartPos()-1);
    	assert( previous != NULL);
    	size_t jump = currHypothesis->GetCurrTargetWordsRange().GetStartPos() - previous->GetCurrTargetWordsRange().GetEndPos();
			IFVERBOSE(2){
				std::cerr << "Jump RIGHT: " << jump << "\n\n";
			}	
			orientations.push_back(RIGHT);	
			if( (currHypothesis->GetWordsBitmap().GetSize() > currSourceWordsRange.GetEndPos()+1) && 
			      (currHypothesis->GetWordsBitmap().GetValue( currSourceWordsRange.GetEndPos()+1 )) ){
				orientations.push_back( ReEvaluateWithNextPhraseInSource(currHypothesis, currSourceWordsRange) );
			}
			else{
				orientations.push_back(NONE);
			}	
			return orientations;
    } else if( currSourceWordsRange.GetStartPos() != 0 && currHypothesis->GetWordsBitmap().GetValue( currSourceWordsRange.GetStartPos()-1 )  ){
    	// CASE 2: previous source word is translated but not in the previous hypothesis
    	// farther right movement -> RIGHT_PLUS
    	const Hypothesis *previous = currHypothesis->GetHypoContainingPosition(currSourceWordsRange.GetStartPos()-1);
    	assert( previous != NULL);
//    	std::cerr << "previous source: " << previous->GetSourcePhraseStringRep() << "  " << previous->GetCurrSourceWordsRange() << "\n";
//    	std::cerr << "previous target: " << previous->GetTargetPhraseStringRep() << "  " << previous->GetCurrTargetWordsRange() << "\n";
    	std::string target = currHypothesis->GetCurrTargetSentence();
			size_t jump = currHypothesis->GetCurrTargetWordsRange().GetStartPos() - previous->GetCurrTargetWordsRange().GetEndPos();
			IFVERBOSE(2){
				std::cerr << "Jump RIGHT_PLUS: " << jump << "\n\n";
			}
    	orientations.push_back(RIGHT_PLUS);
    	if( (currHypothesis->GetWordsBitmap().GetSize() > currSourceWordsRange.GetEndPos()+1) && 
    	      (currHypothesis->GetWordsBitmap().GetValue( currSourceWordsRange.GetEndPos()+1 )) ){
				orientations.push_back( ReEvaluateWithNextPhraseInSource(currHypothesis, currSourceWordsRange) );
			}
			else{
				orientations.push_back(NONE);
			}
    	return orientations;
    } else if( currSourceWordsRange.GetStartPos() == 0 ){
    	// CASE 3: we are at position 0 in the source sentence..
    	// right movement --> RIGHT or RIGHT_PLUS
    	IFVERBOSE(2){
    		std::cerr << "Position 0..\n";
    	}
    	if( currHypothesis->GetCurrTargetWordsRange().GetStartPos() > 0 ){
    		IFVERBOSE(2){
    			std::cerr << "jump RIGHT_PLUS: " << currHypothesis->GetCurrTargetWordsRange().GetStartPos() + 1 << "\n\n";
    		}
				orientations.push_back(RIGHT_PLUS);
    	}
    	else{
    		IFVERBOSE(2){
    			std::cerr << "jump RIGHT: " << currHypothesis->GetCurrTargetWordsRange().GetStartPos() + 1 << "\n\n";
    		}
				orientations.push_back(RIGHT);
    	}
    	if( (currHypothesis->GetWordsBitmap().GetSize() > currSourceWordsRange.GetEndPos()+1) && 
    	      (currHypothesis->GetWordsBitmap().GetValue( currSourceWordsRange.GetEndPos()+1 )) ){
				orientations.push_back( ReEvaluateWithNextPhraseInSource(currHypothesis, currSourceWordsRange) );
			}
			else{
				orientations.push_back(NONE);
			}
			return orientations;
    } else {
    	// CASE 4: Previous source word is not yet translated -> undefined left movement -> LEFT_undef
    	IFVERBOSE(2){
    		std::cerr << "Previous word is not translated yet.. \n";
    	}    	
    	// if next word is already translated, use ONLY the probability of this well-defined left jump
    	// otherwise fallback to LEFT_undef and NONE
    	if( (currHypothesis->GetWordsBitmap().GetSize() > currSourceWordsRange.GetEndPos()+1) && 
    	      (currHypothesis->GetWordsBitmap().GetValue( currSourceWordsRange.GetEndPos()+1 )) ){
				orientations.push_back(NONE);
				IFVERBOSE(2){
      		std::cerr << "no jump w.r.t. previous words..\n"; 
      	}
    		orientations.push_back( ReEvaluateWithNextPhraseInSource(currHypothesis, currSourceWordsRange) );
			}
			else{
				IFVERBOSE(2){
      		std::cerr << "fallback: jump LEFT_undef\n"; 
      	}	
				orientations.push_back(LEFT_undef);
				orientations.push_back(NONE);
			}
      return orientations;
    } 
  }
}

MaxentReordering::OrientationType MaxentOrientationReordering::ReEvaluateWithNextPhraseInSource(Hypothesis* currHypothesis, const WordsRange currSourceWordsRange) const {
	// Word to the right of current phrase is already translated, TODO: what to do? 
  IFVERBOSE(2){
  	std::cerr << "Following word is already translated: backwards evaluation?\n";
  }
  const Hypothesis *next = currHypothesis->GetHypoContainingPosition( currSourceWordsRange.GetEndPos()+1 );
  assert( next != NULL);
  currHypothesis->SetNextHypothesis(next);
//  std::cerr << "next source: " << next->GetSourcePhraseStringRep() << "  " << next->GetCurrSourceWordsRange() << "\n";
//  std::cerr << "next target: " << next->GetTargetPhraseStringRep() << "  " << next->GetCurrTargetWordsRange() << "\n";
  std::string target = currHypothesis->GetCurrTargetSentence();
  int jump = next->GetCurrTargetWordsRange().GetEndPos() - currHypothesis->GetCurrTargetWordsRange().GetStartPos();
  assert(jump <= -1);
  if(jump == -1){
  	IFVERBOSE(2){
			std::cerr << "Following phrase jump LEFT: " << jump << "\n\n";
  	}
		return LEFT;
  }
	else{
		IFVERBOSE(2){
			std::cerr << "Following phrase jump LEFT_PLUS: " << jump << "\n\n";
		}
		return LEFT_PLUS;
	}
}

Score MaxentReordering::GetProb(const Phrase& f, const Phrase& e, const Phrase& f_context) const
{
	return m_Table->GetScore(f, e, Phrase(Output), f_context);
}


}

