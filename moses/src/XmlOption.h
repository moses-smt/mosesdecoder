#ifndef moses_XmlOption_h
#define moses_XmlOption_h

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

std::string ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName);
std::string TrimXml(const std::string& str) ;
bool isXmlTag(const std::string& tag);
std::vector<std::string> TokenizeXml(const std::string& str);

bool ProcessAndStripXMLTags(std::string &line,std::vector<std::vector<XmlOption*> > &res, ReorderingConstraint &reorderingConstraint, std::vector< size_t > &walls );

}

#endif

