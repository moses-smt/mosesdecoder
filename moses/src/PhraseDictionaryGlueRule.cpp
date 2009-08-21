
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
			stream << "[X] [S] ||| [X] [X] ||| [S] [X] ||| 0-0 1-1 ||| 0 ||| 0 0 \n"
						<<	"[X] [S] ||| <s> ||| <s> ||| ||| 0 ||| 0 0 \n"
						<<	"[X] [E] ||| </s> ||| </s> ||| ||| 0 ||| 0 0 \n"
						<<	"[X] [S] ||| [X] [X] ||| [S] [E] ||| 0-0 1-1 ||| 0 ||| 0 0 \n";						
			break;
		case Unbias:
		  stream << "[X] [S] ||| [X] [X] ||| [S] [X] ||| 0-0 1-1 ||| 0 ||| 0 0 \n"
			 << "[X] [S] ||| <s> ||| <s> ||| 0 ||| 0 0 \n"
			 << "[X] [E] ||| </s> ||| </s> ||| 0 ||| 0 0 \n"
			 << "[X] [S] ||| [X] [X] ||| [S] [E] ||| 0-0 1-1 ||| 0 ||| 0 0 \n"
			   << "[X] [X] ||| [X] [X] ||| [X] [X] ||| 0 ||| 0 0 \n";

			break;
		default:
			assert(false);
			break;
	}

	return MyBase::Load(input, output, stream, weight, tableLimit, languageModels, weightWP);
}

} // namespace

