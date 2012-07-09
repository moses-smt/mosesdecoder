#include "StringUtils.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>


bool replace(std::string& str, const std::string& from, const std::string& to) {
  int match = 0;
  while(true){
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos){
      if (match == 0) return false;
      break;
    }
    str.replace(start_pos, from.length(), to);
    match++;
  }
  return true;
}


string int2string(const int a){
  std::stringstream ss;
  std::string str;
  ss << a;
  ss >> str;
  return str;
}

string float2string(const float a){
  std::stringstream ss;
  std::string str;
  ss << a;
  ss >> str;
  return str;
}

int string2int(const string s){
  return std::atoi(s.c_str());
}

double string2double(const string s){
  return std::atof(s.c_str());
}

float string2float(const string s){
  return std::atof(s.c_str());
}

string joinStrings(vector<string> &elements, //!< elements to join
		   string delim //!< delimiter to use
		   ){
  string joinedString = "";
  for(int i = 0; i < elements.size(); i++){
    joinedString += elements[i];
    if (i+1 != elements.size())
      joinedString += delim;
  }
  return joinedString;
}

int StringUtils::SplitString(const string& input, 
			     const string& delimiter,
			     vector<string>& results, 
			     bool includeEmpties){
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = (int)delimiter.size();
  int isize = (int)input.size();

  if( isize == 0 || sizeS2 == 0 ){
      return 0;
  }

  vector<int> positions;
  newPos = input.find (delimiter, 0);
  if( newPos < 0 ){
    results.push_back(input);
    return 0; 
  }

  int numFound = 0;
  while( newPos >= iPos ){
    numFound++;
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.find (delimiter, iPos+sizeS2);
  }

  if( numFound == 0 ){
    return 0;
  }

  for( int i=0; i <= (int)positions.size(); ++i ){
    string s("");
    if( i == 0 ){ 
      s = input.substr( i, positions[i] ); 
    }
    int offset = positions[i-1] + sizeS2;
    if( offset < isize ){
      if( i == positions.size() ){
	s = input.substr(offset);
      }
      else if( i > 0 ){
	s = input.substr( positions[i-1] + sizeS2, 
			  positions[i] - positions[i-1] - sizeS2 );
      }
    }
    if( includeEmpties || ( s.size() > 0 ) ){
      results.push_back(s);
    }
  }
  return numFound;
}


string substitutechar(string input,char oldchar,char newchar){
  for(int i = 0; i < input.size(); i++){
    if (input[i] == oldchar){
      input[i] = newchar;
    }
  }
  return input;
}

bool containspace(string s){
  if (s.find(" ") == string::npos){
    return false;
  }else{
    return true;
  }
}


vector<string> tokenizeString(const string& input, 
			     const string& delimiter,
			     bool includeEmpties){
  vector<string> results;
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = (int)delimiter.size();
  int isize = (int)input.size();

  if( isize == 0 || sizeS2 == 0 ){
    cerr << "empty string or delimiter" << endl;
    return results;
  }

  vector<int> positions;
  newPos = input.find (delimiter, 0);
  if( newPos < 0 ){
    results.push_back(input);
    return results; 
  }

  int numFound = 0;
  while( newPos >= iPos ){
    numFound++;
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.find (delimiter, iPos+sizeS2);
  }

  if( numFound == 0 ){
    return results;
  }

  for( int i=0; i <= (int)positions.size(); ++i ){
    string s("");
    if( i == 0 ){ 
      s = input.substr( i, positions[i] ); 
    }
    int offset = positions[i-1] + sizeS2;
    if( offset < isize ){
      if( i == positions.size() ){
	s = input.substr(offset);
      }
      else if( i > 0 ){
	s = input.substr( positions[i-1] + sizeS2, 
			  positions[i] - positions[i-1] - sizeS2 );
      }
    }
    if( includeEmpties || ( s.size() > 0 ) ){
      results.push_back(s);
    }
  }
  return results;
}
