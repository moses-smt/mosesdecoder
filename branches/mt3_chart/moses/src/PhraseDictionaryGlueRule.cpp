
#include <sstream>
#include "PhraseDictionaryGlueRule.h"
#include "StaticData.h"

namespace Moses
{

PhraseDictionaryGlueRule::PhraseDictionaryGlueRule(size_t numScoreComponent)
: MyBase(numScoreComponent)
{}

bool PhraseDictionaryGlueRule::Load(const std::vector<FactorType> &input
																	, const std::vector<FactorType> &output
																	, const std::vector<float> &weight
																	, size_t tableLimit
																	, const LMList &languageModels
																	, float weightWP)
{
	std::stringstream stream("");
	
	GlueRuleType glueRuleType = StaticData::Instance().GetGlueRuleType();
	
	switch (glueRuleType)
	{
		case Left:
			stream << "[S] ||| [S,1] [X,2] ||| [S,1] [X,2] ||| 100\n"
						<<	"[S] ||| <s> ||| <s> ||| 0\n"
						<<	"[E] ||| </s> ||| </s> ||| 0\n"
						<<	"[S] ||| [S,1] [E,2] ||| [S,1] [E,2] ||| 100\n";						
			break;
		case Unbias:
		  stream << "[S] ||| [S,1] [X,2] ||| [S,1] [X,2] ||| 100\n"
			 << "[S] ||| <s> ||| <s> ||| 0\n"
			 << "[E] ||| </s> ||| </s> ||| 0\n"
			 << "[S] ||| [S,1] [E,2] ||| [S,1] [E,2] ||| 100\n"
			   << "[X] ||| [X,1] [X,2] ||| [X,1] [X,2] ||| 100\n";

			break;
		default:
			assert(false);
			break;
	}

	return MyBase::Load(input, output, stream, weight, tableLimit, languageModels, weightWP);
}

} // namespace

