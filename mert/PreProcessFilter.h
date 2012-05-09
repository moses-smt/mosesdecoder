#ifndef MERT_PREPROCESSFILTER_H_
#define MERT_PREPROCESSFILTER_H_

#include <string>

#include "Fdstream.h"

/*
 * This class runs the filter command in a child process and
 * then use this filter to process given sentences.
 */
class PreProcessFilter
{
public:
    PreProcessFilter(const string& filterCommand);
    string ProcessSentence(const string& sentence);
    ~PreProcessFilter();

private:
    ofdstream* m_toFilter;
    ifdstream* m_fromFilter;    
};

#endif  // MERT_PREPROCESSFILTER_H_
