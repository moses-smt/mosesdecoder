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
	void Load(const string filename);

public:
	Desegmenter(const std::string& file){
		filename = file; 
		Load(filename);//, mmDetok);
	}
	string getFileName(){ return filename; }
	
	vector<string> Search(string myKey);
	string ApplyRules(string &);

	~Desegmenter();
};


/*class Completer
{
private:
	//std::multimap<string, string,std::less< std::string > > mmDetok;
	std::map<string, string> mmDetok;
	std::string filename;
	void Load(const string filename);

public:
	Completer(const std::string& file){
		filename = file; 
		Load(filename);//, mmDetok);
	}
	string getFileName(){ return filename; }	
	string Search(string myKey);

	~Completer();
};
*/

}
