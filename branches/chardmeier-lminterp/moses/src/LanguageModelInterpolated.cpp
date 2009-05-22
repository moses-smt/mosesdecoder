#include <string>
#include <numeric>
#include <vector>
#include "Factor.h"
#include "FactorCollection.h"
#include "TypeDef.h"
#include "InputFileStream.h"
#include "LanguageModelFactory.h"
#include "LanguageModelInterpolated.h"
#include "UserMessage.h"
#include "Util.h"

namespace Moses {

LanguageModelInterpolated::~LanguageModelInterpolated() {
	RemoveAllInColl(m_languageModels);
}

bool LanguageModelInterpolated::Load(const std::string &filePath, FactorType factorType,
	float weight, size_t nGramOrder) {

	m_filePath = filePath;
	m_factorType = factorType;
	m_weight = weight;
	m_nGramOrder = nGramOrder;

	InputFileStream lmConf(filePath);
	std::string line;

	while(getline(lmConf, line)) {
                // comments
                size_t comPos = line.find_first_of("#");
                if (comPos != string::npos)
                        line = line.substr(0, comPos);
                // trim leading and trailing spaces/tabs
                line = Trim(line);

		if(line == "")
			continue;

		std::vector<std::string> fields = Tokenize<std::string>(line);

		if(fields.size() != 4) {
			UserMessage::Add("Expected format for interpolated LM configuration: weight lmtype order lmfile");
			return false;
		}

		m_weights.push_back(Scan<float>(fields[0]));

		LMImplementation impl = static_cast<LMImplementation>(Scan<int>(fields[1]));
		size_t order = Scan<size_t>(fields[2]);

		if(order > nGramOrder) {
			TRACE_ERR("Warning: Overall order of interpolated LM lower than highest component LM order." <<
				std::endl << "         Overall order takes precedence." << std::endl);
			order = nGramOrder;
		}

		std::vector<FactorType> ft;
		ft.push_back(factorType);
		m_languageModels.push_back(LanguageModelFactory::CreateLanguageModel(impl, ft, order, fields[3],
			weight, m_scoreIndexManager, m_dub, false));
	}

	FactorCollection &factorCollection = FactorCollection::Instance();

        m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
        m_sentenceStartArray[m_factorType] = m_sentenceStart;

        m_sentenceEnd = factorCollection.AddFactor(Output, m_factorType, EOS_);
        m_sentenceEndArray[m_factorType] = m_sentenceEnd;
	
	return true;
}

float LanguageModelInterpolated::GetValue(const std::vector<const Word*> &contextFactor,
		State* finalState, unsigned int* len) const {
	std::vector<float> partialScores;

	unsigned int newLen = 0, ml = 0;
	State currentState;

	for(size_t i = 0; i < m_languageModels.size(); i++) {
		partialScores.push_back(UntransformScore(m_languageModels[i]->GetValue(contextFactor, &currentState, &ml)));

		// This class can only be used with language models that correctly return the
		// length of the context that was used!
		assert(ml != InvalidContextLength);

		if(ml > newLen || i == 0) {
			newLen = ml;
			if(finalState) *finalState = currentState;
		}
	}

	if(len) *len = newLen;

	return TransformScore(std::inner_product(m_weights.begin(), m_weights.end(), partialScores.begin(), .0f));
}

void LanguageModelInterpolated::InitializeBeforeSentenceProcessing() const {
	for(size_t i = 0; i < m_languageModels.size(); i++)
		m_languageModels[i]->InitializeBeforeSentenceProcessing();
}

void LanguageModelInterpolated::CleanUpAfterSentenceProcessing() const {
	for(size_t i = 0; i < m_languageModels.size(); i++)
		m_languageModels[i]->CleanUpAfterSentenceProcessing();
}

}
