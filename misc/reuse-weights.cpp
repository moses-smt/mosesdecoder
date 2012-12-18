#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
#include <vector>

using namespace std;

std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters = " \t");

int main(int argc, char **argv)
{
	assert(argc == 2);
	ifstream newWeightStrme(argv[1]);

	map<string, string> newWeights, oldWeights;
		
	// ini file with new weights
	bool inWeightSection = false;
	string line;
	while (getline(newWeightStrme, line)) {
		if (line.substr(0, 1) == "["
				&& line.substr(line.size() -1, 1) == "]") {
			if (line == "[weight]") {
				inWeightSection = true;
			}
			else {
				inWeightSection = false;
			}
		}
		else if (inWeightSection) {
			vector<string> toks = Tokenize(line, "=");
			assert(toks.size() == 2);
			newWeights[ toks[0] ] = toks[1];
		}
	}
	
	// original ini file with old weights
	inWeightSection = false;
  while (getline(cin, line)) {
		if (line.substr(0, 1) == "["
				&& line.substr(line.size() -1, 1) == "]") {
			if (line == "[weight]") {
				inWeightSection = true;
			}
			else {
				inWeightSection = false;
			}
		}
		else if (inWeightSection) {
			if (line.size() > 0 && line.substr(0,1) != "#") {			  
				vector<string> toks = Tokenize(line, "=");
				assert(toks.size() == 2);
				oldWeights[ toks[0] ] = toks[1];
			}
		}
	
		if (!inWeightSection) {
			cout << line << endl;
		}
	}
	
	// merge weights
	map<string, string>::iterator iterNew, iterOld;
	for (iterOld = oldWeights.begin(); iterOld != oldWeights.end(); ++iterOld) {
		const string &key = iterOld->first;
		iterNew = newWeights.find(key);
		if (iterNew == newWeights.end()) {
		  // not in new weights. add
			newWeights[key] = iterOld->second;
		}
	}
	
	// add to new ini file
	cout << "[weight]" << endl;
	for (iterNew = newWeights.begin(); iterNew != newWeights.end(); ++iterNew) {
		const string &key = iterNew->first;
		const string &value = iterNew->second;
		cout << key << "=" << value << endl;
	}
}



std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters)
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}
