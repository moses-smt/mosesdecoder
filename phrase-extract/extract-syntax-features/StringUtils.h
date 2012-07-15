#ifndef __STRINGUTILS_HPP_
#define __STRINGUTILS_HPP_

#include <string>
#include <vector>

using namespace std;

class StringUtils{
 public:
  static int SplitString(const string& input, 
			 const string& delimiter, vector<string>& results, 
			 bool includeEmpties = true);
};

vector<string> tokenizeString(const string& input, const string& delimiter, bool includeEmpties=true);

/**
 * \brief builds a new string by concatenating all the strings in the
 * elements vector, using delim as a separator.  just like a Perl
 * join.
 * @return if element = [a,b,c] and delim = " ", the returned string is "a b c". 
 */
string joinStrings(vector<string> &elements, //!< elements to join
		   string delim //!< delimiter to use
		   );

string float2string(const float a);

/**
 * \brief converts an integer to a string type.
 */
string int2string(const int a);

/**
 * \brief converts a string to an integer.
 */
int string2int(const string s);

double string2double(const string s);

float string2float(const string s);

string substitutechar(string input,char oldchar,char newchar);

bool containspace(string s);

bool replace(std::string& str, const std::string& from, const std::string& to);

#endif

