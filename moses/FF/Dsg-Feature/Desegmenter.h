#pragma once

#include<string>
#include<map>


using namespace std;

namespace Moses
{
class Desegmenter
{
private:
  std::multimap<string, string> mmDesegTable;
  std::string filename;
  bool simple;
  void Load(const string filename);

public:
  Desegmenter(const std::string& file, const bool scheme) {
    filename = file;
    simple=scheme;
    Load(filename);
  }
  string getFileName() {
    return filename;
  }

  vector<string> Search(string myKey);
  string ApplyRules(string &);
  ~Desegmenter();
};
}
