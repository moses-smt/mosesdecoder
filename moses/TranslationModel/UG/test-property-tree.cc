#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// #include <stdio>
#include <string>

using namespace std;

int
main(int argc, char* argv[])
{
  using boost::property_tree::ptree;
  ptree pt;
  read_json(argv[1], pt);

  printf("%s\n", pt.get<std::string>("path").c_str());

}
