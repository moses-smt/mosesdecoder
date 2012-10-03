// $Id$
//#include "beammain.h"
#include "domain.h"
#include "tables-core.h"
#include "InputFileStream.h"
#include "SafeGetline.h"

#define TABLE_LINE_MAX_LENGTH 1000

using namespace std;

namespace MosesTraining
{

// handling of domain names: load database with sentence-id / domain name info
void Domain::load( const std::string &domainFileName ) {
  Moses::InputFileStream fileS( domainFileName );
  istream *fileP = &fileS;
  while(true) {
    char line[TABLE_LINE_MAX_LENGTH];
    SAFE_GETLINE((*fileP), line, TABLE_LINE_MAX_LENGTH, '\n', __FILE__);
    if (fileP->eof()) break;
    // read
    vector< string > domainSpecLine = tokenize( line );
    int lineNumber;
    if (domainSpecLine.size() != 2 ||
        ! sscanf(domainSpecLine[0].c_str(), "%d", &lineNumber)) {
      cerr << "ERROR: in domain specification line: '" << line << "'" << endl;
      exit(1);
    }
    // store
    string &name = domainSpecLine[1];
    spec.push_back( make_pair( lineNumber, name ));
    if (name2id.find( name ) == name2id.end()) {
      name2id[ name ] = list.size();
      list.push_back( name );
    }
  }
}

// get domain name based on sentence number
string Domain::getDomainOfSentence( int sentenceId ) {
  for(size_t i=0; i<spec.size(); i++) {
    if (sentenceId <= spec[i].first) {
      return spec[i].second;
    }
  }
  return "undefined";
}

}

