#ifndef moses_iniReader_h
#define moses_iniReader_h

#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <map>
#include <exception>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

// simple reader of .ini files
class IniReader {
public:
  IniReader(const std::string &file)
  {
    std::ifstream inStr(file.c_str());
    if (! inStr.is_open())
      throw std::runtime_error("Failed to open file " + file);

    std::string section = "";
    std::string line;
    while (getline(inStr, line)) {
      if (line.empty() || line[0] == ';' || line[0] == '#') {
        // empty line or comment, do nothing
      } else if (line[0] == '[') {
        // new section
        section = line.substr(1, line.size() - 2);        
      } else {
        std::vector<std::string> cols;
        boost::split(cols, line, boost::is_any_of("="));
        std::for_each(cols.begin(), cols.end(),
          boost::bind(&boost::trim<std::string>, _1, std::locale()));
        if (section.empty())
          throw std::runtime_error("Missing section");
        if (cols.size() != 2)
          throw std::runtime_error("Failed to parse line: '" + line + "'");
        std::string key = section + "." + cols[0];
        properties[key] = cols[1];
      }
    }
    inStr.close();
  }

  template <class T>
  T Get(const std::string &key, T defaultValue)
  {
    std::map<std::string, std::string>::const_iterator it = properties.find(key);
    return (it == properties.end()) ? defaultValue : boost::lexical_cast<T>(it->second);
  }

private:
  std::map<std::string, std::string> properties;
};

#endif // moses_iniReader_h
