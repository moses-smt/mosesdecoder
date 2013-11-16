#ifndef moses_XmlOption_h
#define moses_XmlOption_h

#include <vector>
#include <string>
#include "WordsRange.h"
#include "TargetPhrase.h"

namespace Moses
{

class TranslationOption;
class ReorderingConstraint;

/** This struct is used for storing XML force translation data for a given range in the sentence
 */
struct XmlOption {

  WordsRange range;
  TargetPhrase targetPhrase;

  XmlOption(const WordsRange &r, const TargetPhrase &tp)
    : range(r), targetPhrase(tp) {
  }

};

std::string ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName);
std::string TrimXml(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">") ;
bool isXmlTag(const std::string& tag, const std::string& lbrackStr="<", const std::string& rbrackStr=">");
std::vector<std::string> TokenizeXml(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">");

bool ProcessAndStripXMLTags(std::string &line, std::vector<XmlOption*> &res, ReorderingConstraint &reorderingConstraint, std::vector< size_t > &walls,
                            std::vector< std::pair<size_t, std::string> > &placeholders,
                            int offset,
                            const std::string& lbrackStr="<", const std::string& rbrackStr=">");

}

#endif

