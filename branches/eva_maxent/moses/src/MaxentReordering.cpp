#include "MaxentReordering.h"
#include "StaticData.h"

namespace Moses
{
MaxentReordering::MaxentReordering(const std::string &filePath, 
									 const std::vector<float>& weights, 
//									 Direction direction, 
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
//  m_Direction = DecodeDirection(direction);
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
	std::cerr << "calculate maxent score..\n"; 
	// TODO: what is the size of this vector?
  std::vector<float> score(GetNumScoreComponents(), 0);
  std::vector<float> values;

  //grab data
  //relates to prev hypothesis as we dont know next phrase for current yet
  //sanity check: is there a previous hypothesis?
  if(0 == hypothesis->GetPrevHypo()->GetId()){
//	continue; //no score continue with next direction
  }
  //grab probs for prev hypothesis
	const ScoreComponentCollection &reorderingScoreColl = 
		hypothesis->GetPrevHypo()->GetCachedMaxentReorderingScore();
	values = reorderingScoreColl.GetScoresForProducer(this);
    
  //add score
  //sanity check: right no. of probabilities? (i.e. 4?)
  assert(values.size() == (GetNumOrientationTypes()));

	// TODO: maxent model has different outcomes:
	// 1 (RIGHT),2 (RIGHT_PLUS,3 (LEFT), 4 (LEFT_PLUS)
	// need to get value for current jump.. what jump (in f) does the current phrase perform
	// with respect to the previously translated phrase? 
	OrientationType orientation = GetOrientationType(hypothesis); 
	
	// TODO: get maxent value for this orientation type 
  float value = values[orientation];
  score[0] = value;
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


// TODO: maxent checks jump probability, not just orientation probability
MaxentReordering::OrientationType MaxentOrientationReordering::GetOrientationType(Hypothesis* currHypothesis) const
{
  const Hypothesis* prevHypothesis = currHypothesis->GetPrevHypo();
  const WordsRange currWordsRange  = currHypothesis->GetCurrSourceWordsRange();
  //check if there is a previous hypo 
  if(0 == prevHypothesis->GetId()){
    if(0 == currWordsRange.GetStartPos()){
      return Monotone;
    } else {
      return Discontinuous;
    }
  } else {
	const WordsRange prevWordsRange  = prevHypothesis->GetCurrSourceWordsRange();

    if(prevWordsRange.GetEndPos() == currWordsRange.GetStartPos()-1){
      return Monotone;
    } else if(prevWordsRange.GetStartPos() == currWordsRange.GetEndPos()+1) {
      return Swap;
    } else {
      return Discontinuous;
    }
  }
}



Score MaxentReordering::GetProb(const Phrase& f, const Phrase& e, const Phrase& f_context) const
{
	std::cerr << "GetProb()\n";
	return m_Table->GetScore(f, e, Phrase(Output), f_context);
}

}

