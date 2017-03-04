#ifndef moses_XmlOption_h
#define moses_XmlOption_h

#include <vector>
#include <string>
#include "Range.h"
#include "TargetPhrase.h"
#include "parameters/AllOptions.h"
namespace Moses
{

class TranslationOption;
class ReorderingConstraint;

/** This struct is used for storing XML force translation data for a given range in the sentence
 */
struct XmlOption {

  Range range;
  TargetPhrase targetPhrase;

  XmlOption(const Range &r, const TargetPhrase &tp)
    : range(r), targetPhrase(tp) {
  }

};

std::string ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName);
std::string TrimXml(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">") ;
bool isXmlTag(const std::string& tag, const std::string& lbrackStr="<", const std::string& rbrackStr=">");
std::vector<std::string> TokenizeXml(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">");

bool ProcessAndStripXMLTags(AllOptions const& opts,
                            std::string &line, std::vector<XmlOption const*> &res,
                            ReorderingConstraint &reorderingConstraint,
                            std::vector< size_t > &walls,
                            std::vector< std::pair<size_t, std::string> > &placeholders,
                            InputType &input);


}

#endif

