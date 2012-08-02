#ifndef moses_LexicalReorderingTableCreator_h
#define moses_LexicalReorderingTableCreator_h

#include "PhraseTableCreator.h"

namespace Moses {

class LexicalReorderingTableCreator {
  private:
    std::string m_inPath;
    std::string m_outPath;
    
    std::FILE* m_outFile;
    
    size_t m_orderBits;
    size_t m_fingerPrintBits;
    
    size_t m_numScoreComponent;
    
    bool m_multipleScoreTrees;
    bool m_quantize;
    
    std::string m_separator;
    
    BlockHashIndex m_hash;
    
#ifdef WITH_THREADS    
    size_t m_threads;
#endif
    
    typedef Counter<float> ScoreCounter;
    typedef CanonicalHuffman<float> ScoreTree;  
  
    std::vector<ScoreCounter*> m_scoreCounters;
    std::vector<ScoreTree*> m_scoreTrees;
    
    StringVector<unsigned char, unsigned long, MmapAllocator> m_encodedScores;
    StringVector<unsigned char, unsigned long, MmapAllocator> m_compressedScores;
    
    std::priority_queue<PackedItem> m_queue;
    long m_lastFlushedLine;
    long m_lastFlushedSourceNum;
    std::string m_lastFlushedSourcePhrase;
    std::vector<std::string> m_lastRange;
    
    void PrintInfo();
    
    void EncodeScores();
    void CalcHuffmanCodes();
    void CompressScores();
    void Save();
    
    std::string MakeSourceTargetKey(std::string&, std::string&);
    
    std::string EncodeLine(std::vector<std::string>& tokens);
    void AddEncodedLine(PackedItem& pi);
    void FlushEncodedQueue(bool force = false);
    
    std::string CompressEncodedScores(std::string &encodedScores);
    void AddCompressedScores(PackedItem& pi);
    void FlushCompressedQueue(bool force = false);
    
  public:
    LexicalReorderingTableCreator(std::string inPath,
                                  std::string outPath,
                                  size_t numScoreComponent = 6,
                                  size_t orderBits = 10,
                                  size_t fingerPrintBits = 16,
                                  bool multipleScoreTrees = true,
                                  size_t quantize = 0
#ifdef WITH_THREADS
                                  , size_t threads = 2
#endif   
                                  );
    
  friend class EncodingTaskReordering;
  friend class CompressionTaskReordering;
};

class EncodingTaskReordering
{
  private:
#ifdef WITH_THREADS
    static boost::mutex m_mutex;
    static boost::mutex m_fileMutex;
#endif
    static size_t m_lineNum;
    static size_t m_sourcePhraseNum;
    static std::string m_lastSourcePhrase;
    
    InputFileStream& m_inFile;
    LexicalReorderingTableCreator& m_creator;
    
  public:
    EncodingTaskReordering(InputFileStream& inFile, LexicalReorderingTableCreator& creator);
    void operator()();
};

class CompressionTaskReordering
{
  private:
#ifdef WITH_THREADS
    static boost::mutex m_mutex;
#endif
    static size_t m_scoresNum;
    StringVector<unsigned char, unsigned long, MmapAllocator> &m_encodedScores;
    LexicalReorderingTableCreator &m_creator;
    
  public:
    CompressionTaskReordering(StringVector<unsigned char, unsigned long, MmapAllocator>&
                    m_encodedScores, LexicalReorderingTableCreator& creator);
    void operator()();
};

}

#endif
