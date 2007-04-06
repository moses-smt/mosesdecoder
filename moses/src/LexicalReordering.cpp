#include "LexicalReordering.h"
#include "StaticData.h"

LexicalReordering::LexicalReordering(const std::string &filePath, 
									 const std::vector<float>& weights, 
									 Direction direction, 
									 Condition condition, 
									 std::vector< FactorType >& f_factors, 
									 std::vector< FactorType >& e_factors)
  : m_NumScoreComponents(weights.size()), m_MaxContextLength(0) 
{
  std::cerr << "Creating lexical reordering...\n";
  //add ScoreProducer
  const_cast<ScoreIndexManager&>(StaticData::Instance().GetScoreIndexManager()).AddScoreProducer(this);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
  std::cerr << "weights: ";
  for(size_t w = 0; w < weights.size(); ++w){
	std::cerr << weights[w] << " ";
  }
  std::cerr << "\n";
  m_Direction = DecodeDirection(direction);
  m_Condition = DecodeCondition(condition);
    
  //m_FactorsE = e_factors;
  //m_FactorsF = f_factors;
  //Todo:should check that
  //- if condition contains e or c than e_factors non empty
  //- if condition contains f f_factors non empty
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
  if(weights.size() == m_Direction.size()){
    m_OneScorePerDirection = true;
	std::cerr << "Reordering types NOT individualy weighted!\n";
  } else {
	m_OneScorePerDirection = false;
  }
  m_Table = LexicalReorderingTable::LoadAvailable(filePath, m_FactorsF, m_FactorsE, m_FactorsC);
}

LexicalReordering::~LexicalReordering(){
  if(m_Table){
	delete m_Table;
  }
}
  
std::vector<float> LexicalReordering::CalcScore(Hypothesis* hypothesis) const {
  std::vector<float> score(GetNumScoreComponents(), 0);
  std::vector<float> values;

  //for every direction
  for(size_t i = 0; i < m_Direction.size(); ++i){
    //grab data
    if(Forward == m_Direction[i]){
      //relates to prev hypothesis as we dont know next phrase for current yet
      //sanity check: is there a previous hypothesis?
      if(0 == hypothesis->GetPrevHypo()->GetId()){
				continue; //no score continue with next direction
      }
      //grab probs for prev hypothesis
			const ScoreComponentCollection &reorderingScoreColl = 
							hypothesis->GetPrevHypo()->GetCachedReorderingScore();
			values = reorderingScoreColl.GetScoresForProducer(this);
			/*
      values = m_Table->GetScore((hypothesis->GetPrevHypo()->GetSourcePhrase()).GetSubString(hypothesis->GetPrevHypo()->GetCurrSourceWordsRange()),
								 hypothesis->GetPrevHypo()->GetCurrTargetPhrase(),
								 auxGetContext(hypothesis->GetPrevHypo()));
			*/
    }
    if(Backward == m_Direction[i])
		{
			const ScoreComponentCollection &reorderingScoreColl = 
				hypothesis->GetCachedReorderingScore();
			values = reorderingScoreColl.GetScoresForProducer(this);
			/*
      values = m_Table->GetScore(hypothesis->GetSourcePhrase().GetSubString(hypothesis->GetCurrSourceWordsRange()),
								 hypothesis->GetCurrTargetPhrase(),
								 auxGetContext(hypothesis));
								 */
    }
    
    //add score
    //sanity check: do we have any probs?
	  assert(values.size() == (GetNumOrientationTypes() * m_Direction.size()));

		OrientationType orientation = GetOrientationType(hypothesis); 
    float value = values[orientation + i * GetNumOrientationTypes()];
    if(m_OneScorePerDirection){ 
      //one score per direction
      score[i] = value;
    } else {
      //one score per direction and orientation
      score[orientation + i * GetNumOrientationTypes()] = value; 
    }
  }
  return score;
}

Phrase LexicalReordering::auxGetContext(const Hypothesis* hypothesis) const { 
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

std::vector<LexicalReordering::Condition> LexicalReordering::DecodeCondition(LexicalReordering::Condition c){
  std::vector<LexicalReordering::Condition> result;
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

std::vector<LexicalReordering::Direction> LexicalReordering::DecodeDirection(LexicalReordering::Direction d){
  std::vector<Direction> result;
  if(Bidirectional == d){
    result.push_back(Backward);
    result.push_back(Forward);
  } else {
    result.push_back(d);
  }
  return result;
}

LexicalReordering::OrientationType LexicalMonotonicReordering::GetOrientationType(Hypothesis* currHypothesis) const
{
  const Hypothesis* prevHypothesis = currHypothesis->GetPrevHypo();
  const WordsRange currWordsRange  = currHypothesis->GetCurrSourceWordsRange();
  //check if there is a previous hypo 
  if(0 == prevHypothesis->GetId()){
    if(0 == currWordsRange.GetStartPos()){
      return Monotone;
    } else {
      return NonMonotone;
    }
  } else {
	const WordsRange  prevWordsRange = prevHypothesis->GetCurrSourceWordsRange();

    if(prevWordsRange.GetEndPos() == currWordsRange.GetStartPos()-1){
      return Monotone;
    } else {
      return NonMonotone;
    }
  }
} 

LexicalReordering::OrientationType LexicalOrientationReordering::GetOrientationType(Hypothesis* currHypothesis) const
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


LexicalReordering::OrientationType LexicalDirectionalReordering::GetOrientationType(Hypothesis* currHypothesis) const{
  const Hypothesis* prevHypothesis = currHypothesis->GetPrevHypo();
  const WordsRange currWordsRange = currHypothesis->GetCurrSourceWordsRange();
  //check if there is a previous hypo 
  if(0 == prevHypothesis->GetId()){
	return Right;
  } else {
	const WordsRange prevWordsRange = prevHypothesis->GetCurrSourceWordsRange();

    if(prevWordsRange.GetEndPos() <= currWordsRange.GetStartPos()){
	  return Right;
	} else {
	  return Left;
	}
  }
} 

Score LexicalReordering::GetProb(const Phrase& f, const Phrase& e) const
{
	return m_Table->GetScore(f, e, Phrase(Output));
}
