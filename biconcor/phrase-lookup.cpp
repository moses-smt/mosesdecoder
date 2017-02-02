#include "SuffixArray.h"
#include "../util/tokenize.hh"
#include <getopt.h>

using namespace std;

size_t lookup( string );
vector<string> tokenize( const char input[] );
SuffixArray suffixArray;

int main(int argc, char* argv[])
{
  // handle parameters
  string query;
  string fileNameSuffix;
  string fileNameSource;
  bool loadFlag = false;
  bool saveFlag = false;
  bool createFlag = false;
  bool queryFlag = false;
  bool querySentenceFlag = false;

  int stdioFlag = false;  // receive requests from STDIN, respond to STDOUT
  string info = "usage: biconcor\n\t[--load model-file]\n\t[--save model-file]\n\t[--create corpus]\n\t[--query string]\n\t[--stdio]\n";
  while(1) {
    static struct option long_options[] = {
      {"load", required_argument, 0, 'l'},
      {"save", required_argument, 0, 's'},
      {"create", required_argument, 0, 'c'},
      {"query", required_argument, 0, 'q'},
      {"query-sentence", required_argument, 0, 'Q'},
      {"document", required_argument, 0, 'd'},
      {"stdio", no_argument, 0, 'i'},
      {"stdio-sentence", no_argument, 0, 'I'},
      {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long (argc, argv, "l:s:c:q:Q:iId", long_options, &option_index);
    if (c == -1) break;
    switch (c) {
    case 'l':
      fileNameSuffix = string(optarg);
      loadFlag = true;
      break;
    case 's':
      fileNameSuffix = string(optarg);
      saveFlag = true;
      break;
    case 'c':
      fileNameSource = string(optarg);
      createFlag = true;
      break;
    case 'q':
      query = string(optarg);
      queryFlag = true;
      break;
    case 'Q':
      query = string(optarg);
      querySentenceFlag = true;
      break;
    case 'i':
      stdioFlag = true;
      break;
    case 'I':
      stdioFlag = true;
      querySentenceFlag = true;
      break;
    case 'd':
      suffixArray.UseDocument();
      break;
    default:
      cerr << info;
      exit(1);
    }
  }

  // check if parameter settings are legal
  if (saveFlag && !createFlag) {
    cerr << "error: cannot save without creating\n" << info;
    exit(1);
  }
  if (saveFlag && loadFlag) {
    cerr << "error: cannot load and save at the same time\n" << info;
    exit(1);
  }
  if (!loadFlag && !createFlag) {
    cerr << "error: neither load or create - i have no info!\n" << info;
    exit(1);
  }

  // get suffix array
  if (createFlag) {
    cerr << "will create\n";
    cerr << "corpus is in " << fileNameSource << endl;
    suffixArray.Create( fileNameSource );
    if (saveFlag) {
      suffixArray.Save( fileNameSuffix );
      cerr << "will save in " << fileNameSuffix << endl;
    }
  }
  if (loadFlag) {
    cerr << "will load from " << fileNameSuffix << endl;
    suffixArray.Load( fileNameSuffix );
  }

  // do something with it
  if (stdioFlag) {
    while(true) {
      string query;
      if (getline(cin, query, '\n').eof()) {
        return 0;
      }
      if (querySentenceFlag) {
        vector< string > queryString = util::tokenize( query.c_str() );
        suffixArray.PrintSentenceMatches( queryString );
      } else {
        cout << lookup( query ) << endl;
      }
    }
  } else if (queryFlag) {
    cout << lookup( query ) << endl;
  } else if (querySentenceFlag) {
    vector< string > queryString = util::tokenize( query.c_str() );
    suffixArray.PrintSentenceMatches( queryString );
  }
  return 0;
}

size_t lookup( string query )
{
  cerr << "query is " << query << endl;
  vector< string > queryString = util::tokenize( query.c_str() );
  return suffixArray.Count( queryString );
}
