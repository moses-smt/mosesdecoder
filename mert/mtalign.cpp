
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "util/exception.hh"
#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"

#include "Scorer.h"
#include "ScorerFactory.h"

#include <boost/unordered_map.hpp>

using namespace MosesTuning;

namespace po = boost::program_options;

const size_t MAX_NGRAM_ORDER = 4;

StringPiece operator+(const StringPiece& s1, const StringPiece& s2) {
  const char* start = std::min(s1.data(), s2.data()); 
  const char* end   = std::max(s1.data() + s1.length(), s2.data() + s2.length());
  size_t length = end - start;
  return StringPiece(start, length);
}  

class Sentence {
  public:
    Sentence(StringPiece sentence, size_t start, size_t length,
             std::vector<StringPiece>& tokens)
    : m_sentence(sentence), m_tokens(&tokens), m_start(start), m_length(length)
    {}
        
    StringPiece str() const {
      return m_sentence;
    }
    
    StringPiece operator[](size_t i) const {
      return (*m_tokens)[m_start + i];
    }
    
    size_t size() const {
      return m_length;
    }
    
    Sentence operator+(const Sentence& s) const {
      size_t start = std::min(m_start, s.m_start); 
      size_t end   = std::max(m_start + m_length, s.m_start + s.m_length);
      size_t length = end - start;
      return Sentence(m_sentence + s.m_sentence, start, length, *m_tokens);
    }
    
  private:
    StringPiece m_sentence;
    std::vector<StringPiece>* m_tokens;
    size_t m_start;
    size_t m_length;
};

inline std::ostream& operator<<(std::ostream& o, const Sentence& sentence) {
  return o << sentence.str();
}

class Corpus {
  public:
    
    Corpus(const std::string& fileName)
      : m_fileName(fileName) {
      std::ifstream fileStream(m_fileName.c_str());
      if (!fileStream.good())
        throw std::runtime_error("Error opening file:" + fileName);
      
      size_t currentPos = 0;
      std::vector<size_t> sentenceStarts;
      std::vector<size_t> tokenStarts;
      
      std::stringstream corpusStream;
      std::string line;
      while (std::getline(fileStream, line)) {
        sentenceStarts.push_back(currentPos);
        
        std::stringstream lineStream(line);
        std::string token;
        while(lineStream >> token) {
          tokenStarts.push_back(currentPos);
          corpusStream << token << " ";
          currentPos += token.size() + 1;
        }
      }
      
      m_corpus = corpusStream.str();
      
      size_t j = 0;
      for(size_t i = 0; i < sentenceStarts.size() - 1; i++) {
        size_t start = sentenceStarts[i];
        size_t length = sentenceStarts[i + 1] - start - 1;
        
        if(sentenceStarts[i] == sentenceStarts[i + 1])
          length = 0;
        
        size_t tokens = 0;
        while(tokenStarts[j + tokens] < start + length) {
          size_t tStart =  tokenStarts[j + tokens];
          size_t tLength = tokenStarts[j + tokens + 1] - tStart - 1;
          m_tokens.push_back(StringPiece(m_corpus.c_str() + tStart, tLength));
          tokens++;
        }
        
        Sentence sentence(StringPiece(m_corpus.c_str() + start, length),
                          j, tokens, m_tokens);
        m_sentences.push_back(sentence);
        
        j += tokens;
      }
      
      size_t start = sentenceStarts.back();
      size_t length = currentPos - start - 1;
      
      if(start == currentPos)
        length = 0;
      
      size_t tokens = 0;
      while(tokenStarts[j + tokens] < start + length && j + tokens < tokenStarts.size()) {
        size_t tStart =  tokenStarts[j + tokens];
        size_t tLength = tokenStarts[j + tokens + 1] - tStart - 1;
        
        if(j + tokens + 1 >= tokenStarts.size())
          tLength = currentPos - tStart - 1;
        
        m_tokens.push_back(StringPiece(m_corpus.c_str() + tStart, tLength));
        tokens++;
      }
      
      Sentence sentence(StringPiece(m_corpus.c_str() + start, length),
                        j, tokens, m_tokens);
      m_sentences.push_back(sentence);
    }
    
    const Sentence operator[](size_t i) const {
      return m_sentences[i];
    }
    
    size_t size() const {
      return m_sentences.size();
    }
    
    //size_t pos(size_t i) const {
    //  return m_sentences[i].data() - m_sentences[0].data();
    //}
    //
    //const NGramsPos& ngrams() const {
    //  return m_ngrams;
    //}
    
  private:
    std::string m_fileName;
    
    std::string m_corpus;
    std::vector<StringPiece> m_tokens;
    std::vector<Sentence> m_sentences;
};

class Stats {
  public:
    Stats()
    : m_stats(MAX_NGRAM_ORDER * 2 + 1, 0)
    {}
    
    float& operator[](size_t i) {
      return m_stats[i];
    }
    
    float& back() {
      return m_stats.back();
    }
    
    size_t size() const {
      return m_stats.size();
    }
    
    Stats& operator+=(Stats& o) {
      for(size_t i = 0; i < m_stats.size(); i++)
        m_stats[i] += o[i];
      return *this;
    }
    
    Stats operator+(Stats o) {
      Stats out;
      out += o;
      out += *this;
      return out;
    }
    
  private:
    std::vector<float> m_stats;
};


void computeBLEUstats(const Sentence& c, const Sentence& r, Stats& stats) {
  size_t cLen = c.size();
  size_t rLen = r.size();
  
  typedef std::pair<size_t, size_t> CountOrder;
  typedef boost::unordered_map<StringPiece, CountOrder> NGramCounts;
  
  NGramCounts ccounts;
  NGramCounts rcounts;
  
  for(size_t i = 0; i < cLen; i++) {
    for(size_t j = 0; j < MAX_NGRAM_ORDER && i + j < cLen; j++) {
      StringPiece ngram = c[i] + c[i + j];
      if(ccounts.count(ngram) > 0)
        ccounts[ngram].second++;
      else
        ccounts[ngram] = CountOrder(j, 1);
    }
  }
  
  for(size_t i = 0; i < rLen; i++) {
    for(size_t j = 0; j < MAX_NGRAM_ORDER && i + j < rLen; j++) {
      StringPiece ngram = r[i] + r[i + j];
      if(rcounts.count(ngram) > 0)
        rcounts[ngram].second++;
      else
        rcounts[ngram] = CountOrder(j, 1);
    }
  }
    
  for(NGramCounts::iterator it = ccounts.begin(); it != ccounts.end(); it++) {
    
    size_t order = it->second.first;
    size_t guess = it->second.second;
    size_t v = 0;
    if(rcounts.count(it->first))
      v = rcounts[it->first].second;
    size_t correct = std::min(v, guess);
    
    stats[order * 2]     += correct;
    stats[order * 2 + 1] += guess;
  }
  
  stats.back() = rLen;
}

Stats computeBLEUstats(const Sentence& c, const Sentence& r) {
  Stats stats;
  computeBLEUstats(c, r, stats);
  return stats;
}


float computeBLEU(Stats& stats) {
  UTIL_THROW_IF(stats.size() != MAX_NGRAM_ORDER * 2 + 1, util::Exception, "Error");

  float logbleu = 0.0;
  for (size_t i = 0; i < MAX_NGRAM_ORDER; ++i) {
    if (stats[2*i] == 0) {
      return 0.0;
    }
    logbleu += log(stats[2*i]) - log(stats[2*i+1]);

  }
  logbleu /= MAX_NGRAM_ORDER;
  // reflength divided by test length
  const float brevity = 1.0 - static_cast<float>(stats[MAX_NGRAM_ORDER * 2]) / stats[1];
  if (brevity < 0.0) {
    logbleu += brevity;
  }
  return exp(logbleu);
}

int main(int argc, char** argv)
{
  bool help;
  std::string sctype = "BLEU";
  std::string scconfig = "";
  std::string sourceFileName;
  std::string targetFileName;

  // Command-line processing follows pro.cpp
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
    ("sctype", po::value<std::string>(&sctype), "the scorer type (default BLEU)")
    ("scconfig,c", po::value<std::string>(&scconfig), "configuration string passed to scorer")
    ("source,s", po::value<std::string>(&sourceFileName)->required(), "source language file")
    ("target,t", po::value<std::string>(&targetFileName)->required(), "target language file")
  ;

  po::options_description cmdline_options;
  cmdline_options.add(desc);
  po::variables_map vm;
  
  try { 
    po::store(po::command_line_parser(argc,argv).
              options(cmdline_options).run(), vm);
    po::notify(vm);
  }
  catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl << std::endl;
    
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << desc << std::endl;
    exit(0);
  }
  
  if (help) {
    std::cout << "Usage: " + std::string(argv[0]) +  " [options]" << std::endl;
    std::cout << desc << std::endl;
    exit(0);
  }
  
  Corpus source(sourceFileName);
  Corpus target(targetFileName);
  
  Stats final;
  for(size_t i = 0; i < source.size() && i < target.size(); i++) {
    Stats stats;
    computeBLEUstats(source[i], target[i], stats);
    final = final + stats;
  }

  for(size_t i = 0; i < final.size(); i++) {
    std::cout << final[i] << " ";
  }
  std::cout << std::endl;
  
  std::cout << computeBLEU(final) << std::endl;
  
  std::vector< std::vector<Stats> > S(source.size(), std::vector<Stats>(target.size()));
  
  Stats empty;
  
  for(size_t i = 0; i < S.size(); i++) {
    for(size_t j = 0; j < S[i].size(); j++) {
      
      Stats a01 = (j > 0) ? S[i][j-1] : empty;
      Stats a10 = (i > 0) ? S[i-1][j] : empty;
      Stats a11 = (i > 0 && j > 0) ? S[i-1][j-1] + computeBLEUstats(source[i], target[j]) : empty;
      Stats a12 = (i > 0 && j > 1) ? S[i-1][j-2] + computeBLEUstats(source[i], target[j-1] + target[j]) : empty;
      Stats a21 = (i > 1 && j > 0) ? S[i-2][j-1] + computeBLEUstats(source[i-1] + source[i], target[j]) : empty;
            
      Stats bestStats;
      float bestBLEU = 0;
      float temp = 0;
      
      temp = computeBLEU(a01);
      if(temp > bestBLEU) {
        bestBLEU = temp;
        bestStats = a01;
      }

      temp = computeBLEU(a10);
      if(temp > bestBLEU) {
        bestBLEU = temp;
        bestStats = a10;
      }

      temp = computeBLEU(a11);
      if(temp > bestBLEU) {
        bestBLEU = temp;
        bestStats = a11;
      }

      temp = computeBLEU(a12);
      if(temp > bestBLEU) {
        bestBLEU = temp;
        bestStats = a12;
      }

      temp = computeBLEU(a21);
      if(temp > bestBLEU) {
        bestBLEU = temp;
        bestStats = a21;
      }
            
      S[i][j] = bestStats;
    }
  }
  
  std::cout << computeBLEU(S.back().back()) << std::endl;

}
