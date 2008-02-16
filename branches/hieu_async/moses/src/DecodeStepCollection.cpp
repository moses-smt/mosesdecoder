

#include "DecodeStepCollection.h"
#include "DecodeStepTranslation.h"
#include "Util.h"

using namespace std;

DecodeStepCollection::~DecodeStepCollection()
{
	RemoveAllInColl(m_stepColl);
}

void DecodeStepCollection::CalcConflictingOutputFactors()
{
	iterator iterStep, iterOther;
	for (iterStep = m_stepColl.begin() ; iterStep != m_stepColl.end() ; ++iterStep)
	{
		DecodeStepTranslation *step = *iterStep;
		const FactorMask &mask = step->GetOutputFactorMask();
		
		iterOther = iterStep;
		for (++iterOther ; iterOther != m_stepColl.end() ; ++iterOther)
		{ // don't compare with itself
			DecodeStepTranslation *otherStep = *iterOther;
			const FactorMask &otherMask = otherStep->GetOutputFactorMask();
			FactorMask intersect = mask.GetIntersection(otherMask);
			
			step->AddConflictMask(intersect);
			otherStep->AddConflictMask(intersect);
		}
	}
}


