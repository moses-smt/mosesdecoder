#pragma once

#include <vector>
#include <string>
#include <fstream>

#include <boost/unordered_map.hpp>

#include "util/string_piece.hh"

StringPiece operator+(const StringPiece& s1, const StringPiece& s2) {
  const char* start = std::min(s1.data(), s2.data()); 
  const char* end   = std::max(s1.data() + s1.length(), s2.data() + s2.length());
  size_t length = end - start;
  return StringPiece(start, length);
}

typedef std::vector<StringPiece> NGramsByOrder;
typedef std::vector<NGramsByOrder> NGrams;    

const size_t MAX_NGRAM_ORDER = 2;

class Sentence {
  public:
    Sentence(std::vector<StringPiece>& tokens)
    : m_sentence(StringPiece()), m_tokens(&tokens), m_start(0), m_length(0)
    {}
    
    Sentence(StringPiece sentence, size_t start, size_t length,
             std::vector<StringPiece>& tokens)
    : m_sentence(sentence), m_tokens(&tokens), m_start(start), m_length(length)
    {
      CollectNGrams();
    }
    
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
    
    void CollectNGrams() {
      if(m_ngrams.empty()) {
        m_ngrams.resize(MAX_NGRAM_ORDER);
        for(size_t i = 0; i < size(); i++) {
          for(size_t j = 0; j < MAX_NGRAM_ORDER && i + j < size(); j++) {
            StringPiece ngram = (*this)[i] + (*this)[i + j];
            m_ngrams[j].push_back(ngram);
          }
        }
        for(size_t i = 0; i < MAX_NGRAM_ORDER; i++)
          std::sort(m_ngrams[i].begin(), m_ngrams[i].end());
      }
    }
    
    const NGrams& ngrams() const {
      return m_ngrams;
    }
    
  private:
    StringPiece m_sentence;
    std::vector<StringPiece>* m_tokens;
    size_t m_start;
    size_t m_length;

    NGrams m_ngrams;
};

inline std::ostream& operator<<(std::ostream& o, const Sentence& sentence) {
  return o << sentence.str();
}

class Corpus {
  public:
    
    typedef std::pair<size_t, size_t> Range;
    typedef boost::unordered_map<Range, Sentence*> Ranges;
    
    Corpus(const std::string& fileName)
      : m_fileName(fileName), m_emptySentence(m_tokens) {
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
        size_t start  = sentenceStarts[i];
        size_t length = sentenceStarts[i + 1] - start - 1;
        
        if(sentenceStarts[i] == sentenceStarts[i + 1])
          length = 0;
        
        size_t tokens = 0;
        while(j + tokens < tokenStarts.size() && tokenStarts[j + tokens] < start + length) {
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
    
    const Sentence& operator()(int i, int j) const {
      return const_cast<Corpus&>(*this)(i, j);
    }
    
    const Sentence& operator()(int i, int j) {
      if(i < 0 || j < 0)
        return m_emptySentence;
      
      if(i == j) {
        return m_sentences[i];
      }
      else if(j > i) {
        Range range(i, j);
        Ranges::iterator it = m_ranges.find(range);
        if(it != m_ranges.end()) {
          return *it->second;
        }
        else {
          Sentence* sentenceRange = new Sentence(m_sentences[i] + m_sentences[j]);
          m_ranges[range] = sentenceRange;
          return *sentenceRange;
        }
      }
      else {
        return m_emptySentence;
      }
    }
    
    const Sentence& operator[](int i) const {
      if(i < 0)
        return m_emptySentence;
      return m_sentences[i];
    }
    
    size_t size() const {
      return m_sentences.size();
    }
    
  private:
    std::string m_fileName;
    
    std::string m_corpus;
    std::vector<StringPiece> m_tokens;
    std::vector<Sentence> m_sentences;
    Sentence m_emptySentence;
    Ranges m_ranges;
};
