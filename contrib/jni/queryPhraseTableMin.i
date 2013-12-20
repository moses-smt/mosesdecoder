/* File: queryPhraseTableMin.i */
%include "std_string.i"

%module queryPhraseTableMin
%{
#include "QueryPhraseTableMin.h"
%}

class QueryPhraseTableMin {
  public:
    QueryPhraseTableMin(std::string path);
    std::string query(std::string phrase);
};
