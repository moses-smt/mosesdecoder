#pragma once

#include <vector>
#include <string>
#include "WordsRange.h"
#include "TargetPhrase.h"
#include "ReorderingConstraint.h"

namespace Moses
{

class TranslationOption;

/** This struct is used for storing XML force translation data for a given range in the sentence
 */
struct XmlOption {

	WordsRange range;
	TargetPhrase targetPhrase;
	std::vector<XmlOption*> linkedOptions;

	XmlOption(const WordsRange &r, const TargetPhrase &tp): range(r), targetPhrase(tp), linkedOptions(0) {}

};

bool ProcessAndStripXMLTags(std::string &line,std::vector<std::vector<XmlOption*> > &res, ReorderingConstraint &reorderingConstraint, std::vector< size_t > &walls );

}


