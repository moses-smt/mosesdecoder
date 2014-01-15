#include <iostream>
#include <cstdlib>
#include <boost/program_options.hpp>
#include "moses/Util.h"
#include "DeEn.h"

using namespace std;

bool g_debug = false;

Phrase Tokenize(const string &line);

int main(int argc, char** argv)
{
  cerr << "Starting" << endl;

  namespace po = boost::program_options;
  po::options_description desc("Options");
  desc.add_options()
    ("help", "Print help messages")
    ("add", "additional options")
    ("source-language,s", po::value<string>()->required(), "Source Language")
    ("target-language,t", po::value<string>()->required(), "Target Language");

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argc, argv, desc),
              vm); // can throw

    /** --help option
     */
    if ( vm.count("help")  )
    {
      std::cout << "Basic Command Line Parameter App" << std::endl
                << desc << std::endl;
      return EXIT_SUCCESS;
    }

    po::notify(vm); // throws on error, so do after help in case
                    // there are any problems
  }
  catch(po::error& e)
  {
    std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
    std::cerr << desc << std::endl;
    return EXIT_FAILURE;
  }

  string sourceLang = vm["source-language"].as<string>();
  string targetLang = vm["target-language"].as<string>();
  cerr << sourceLang << " " << targetLang << endl;

  string line;
  size_t lineNum = 1;

  while (getline(cin, line)) {
    cerr << lineNum << ":" << line << endl;
    Phrase source = Tokenize(line);

    LabelDeEn(source, cout);

    ++lineNum;
  }



  cerr << "Finished" << endl;
  return EXIT_SUCCESS;
}

Phrase Tokenize(const string &line)
{
  Phrase ret;

  vector<string> toks = Moses::Tokenize(line);
  for (size_t i = 0; i < toks.size(); ++i) {
    Word word = Moses::Tokenize(toks[i], "|");
    ret.push_back(word);
  }

  return ret;
}

