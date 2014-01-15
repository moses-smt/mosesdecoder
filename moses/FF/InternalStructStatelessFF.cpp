#include "InternalStructStatelessFF.h"
#include "moses/InputPath.h"
#include "moses/ScoreComponentCollection.h"
using namespace std;

namespace Moses
{
void InternalStructStatelessFF::Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const
{
//	cerr  << "MARIA!!!" << endl;
	scoreBreakdown.PlusEquals(this, 0);

}

void InternalStructStatelessFF::Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection *estimatedFutureScore) const
	{

cerr  << "HHHHH" << scoreBreakdown << endl;
scoreBreakdown.PlusEquals(this, 66);
/*	FactorList f_mask;
	f_mask.push_back(0);
		//if(inputPath.GetPhrase().GetStringRep(f_mask).)
	int score =50;
	for(size_t i=0;i<inputPath.GetPhrase().GetSize();i++){
		if(inputPath.GetPhrase(). GetFactor(i,0)->GetString().as_string()=="ist"){
			//cout<<inputPath.GetPhrase().GetStringRep(f_mask);
			score+=1;
		}
	}
	scoreBreakdown.PlusEquals(this, score);
*/
}

}

