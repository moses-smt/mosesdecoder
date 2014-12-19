
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
#include <iterator>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "util/exception.hh"
#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"


namespace po = boost::program_options;

StringPiece operator+(const StringPiece& s1, const StringPiece& s2) {
  const char* start = std::min(s1.data(), s2.data()); 
  const char* end   = std::max(s1.data() + s1.length(), s2.data() + s2.length());
  size_t length = end - start;
  return StringPiece(start, length);
}

typedef std::vector<StringPiece> NGramsByOrder;
typedef std::vector<NGramsByOrder> NGrams;    

const size_t MAX_NGRAM_ORDER = 4;

class Sentence {
  public:
    Sentence(std::vector<StringPiece>& tokens)
    : /*m_id(0),*/ m_sentence(StringPiece()), m_tokens(&tokens), m_start(0), m_length(0)
    {}
    
    Sentence(StringPiece sentence, size_t start, size_t length,
             std::vector<StringPiece>& tokens)
    : /*m_id(0),*/ m_sentence(sentence), m_tokens(&tokens), m_start(start), m_length(length)
    {
      CollectNGrams();
    }
        
    //void setId(size_t id) {
    //  m_id = id;  
    //}
    //
    //size_t getId() const {
    //  return m_id;
    //}
    
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
    //size_t m_id;
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
        //m_sentences.back().setId(m_sentences.size());
        
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
      //m_sentences.back().setId(m_sentences.size());
    }
    
    const Sentence& operator()(size_t i, size_t j) {
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
          //sentenceRange->setId(m_sentences.size() + m_ranges.size());
          return *sentenceRange;
        }
      }
      else {
        return m_emptySentence;
      }
    }
    
    const Sentence& operator[](size_t i) const {
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

template <size_t MAX_NGRAM_ORDER = 4, bool SMOOTH = true>
class BLEU {      
  public:    
    template <class Sentence>
    float operator()(const Sentence& c, const Sentence& r) const {
      if(c.size() == 0 || r.size() == 0)
        return 0;
      
      std::vector<float> stats(MAX_NGRAM_ORDER * 3, 0);
      computeBLEUSymStats(c, r, stats);
      return computeBLEUSym(stats);
    }
    
    float computeBLEUSym(const std::vector<float>& stats) const {
      UTIL_THROW_IF(stats.size() != MAX_NGRAM_ORDER * 3, util::Exception, "Error");
    
      float logbleu1 = 0.0;
      float logbleu2 = 0.0;
      
      float smoothing = 0;
      for (size_t i = 0; i < MAX_NGRAM_ORDER; ++i) {
        
        if(SMOOTH && i > 0)
          smoothing = 1;
      
        if(stats[3*i+1] + smoothing == 0)
          return 0.0;
      
        logbleu1 += log(stats[3*i] + smoothing) - log(stats[3*i+1] + smoothing);
        logbleu2 += log(stats[3*i] + smoothing) - log(stats[3*i+2] + smoothing);
      }
      logbleu1 /= MAX_NGRAM_ORDER;
      logbleu2 /= MAX_NGRAM_ORDER;
      
      // reflength divided by test length
      const float brevity1 = 1.0 - static_cast<float>(stats[2]) / stats[1];
      if (brevity1 < 0.0) {
        logbleu1 += brevity1;
      }
      const float brevity2 = 1.0 - static_cast<float>(stats[1]) / stats[2];
      if (brevity2 < 0.0) {
        logbleu2 += brevity2;
      }
    
      return exp((logbleu1 + logbleu2)/2);
    }
    
    template <class Sentence>
    void computeBLEUSymStats(const Sentence& c, const Sentence& r,
                             std::vector<float>& stats) const {
      
      for(size_t i = 0; i < MAX_NGRAM_ORDER; i++) {
        size_t correct = 0;
        
        // Check for common n-grams if there where common (n-1)-grams
        if(i == 0 || (i > 0 && stats[(i - 1) * 3] > 0)) 
          countCommon(c.ngrams()[i], r.ngrams()[i], correct);
        
        stats[i * 3]     += correct;
        stats[i * 3 + 1] += c.ngrams()[i].size();
        stats[i * 3 + 2] += r.ngrams()[i].size();
      }
    }
    
    size_t numStats() {
      return MAX_NGRAM_ORDER * 3;
    }

  private:
    
    template <class SortedNGrams>
    void countCommon(const SortedNGrams& n1,
                     const SortedNGrams& n2,
                     size_t& common) const {
      NGramsByOrder::const_iterator it1 = n1.begin();
      NGramsByOrder::const_iterator it2 = n2.begin();
      
      common = 0;
      while (it1 != n1.end() && it2 != n2.end()) {
        if (*it1 < *it2) {
          ++it1;
        }
        else {
          if (!(*it2 < *it1)) {
            ++common;
            ++it1;
          }
          ++it2;
        }
      }
    }    
};

struct SearchType {
  inline size_t& operator[](size_t i) {
    return pair[i];
  }

  inline const size_t& operator[](size_t i) const {
    return pair[i];
  }
  
  size_t pair[2];
};

typedef std::vector<SearchType> SearchTypes;

struct Search {
  public:
    Search(SearchType array[], size_t n)
    {
      std::copy(array, array + n,
          std::back_inserter(m_searchTypes));
    }
    
    virtual const SearchTypes& operator()() const {
      return m_searchTypes;
    }
  
  protected:
    SearchTypes m_searchTypes;
};

SearchType fastTypes[] = {{0,1}, {1,0}, {1,1}};
struct Fast : public Search {
  Fast() : Search(fastTypes, sizeof(fastTypes)/sizeof(SearchType)) {}
};

SearchType fullTypes[] = {{0,1}, {1,0}, {1,1}, {1,2}, {2,1},
                          {2,2}, {1,3}, {3,1}, {2,3}, {3,2},
                          {1,4}, {4,1}};
struct Full : public Search {
  Full() : Search(fullTypes, sizeof(fullTypes)/sizeof(SearchType)) {}
};

template <class ScorerType, class SearchType>
class Config {
  public:
    const ScorerType& scorer() const {
      return m_scorer;
    }
    
    const SearchType& search() const {
      return m_search;
    }
    
  private:
    ScorerType m_scorer;
    SearchType m_search;
};

struct Rung {
  size_t i;
  size_t j;

  size_t iType;
  size_t jType;
};

typedef std::vector<Rung> Rungs;

template<class ConfigType, class CorpusType>
class Dynamic {
  public:
    Dynamic(CorpusType& corpus1, CorpusType& corpus2)
    : m_corpus1(corpus1), m_corpus2(corpus2),
      m_seen(m_corpus1.size() + 1, std::vector<float>(m_corpus2.size() + 1, -100)),
      m_prev(m_corpus1.size() + 1, std::vector<SearchType>(m_corpus2.size() + 1))
    {}
    
    void align() {
      align(m_corpus1.size(), m_corpus2.size());
    }
    
    float align(size_t i, size_t j) {
      if(i == 0 || j == 0)
        return 0;
      
      if(m_seen[i][j] != -100)
        return m_seen[i][j];
      
      SearchTypes searchTypes = m_config.search()();
      
      float bestScore = 0;
      SearchType bestSearchType = searchTypes[0];
      
      for(size_t k = 0; k < searchTypes.size(); k++) {
        size_t iType = searchTypes[k][0];
        size_t jType = searchTypes[k][1];
        
        float score = -10;
        if(i >= iType && j >= jType) {
          score = align(i-iType, j-jType)
                   + m_config.scorer()(m_corpus1(i-iType, i-1), m_corpus2(j-jType, j-1));
        
          if(score > bestScore) {
            bestScore = score;
            bestSearchType = searchTypes[k];
          }
        }
      }
      
      m_seen[i][j] = bestScore;
      m_prev[i][j] = bestSearchType;
      return bestScore;    
    }
  
    std::vector<Rung> backTrack() {
      Rungs rungs;
      backTrack(m_corpus1.size(), m_corpus2.size(), rungs);
      return rungs;
    }
  
    void backTrack(size_t i, size_t j, Rungs& rungs) {
      SearchType p = m_prev[i][j];
      if(i > p[0] && j > p[1]) 
        backTrack(i - p[0], j - p[1], rungs);
      
      Rung rung;
      rung.i = i;
      rung.j = j;
      rung.iType = p[0];
      rung.jType = p[1];
      rungs.push_back(rung);
    }

    
  private:
    ConfigType m_config;
    CorpusType& m_corpus1;
    CorpusType& m_corpus2;
    
    std::vector< std::vector<float> > m_seen;
    std::vector< std::vector<SearchType> > m_prev;
};

typedef Config<BLEU<2>, Fast> FastBLEU2;
typedef Config<BLEU<2>, Full> FullBLEU2;

typedef Dynamic<FastBLEU2, Corpus> FastBLEU2Aligner;
typedef Dynamic<FullBLEU2, Corpus> FullBLEU2Aligner;

int main(int argc, char** argv)
{
  bool help;
  
  std::string sourceFileName;
  std::string targetFileName;

  std::string sourceFileNameOrig;
  std::string targetFileNameOrig;

  bool ladder;
  
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", po::value(&help)->zero_tokens()->default_value(false), "Print this help message and exit")
    ("source,s", po::value<std::string>(&sourceFileName)->required(), "source language file (Processed)")
    ("target,t", po::value<std::string>(&targetFileName)->required(), "target language file (Processed)")
    ("Source,S", po::value<std::string>(&sourceFileNameOrig), "source language file (Original), if given will replace output of --source")
    ("Target,T", po::value<std::string>(&targetFileNameOrig), "target language file (Original), if given will replace output of --target")
    ("ladder,l", po::value(&ladder)->zero_tokens()->default_value(false), "Output in hunalign ladder format")
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
  
  FullBLEU2Aligner fb2Align(source, target);
  fb2Align.align();
  Rungs rungs = fb2Align.backTrack();
  
  float bleuSum = 0;
  size_t keptRungs = 0;
  
  BLEU<2> scorer;
  BLEU<2, false> docScorer; 
  std::vector<float> stats(docScorer.numStats(), 0);
  
  if(ladder && rungs.size() > 0) {
    Rung r = rungs[0];
    if(r.i - r.iType != 0 || r.j - r.jType != 0)
      std::cout << 0 << "\t" << 0 << "\t" << -1 << std::endl;
  }
  for(size_t i = 0; i < rungs.size(); i++) {
    Rung r = rungs[i];
    if(r.iType && r.jType) {
      const Sentence& s1 = source(r.i - r.iType, r.i - 1);
      const Sentence& s2 = target(r.j - r.jType, r.j - 1);
      float bleu = scorer(s1, s2);
      
      docScorer.computeBLEUSymStats(s1, s2, stats);
      
      if(ladder) { 
        std::cout << r.i-r.iType << "\t" << r.j-r.jType << "\t" << bleu << std::endl;
      }
      else {
        std::cout << r.iType << "-" << r.jType << "\t" << bleu <<  "\t" << s1 << "\t" << s2 << std::endl;
      }
            
      bleuSum += bleu;
      keptRungs++;
    }
    else if(ladder) {
        std::cout << r.i-r.iType << "\t" << r.j-r.jType << "\t" << -1 << std::endl;
    }
  }
  if(ladder) {
    std::cout << source.size() << "\t" << target.size() << "\t" << 0 << std::endl;
  }
  
  std::cerr << "Document BLEU-2: " << docScorer.computeBLEUSym(stats) << std::endl;
  std::cerr << "Quality " << bleuSum/keptRungs << "/" << bleuSum/rungs.size() << std::endl;
}
