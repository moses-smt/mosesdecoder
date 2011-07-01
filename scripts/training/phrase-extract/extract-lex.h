#pragma once

#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>


//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
	std::stringstream stream(input);
	T ret;
	stream >> ret;
	return ret;
}


//! speeded up version of above
template<typename T>
inline void Scan(std::vector<T> &output, const std::vector< std::string > &input)
{
	output.resize(input.size());
	for (size_t i = 0 ; i < input.size() ; i++)
	{
		output[i] = Scan<T>( input[i] );
	}
}
	

inline void Tokenize(std::vector<std::string> &output
                     , const std::string& str
                     , const std::string& delimiters = " \t")
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    output.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

// speeded up version of above
template<typename T>
inline void Tokenize( std::vector<T> &output
										 , const std::string &input
										 , const std::string& delimiters = " \t")
{
	std::vector<std::string> stringVector;
	Tokenize(stringVector, input, delimiters);
	return Scan<T>(output, stringVector );
}

class WordCount
{
	friend std::ostream& operator<<(std::ostream&, const WordCount&);
public:
  const std::string *m_str;
  mutable float m_count;

  WordCount(const std::string *str, float count)
  :m_str(str)
  ,m_count(count)
  {}

  void AddCount(float incr) const;

  const std::string GetString() const
  { return *m_str; }
  const float GetCount() const
  { return m_count; }

	//! transitive comparison used for adding objects into FactorCollection
	inline bool operator<(const WordCount &other) const
	{ 
		return m_str < other.m_str;
	}
};

class Vocab
{
  std::set<std::string> m_coll;
public:
  const std::string *GetOrAdd(const std::string &word);
};

typedef std::set<WordCount> WordCountColl;

class ExtractLex
{
  Vocab m_vocab;
  std::map<WordCount, WordCountColl> m_collS2T, m_collT2S;

  void Process(const std::string *target, const std::string *source);
  void Process(const WordCount &in, const WordCount &out, std::map<WordCount, WordCountColl> &coll);
  void Output(const std::map<WordCount, WordCountColl> &coll, std::ofstream &outStream);

public:
  void Process(std::vector<std::string> &toksTarget, std::vector<std::string> &toksSource, std::vector<std::string> &toksAlign);
  void Output(std::ofstream &streamLexS2T, std::ofstream &streamLexT2S);

};

