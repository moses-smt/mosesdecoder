%include "std_string.i"

%module SwigQueryPt
%{
#include "QueryPt.h"
%}

class QueryPt {
  public:
    QueryPt(std::string path, int scores);
    std::string query(std::string phrase);
};
