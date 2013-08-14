#ifndef moses_DltOption_h
#define moses_DltOption_h

#include <vector>
#include <string>

namespace Moses
{

/** This struct is used for handling any information useful for Document-Level Translation
This information stay in start-end xml tag, like <dlt key="value" key="value"/>
 */
struct DltOption {

  DltOption() {}

};

std::string ParseDltTagAttribute(const std::string& tag,const std::string& attributeName);
std::string TrimDlt(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">");
bool isDltTag(const std::string& tag, const std::string& lbrackStr="<", const std::string& rbrackStr=">");
std::vector<std::string> TokenizeXml(const std::string& str, const std::string& lbrackStr="<", const std::string& rbrackStr=">");

bool ProcessAndStripDltTags(std::string &line, const std::string& lbrackStr="<", const std::string& rbrackStr=">");

}

#endif

