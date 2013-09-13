#include "InternalStructStatelessFF.h"
#include "moses/InputPath.h"
using namespace std;

namespace Moses
{
void InternalStructStatelessFF::Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{
	FactorList f_mask;
	f_mask.push_back(0);
		//if(inputPath.GetPhrase().GetStringRep(f_mask).)

	for(size_t i=0;i<inputPath.GetPhrase().GetSize();i++){
		if(inputPath.GetPhrase(). GetFactor(i,0)->GetString().as_string()=="ist"){
			cout<<inputPath.GetPhrase().GetStringRep(f_mask);
		}
	}
}
}

