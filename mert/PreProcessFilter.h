#pragma once

#include <string>

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)

namespace MosesTuning
{


class ofdstream;
class ifdstream;

/*
 * This class runs the filter command in a child process and
 * then use this filter to process given sentences.
 */
class PreProcessFilter
{
public:
  explicit PreProcessFilter(const std::string& filterCommand);
  std::string ProcessSentence(const std::string& sentence);
  ~PreProcessFilter();

private:
  ofdstream* m_toFilter;
  ifdstream* m_fromFilter;
};

}

#endif
