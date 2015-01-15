// $Id$
// vim:tabstop=2
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <cstdio>

#include "PhraseTableCreator.h"
#include "ConsistentPhrases.h"
#include "ThrowingFwrite.h"
#include "util/file.hh"
#include "util/exception.hh"

namespace Moses
{

bool operator<(const PackedItem &pi1, const PackedItem &pi2)
{
  if(pi1.GetLine() < pi2.GetLine())
    return false;
  return true;
}

std::string PhraseTableCreator::m_phraseStopSymbol = "__SPECIAL_STOP_SYMBOL__";
std::string PhraseTableCreator::m_separator = "|||";

PhraseTableCreator::PhraseTableCreator(std::string inPath,
                                       std::string outPath,
                                       std::string tempfilePath,
                                       size_t numScoreComponent,
                                       size_t sortScoreIndex,
                                       Coding coding,
                                       size_t orderBits,
                                       size_t fingerPrintBits,
                                       bool useAlignmentInfo,
                                       bool multipleScoreTrees,
                                       size_t quantize,
                                       size_t maxRank,
                                       bool warnMe
#ifdef WITH_THREADS
                                       , size_t threads
#endif
                                      )
  : m_inPath(inPath), m_outPath(outPath), m_tempfilePath(tempfilePath),
    m_outFile(std::fopen(m_outPath.c_str(), "w")), m_numScoreComponent(numScoreComponent),
    m_sortScoreIndex(sortScoreIndex), m_warnMe(warnMe),
    m_coding(coding), m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
    m_useAlignmentInfo(useAlignmentInfo),
    m_multipleScoreTrees(multipleScoreTrees),
    m_quantize(quantize), m_maxRank(maxRank),
#ifdef WITH_THREADS
    m_threads(threads),
    m_srcHash(m_orderBits, m_fingerPrintBits, 1),
    m_rnkHash(10, 24, m_threads),
#else
    m_srcHash(m_orderBits, m_fingerPrintBits),
    m_rnkHash(m_orderBits, m_fingerPrintBits),
#endif
    m_maxPhraseLength(0),
    m_lastFlushedLine(-1), m_lastFlushedSourceNum(0),
    m_lastFlushedSourcePhrase("")
{
  PrintInfo();

  AddTargetSymbolId(m_phraseStopSymbol);

  size_t cur_pass = 1;
  size_t all_passes = 2;
  if(m_coding == PREnc)
    all_passes = 3;

  m_scoreCounters.resize(m_multipleScoreTrees ? m_numScoreComponent : 1);
  for(std::vector<ScoreCounter*>::iterator it = m_scoreCounters.begin();
      it != m_scoreCounters.end(); it++)
    *it = new ScoreCounter();
  m_scoreTrees.resize(m_multipleScoreTrees ? m_numScoreComponent : 1);

  // 0th pass
  if(m_coding == REnc) {
    size_t found = inPath.find_last_of("/\\");
    std::string path;
    if(found != std::string::npos)
      path = inPath.substr(0, found);
    else
      path = ".";
    LoadLexicalTable(path + "/lex.f2e");
  } else if(m_coding == PREnc) {
    std::cerr << "Pass " << cur_pass << "/" << all_passes << ": Creating hash function for rank assignment" << std::endl;
    cur_pass++;
    CreateRankHash();
  }

  // 1st pass
  std::cerr << "Pass " << cur_pass << "/" << all_passes << ": Creating source phrase index + Encoding target phrases" << std::endl;
  m_srcHash.BeginSave(m_outFile);

  if(tempfilePath.size()) {
    MmapAllocator<unsigned char> allocEncoded(util::FMakeTemp(tempfilePath));
    m_encodedTargetPhrases = new StringVector<unsigned char, unsigned long, MmapAllocator>(allocEncoded);
  } else {
    m_encodedTargetPhrases = new StringVector<unsigned char, unsigned long, MmapAllocator>();
  }
  EncodeTargetPhrases();

  cur_pass++;

  std::cerr << "Intermezzo: Calculating Huffman code sets" << std::endl;
  CalcHuffmanCodes();

  // 2nd pass
  std::cerr << "Pass " << cur_pass << "/" << all_passes << ": Compressing target phrases" << std::endl;

  if(tempfilePath.size()) {
    MmapAllocator<unsigned char> allocCompressed(util::FMakeTemp(tempfilePath));
    m_compressedTargetPhrases = new StringVector<unsigned char, unsigned long, MmapAllocator>(allocCompressed);
  } else {
    m_compressedTargetPhrases = new StringVector<unsigned char, unsigned long, MmapAllocator>();
  }
  CompressTargetPhrases();

  std::cerr << "Saving to " << m_outPath << std::endl;
  Save();
  std::cerr << "Done" << std::endl;
  std::fclose(m_outFile);
}

PhraseTableCreator::~PhraseTableCreator()
{
  delete m_symbolTree;
  if(m_useAlignmentInfo)
    delete m_alignTree;
  for(size_t i = 0; i < m_scoreTrees.size(); i++) {
    delete m_scoreTrees[i];
    delete m_scoreCounters[i];
  }

  delete m_encodedTargetPhrases;
  delete m_compressedTargetPhrases;
}

void PhraseTableCreator::PrintInfo()
{
  std::string encodings[3] = {"Huffman", "Huffman + REnc", "Huffman + PREnc"};

  std::cerr << "Used options:" << std::endl;
  std::cerr << "\tText phrase table will be read from: " << m_inPath << std::endl;
  std::cerr << "\tOutput phrase table will be written to: " << m_outPath << std::endl;
  std::cerr << "\tStep size for source landmark phrases: 2^" << m_orderBits << "=" << (1ul << m_orderBits) << std::endl;
  std::cerr << "\tSource phrase fingerprint size: " << m_fingerPrintBits << " bits / P(fp)=" << (float(1)/(1ul << m_fingerPrintBits)) << std::endl;
  std::cerr << "\tSelected target phrase encoding: " << encodings[m_coding] << std::endl;
  if(m_coding == PREnc) {
    std::cerr << "\tMaxiumum allowed rank for PREnc: ";
    if(!m_maxRank)
      std::cerr << "unlimited" << std::endl;
    else
      std::cerr << m_maxRank << std::endl;
  }
  std::cerr << "\tNumber of score components in phrase table: " << m_numScoreComponent << std::endl;
  std::cerr << "\tSingle Huffman code set for score components: " << (m_multipleScoreTrees ? "no" : "yes") << std::endl;
  std::cerr << "\tUsing score quantization: ";
  if(m_quantize)
    std::cerr << m_quantize << " best" << std::endl;
  else
    std::cerr << "no" << std::endl;
  std::cerr << "\tExplicitly included alignment information: " << (m_useAlignmentInfo ? "yes" : "no") << std::endl;

#ifdef WITH_THREADS
  std::cerr << "\tRunning with " << m_threads << " threads" << std::endl;
#endif
  std::cerr << std::endl;
}

void PhraseTableCreator::Save()
{
  // Save type of encoding
  ThrowingFwrite(&m_coding, sizeof(m_coding), 1, m_outFile);
  ThrowingFwrite(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, m_outFile);
  ThrowingFwrite(&m_useAlignmentInfo, sizeof(m_useAlignmentInfo), 1, m_outFile);
  ThrowingFwrite(&m_maxRank, sizeof(m_maxRank), 1, m_outFile);
  ThrowingFwrite(&m_maxPhraseLength, sizeof(m_maxPhraseLength), 1, m_outFile);

  if(m_coding == REnc) {
    // Save source language symbols for REnc
    std::vector<std::string> temp1;
    temp1.resize(m_sourceSymbolsMap.size());
    for(boost::unordered_map<std::string, unsigned>::iterator it
        = m_sourceSymbolsMap.begin(); it != m_sourceSymbolsMap.end(); it++)
      temp1[it->second] = it->first;
    std::sort(temp1.begin(), temp1.end());
    StringVector<unsigned char, unsigned, std::allocator> sourceSymbols;
    for(std::vector<std::string>::iterator it = temp1.begin();
        it != temp1.end(); it++)
      sourceSymbols.push_back(*it);
    sourceSymbols.save(m_outFile);

    // Save lexical translation table for REnc
    size_t size = m_lexicalTableIndex.size();
    ThrowingFwrite(&size, sizeof(size_t), 1, m_outFile);
    ThrowingFwrite(&m_lexicalTableIndex[0], sizeof(size_t), size, m_outFile);
    size = m_lexicalTable.size();
    ThrowingFwrite(&size, sizeof(size_t), 1, m_outFile);
    ThrowingFwrite(&m_lexicalTable[0], sizeof(SrcTrg), size, m_outFile);
  }

  // Save target language symbols
  std::vector<std::string> temp2;
  temp2.resize(m_targetSymbolsMap.size());
  for(boost::unordered_map<std::string, unsigned>::iterator it
      = m_targetSymbolsMap.begin(); it != m_targetSymbolsMap.end(); it++)
    temp2[it->second] = it->first;
  StringVector<unsigned char, unsigned, std::allocator> targetSymbols;
  for(std::vector<std::string>::iterator it = temp2.begin();
      it != temp2.end(); it++)
    targetSymbols.push_back(*it);
  targetSymbols.save(m_outFile);

  // Save Huffman codes for target language symbols
  m_symbolTree->Save(m_outFile);

  // Save number of Huffman code sets for scores and
  // save Huffman code sets
  ThrowingFwrite(&m_multipleScoreTrees, sizeof(m_multipleScoreTrees), 1, m_outFile);
  size_t numScoreTrees = m_scoreTrees.size();
  for(size_t i = 0; i < numScoreTrees; i++)
    m_scoreTrees[i]->Save(m_outFile);

  // Save Huffman codes for alignments
  if(m_useAlignmentInfo)
    m_alignTree->Save(m_outFile);

  // Save compressed target phrase collections
  m_compressedTargetPhrases->save(m_outFile);
}

void PhraseTableCreator::LoadLexicalTable(std::string filePath)
{
  std::vector<SrcTrgProb> t_lexTable;

  std::cerr << "Reading in lexical table for Rank Encoding" << std::endl;
  std::ifstream lexIn(filePath.c_str(), std::ifstream::in);
  std::string src, trg;
  float prob;

  // Reading in the translation probability lexicon

  std::cerr << "\tLoading from " << filePath << std::endl;
  while(lexIn >> trg >> src >> prob) {
    t_lexTable.push_back(SrcTrgProb(SrcTrgString(src, trg), prob));
    AddSourceSymbolId(src);
    AddTargetSymbolId(trg);
  }

  // Sorting lexicon by source words by lexicographical order, corresponding
  // target words by decreasing probability.

  std::cerr << "\tSorting according to translation rank" << std::endl;
  std::sort(t_lexTable.begin(), t_lexTable.end(), SrcTrgProbSorter());

  // Re-assigning source word ids in lexicographical order

  std::vector<std::string> temp1;
  temp1.resize(m_sourceSymbolsMap.size());
  for(boost::unordered_map<std::string, unsigned>::iterator it
      = m_sourceSymbolsMap.begin(); it != m_sourceSymbolsMap.end(); it++)
    temp1[it->second] = it->first;

  std::sort(temp1.begin(), temp1.end());

  for(size_t i = 0; i < temp1.size(); i++)
    m_sourceSymbolsMap[temp1[i]] = i;

  // Building the lexicon based on source and target word ids

  std::string srcWord = "";
  size_t srcIdx = 0;
  for(std::vector<SrcTrgProb>::iterator it = t_lexTable.begin();
      it != t_lexTable.end(); it++) {
    // If we encounter a new source word
    if(it->first.first != srcWord) {
      srcIdx = GetSourceSymbolId(it->first.first);

      // Store position of first translation
      if(srcIdx >= m_lexicalTableIndex.size())
        m_lexicalTableIndex.resize(srcIdx + 1);
      m_lexicalTableIndex[srcIdx] = m_lexicalTable.size();
    }

    // Store pair of source word and target word
    size_t trgIdx = GetTargetSymbolId(it->first.second);
    m_lexicalTable.push_back(SrcTrg(srcIdx, trgIdx));

    srcWord = it->first.first;
  }
  std::cerr << "\tLoaded " << m_lexicalTable.size() << " lexical pairs" << std::endl;
  std::cerr << std::endl;
}

void PhraseTableCreator::CreateRankHash()
{
  InputFileStream inFile(m_inPath);

#ifdef WITH_THREADS
  boost::thread_group threads;
  for (size_t i = 0; i < m_threads; ++i) {
    RankingTask* rt = new RankingTask(inFile, *this);
    threads.create_thread(*rt);
  }
  threads.join_all();
#else
  RankingTask* rt = new RankingTask(inFile, *this);
  (*rt)();
  delete rt;
#endif
  FlushRankedQueue(true);
}

inline std::string PhraseTableCreator::MakeSourceKey(std::string &source)
{
  return source + " " + m_separator + " ";
}

inline std::string PhraseTableCreator::MakeSourceTargetKey(std::string &source, std::string &target)
{
  return source + " " + m_separator + " " + target + " " + m_separator + " ";
}

void PhraseTableCreator::EncodeTargetPhrases()
{
  InputFileStream inFile(m_inPath);

#ifdef WITH_THREADS
  boost::thread_group threads;
  for (size_t i = 0; i < m_threads; ++i) {
    EncodingTask* et = new EncodingTask(inFile, *this);
    threads.create_thread(*et);
  }
  threads.join_all();
#else
  EncodingTask* et = new EncodingTask(inFile, *this);
  (*et)();
  delete et;
#endif
  FlushEncodedQueue(true);
}


void PhraseTableCreator::CompressTargetPhrases()
{
#ifdef WITH_THREADS
  boost::thread_group threads;
  for (size_t i = 0; i < m_threads; ++i) {
    CompressionTask* ct = new CompressionTask(*m_encodedTargetPhrases, *this);
    threads.create_thread(*ct);
  }
  threads.join_all();
#else
  CompressionTask* ct = new CompressionTask(*m_encodedTargetPhrases, *this);
  (*ct)();
  delete ct;
#endif
  FlushCompressedQueue(true);
}

void PhraseTableCreator::CalcHuffmanCodes()
{
  std::cerr << "\tCreating Huffman codes for " << m_symbolCounter.Size()
            << " target phrase symbols" << std::endl;

  m_symbolTree = new SymbolTree(m_symbolCounter.Begin(),
                                m_symbolCounter.End());

  std::vector<ScoreTree*>::iterator treeIt = m_scoreTrees.begin();
  for(std::vector<ScoreCounter*>::iterator it = m_scoreCounters.begin();
      it != m_scoreCounters.end(); it++) {
    if(m_quantize)
      (*it)->Quantize(m_quantize);

    std::cerr << "\tCreating Huffman codes for " << (*it)->Size()
              << " scores" << std::endl;

    *treeIt = new ScoreTree((*it)->Begin(), (*it)->End());
    treeIt++;
  }

  if(m_useAlignmentInfo) {
    std::cerr << "\tCreating Huffman codes for " << m_alignCounter.Size()
              << " alignment points" << std::endl;
    m_alignTree = new AlignTree(m_alignCounter.Begin(), m_alignCounter.End());
  }
  std::cerr << std::endl;
}


void PhraseTableCreator::AddSourceSymbolId(std::string& symbol)
{
  if(m_sourceSymbolsMap.count(symbol) == 0) {
    unsigned value = m_sourceSymbolsMap.size();
    m_sourceSymbolsMap[symbol] = value;
  }
}

void PhraseTableCreator::AddTargetSymbolId(std::string& symbol)
{
  if(m_targetSymbolsMap.count(symbol) == 0) {
    unsigned value = m_targetSymbolsMap.size();
    m_targetSymbolsMap[symbol] = value;
  }
}

unsigned PhraseTableCreator::GetSourceSymbolId(std::string& symbol)
{
  boost::unordered_map<std::string, unsigned>::iterator it
  = m_sourceSymbolsMap.find(symbol);

  if(it != m_sourceSymbolsMap.end())
    return it->second;
  else
    return m_sourceSymbolsMap.size();
}

unsigned PhraseTableCreator::GetTargetSymbolId(std::string& symbol)
{
  boost::unordered_map<std::string, unsigned>::iterator it
  = m_targetSymbolsMap.find(symbol);

  if(it != m_targetSymbolsMap.end())
    return it->second;
  else
    return m_targetSymbolsMap.size();
}

unsigned PhraseTableCreator::GetOrAddTargetSymbolId(std::string& symbol)
{
#ifdef WITH_THREADS
  boost::mutex::scoped_lock lock(m_mutex);
#endif
  boost::unordered_map<std::string, unsigned>::iterator it
  = m_targetSymbolsMap.find(symbol);

  if(it != m_targetSymbolsMap.end())
    return it->second;
  else {
    unsigned value = m_targetSymbolsMap.size();
    m_targetSymbolsMap[symbol] = value;
    return value;
  }
}

unsigned PhraseTableCreator::GetRank(unsigned srcIdx, unsigned trgIdx)
{
  size_t srcTrgIdx = m_lexicalTableIndex[srcIdx];
  while(srcTrgIdx < m_lexicalTable.size()
        && srcIdx == m_lexicalTable[srcTrgIdx].first
        && m_lexicalTable[srcTrgIdx].second != trgIdx)
    srcTrgIdx++;

  if(srcTrgIdx < m_lexicalTable.size()
      && m_lexicalTable[srcTrgIdx].second == trgIdx)
    return srcTrgIdx - m_lexicalTableIndex[srcIdx];
  else
    return m_lexicalTable.size();
}

unsigned PhraseTableCreator::EncodeREncSymbol1(unsigned trgIdx)
{
  assert((~(1 << 31)) > trgIdx);
  return trgIdx;
}

unsigned PhraseTableCreator::EncodeREncSymbol2(unsigned pos, unsigned rank)
{
  unsigned symbol = rank;
  symbol |= 1 << 30;
  symbol |= pos << 24;
  return symbol;
}

unsigned PhraseTableCreator::EncodeREncSymbol3(unsigned rank)
{
  unsigned symbol = rank;
  symbol |= 2 << 30;
  return symbol;
}

unsigned PhraseTableCreator::EncodePREncSymbol1(unsigned trgIdx)
{
  assert((~(1 << 31)) > trgIdx);
  return trgIdx;
}

unsigned PhraseTableCreator::EncodePREncSymbol2(int left, int right, unsigned rank)
{
  // "left" and "right" must be smaller than 2^5
  // "rank" must be smaller than 2^19
  left  = left  + 32;
  right = right + 32;

  assert(64 > left);
  assert(64 > right);
  assert(524288 > rank);

  unsigned symbol = 0;
  symbol |=    1  << 31;
  symbol |= left  << 25;
  symbol |= right << 19;
  symbol |= rank;
  return symbol;
}

void PhraseTableCreator::EncodeTargetPhraseNone(std::vector<std::string>& t,
    std::ostream& os)
{
  std::stringstream encodedTargetPhrase;
  size_t j = 0;
  while(j < t.size()) {
    unsigned targetSymbolId = GetOrAddTargetSymbolId(t[j]);

    m_symbolCounter.Increase(targetSymbolId);
    os.write((char*)&targetSymbolId, sizeof(targetSymbolId));
    j++;
  }

  unsigned stopSymbolId = GetTargetSymbolId(m_phraseStopSymbol);
  os.write((char*)&stopSymbolId, sizeof(stopSymbolId));
  m_symbolCounter.Increase(stopSymbolId);
}

void PhraseTableCreator::EncodeTargetPhraseREnc(std::vector<std::string>& s,
    std::vector<std::string>& t,
    std::set<AlignPoint>& a,
    std::ostream& os)
{
  std::stringstream encodedTargetPhrase;

  std::vector<std::vector<size_t> > a2(t.size());
  for(std::set<AlignPoint>::iterator it = a.begin(); it != a.end(); it++)
    a2[it->second].push_back(it->first);

  for(size_t i = 0; i < t.size(); i++) {
    unsigned idxTarget = GetOrAddTargetSymbolId(t[i]);
    unsigned encodedSymbol = -1;

    unsigned bestSrcPos = s.size();
    unsigned bestDiff = s.size();
    unsigned bestRank = m_lexicalTable.size();
    unsigned badRank = m_lexicalTable.size();

    for(std::vector<size_t>::iterator it = a2[i].begin(); it != a2[i].end(); it++) {
      unsigned idxSource = GetSourceSymbolId(s[*it]);
      size_t r = GetRank(idxSource, idxTarget);
      if(r != badRank) {
        if(r < bestRank) {
          bestRank = r;
          bestSrcPos = *it;
          bestDiff = abs(*it-i);
        } else if(r == bestRank && unsigned(abs(*it-i)) < bestDiff) {
          bestSrcPos = *it;
          bestDiff = abs(*it-i);
        }
      }
    }

    if(bestRank != badRank && bestSrcPos < s.size()) {
      if(bestSrcPos == i)
        encodedSymbol = EncodeREncSymbol3(bestRank);
      else
        encodedSymbol = EncodeREncSymbol2(bestSrcPos, bestRank);
      a.erase(AlignPoint(bestSrcPos, i));
    } else {
      encodedSymbol = EncodeREncSymbol1(idxTarget);
    }

    os.write((char*)&encodedSymbol, sizeof(encodedSymbol));
    m_symbolCounter.Increase(encodedSymbol);
  }

  unsigned stopSymbolId = GetTargetSymbolId(m_phraseStopSymbol);
  unsigned encodedSymbol = EncodeREncSymbol1(stopSymbolId);
  os.write((char*)&encodedSymbol, sizeof(encodedSymbol));
  m_symbolCounter.Increase(encodedSymbol);
}

void PhraseTableCreator::EncodeTargetPhrasePREnc(std::vector<std::string>& s,
    std::vector<std::string>& t,
    std::set<AlignPoint>& a,
    size_t ownRank,
    std::ostream& os)
{
  std::vector<unsigned> encodedSymbols(t.size());
  std::vector<unsigned> encodedSymbolsLengths(t.size(), 0);

  ConsistentPhrases cp(s.size(), t.size(), a);
  while(!cp.Empty()) {
    ConsistentPhrases::Phrase p = cp.Pop();

    std::stringstream key1;
    key1 << s[p.i];
    for(int i = p.i+1; i < p.i+p.m; i++)
      key1 << " " << s[i];

    std::stringstream key2;
    key2 << t[p.j];
    for(int i = p.j+1; i < p.j+p.n; i++)
      key2 << " " << t[i];

    int rank = -1;
    std::string key1Str = key1.str(), key2Str = key2.str();
    size_t idx = m_rnkHash[MakeSourceTargetKey(key1Str, key2Str)];
    if(idx != m_rnkHash.GetSize())
      rank = m_ranks[idx];

    if(rank >= 0 && (m_maxRank == 0 || unsigned(rank) < m_maxRank)) {
      if(unsigned(p.m) != s.size() || unsigned(rank) < ownRank) {
        std::stringstream encodedSymbol;
        encodedSymbols[p.j] = EncodePREncSymbol2(p.i-p.j, s.size()-(p.i+p.m), rank);
        encodedSymbolsLengths[p.j] = p.n;

        std::set<AlignPoint> tAlignment;
        for(std::set<AlignPoint>::iterator it = a.begin();
            it != a.end(); it++)
          if(it->first < p.i || it->first >= p.i + p.m
              || it->second < p.j || it->second >= p.j + p.n)
            tAlignment.insert(*it);
        a = tAlignment;
        cp.RemoveOverlap(p);
      }
    }
  }

  std::stringstream encodedTargetPhrase;

  size_t j = 0;
  while(j < t.size()) {
    if(encodedSymbolsLengths[j] > 0) {
      unsigned encodedSymbol = encodedSymbols[j];
      m_symbolCounter.Increase(encodedSymbol);
      os.write((char*)&encodedSymbol, sizeof(encodedSymbol));
      j += encodedSymbolsLengths[j];
    } else {
      unsigned targetSymbolId = GetOrAddTargetSymbolId(t[j]);
      unsigned encodedSymbol = EncodePREncSymbol1(targetSymbolId);
      m_symbolCounter.Increase(encodedSymbol);
      os.write((char*)&encodedSymbol, sizeof(encodedSymbol));
      j++;
    }
  }

  unsigned stopSymbolId = GetTargetSymbolId(m_phraseStopSymbol);
  unsigned encodedSymbol = EncodePREncSymbol1(stopSymbolId);
  os.write((char*)&encodedSymbol, sizeof(encodedSymbol));
  m_symbolCounter.Increase(encodedSymbol);
}

void PhraseTableCreator::EncodeScores(std::vector<float>& scores, std::ostream& os)
{
  size_t c = 0;
  float score;

  while(c < scores.size()) {
    score = scores[c];
    score = FloorScore(TransformScore(score));
    os.write((char*)&score, sizeof(score));
    m_scoreCounters[m_multipleScoreTrees ? c : 0]->Increase(score);
    c++;
  }
}

void PhraseTableCreator::EncodeAlignment(std::set<AlignPoint>& alignment,
    std::ostream& os)
{
  for(std::set<AlignPoint>::iterator it = alignment.begin();
      it != alignment.end(); it++) {
    os.write((char*)&(*it), sizeof(AlignPoint));
    m_alignCounter.Increase(*it);
  }
  AlignPoint stop(-1, -1);
  os.write((char*) &stop, sizeof(AlignPoint));
  m_alignCounter.Increase(stop);
}

std::string PhraseTableCreator::EncodeLine(std::vector<std::string>& tokens, size_t ownRank)
{
  std::string sourcePhraseStr = tokens[0];
  std::string targetPhraseStr = tokens[1];
  std::string scoresStr = tokens[2];

  std::string alignmentStr = "";
  if(tokens.size() > 3)
    alignmentStr = tokens[3];

  std::vector<std::string> s = Tokenize(sourcePhraseStr);

  size_t phraseLength = s.size();
  if(m_maxPhraseLength < phraseLength)
    m_maxPhraseLength = phraseLength;

  std::vector<std::string> t = Tokenize(targetPhraseStr);
  std::vector<float> scores = Tokenize<float>(scoresStr);

  if(scores.size() != m_numScoreComponent) {
    std::stringstream strme;
    strme << "Error: Wrong number of scores detected ("
          << scores.size() << " != " << m_numScoreComponent << ") :" << std::endl;
    strme << "Line: " << tokens[0] << " ||| " << tokens[1] << " ||| " << tokens[2] << " ..." << std::endl;
    UTIL_THROW2(strme.str());
  }

  std::set<AlignPoint> a;
  if(m_coding != None || m_useAlignmentInfo) {
    std::vector<size_t> positions = Tokenize<size_t>(alignmentStr, " \t-");
    for(size_t i = 0; i < positions.size(); i += 2) {
      a.insert(AlignPoint(positions[i], positions[i+1]));
    }
  }

  std::stringstream encodedTargetPhrase;

  if(m_coding == PREnc) {
    EncodeTargetPhrasePREnc(s, t, a, ownRank, encodedTargetPhrase);
  } else if(m_coding == REnc) {
    EncodeTargetPhraseREnc(s, t, a, encodedTargetPhrase);
  } else {
    EncodeTargetPhraseNone(t, encodedTargetPhrase);
  }

  EncodeScores(scores, encodedTargetPhrase);

  if(m_useAlignmentInfo)
    EncodeAlignment(a, encodedTargetPhrase);

  return encodedTargetPhrase.str();
}

std::string PhraseTableCreator::CompressEncodedCollection(std::string encodedCollection)
{
  enum EncodeState {
    ReadSymbol, ReadScore, ReadAlignment,
    EncodeSymbol, EncodeScore, EncodeAlignment
  };
  EncodeState state = ReadSymbol;

  unsigned phraseStopSymbolId;
  if(m_coding == REnc)
    phraseStopSymbolId = EncodeREncSymbol1(GetTargetSymbolId(m_phraseStopSymbol));
  else if(m_coding == PREnc)
    phraseStopSymbolId = EncodePREncSymbol1(GetTargetSymbolId(m_phraseStopSymbol));
  else
    phraseStopSymbolId = GetTargetSymbolId(m_phraseStopSymbol);
  AlignPoint alignStopSymbol(-1, -1);

  std::stringstream encodedStream(encodedCollection);
  encodedStream.unsetf(std::ios::skipws);

  std::string compressedEncodedCollection;
  BitWrapper<> bitStream(compressedEncodedCollection);

  unsigned symbol;
  float score;
  size_t currScore = 0;
  AlignPoint alignPoint;

  while(encodedStream) {
    switch(state) {
    case ReadSymbol:
      encodedStream.read((char*) &symbol, sizeof(unsigned));
      state = EncodeSymbol;
      break;
    case ReadScore:
      if(currScore == m_numScoreComponent) {
        currScore = 0;
        if(m_useAlignmentInfo)
          state = ReadAlignment;
        else
          state = ReadSymbol;
      } else {
        encodedStream.read((char*) &score, sizeof(float));
        currScore++;
        state = EncodeScore;
      }
      break;
    case ReadAlignment:
      encodedStream.read((char*) &alignPoint, sizeof(AlignPoint));
      state = EncodeAlignment;
      break;

    case EncodeSymbol:
      state = (symbol == phraseStopSymbolId) ? ReadScore : ReadSymbol;
      m_symbolTree->Put(bitStream, symbol);
      break;
    case EncodeScore: {
      state = ReadScore;
      size_t idx = m_multipleScoreTrees ? currScore-1 : 0;
      if(m_quantize)
        score = m_scoreCounters[idx]->LowerBound(score);
      m_scoreTrees[idx]->Put(bitStream, score);
    }
    break;
    case EncodeAlignment:
      state = (alignPoint == alignStopSymbol) ? ReadSymbol : ReadAlignment;
      m_alignTree->Put(bitStream, alignPoint);
      break;
    }
  }

  return compressedEncodedCollection;
}

void PhraseTableCreator::AddRankedLine(PackedItem& pi)
{
  m_queue.push(pi);
}

void PhraseTableCreator::FlushRankedQueue(bool force)
{
  size_t step = 1ul << 10;

  while(!m_queue.empty() && m_lastFlushedLine + 1 == m_queue.top().GetLine()) {
    m_lastFlushedLine++;

    PackedItem pi = m_queue.top();
    m_queue.pop();

    if(m_lastSourceRange.size() == step) {
      m_rnkHash.AddRange(m_lastSourceRange);
      m_lastSourceRange.clear();
    }

    if(m_lastFlushedSourcePhrase != pi.GetSrc()) {
      if(m_rankQueue.size()) {
        m_lastFlushedSourceNum++;
        if(m_lastFlushedSourceNum % 100000 == 0) {
          std::cerr << ".";
        }
        if(m_lastFlushedSourceNum % 5000000 == 0) {
          std::cerr << "[" << m_lastFlushedSourceNum << "]" << std::endl;
        }

        m_ranks.resize(m_lastFlushedLine + 1);
        int r = 0;
        while(!m_rankQueue.empty()) {
          m_ranks[m_rankQueue.top().second] = r++;
          m_rankQueue.pop();
        }
      }
    }

    m_lastSourceRange.push_back(pi.GetTrg());

    m_rankQueue.push(std::make_pair(pi.GetScore(), pi.GetLine()));
    m_lastFlushedSourcePhrase = pi.GetSrc();
  }

  if(force) {
    m_rnkHash.AddRange(m_lastSourceRange);
    m_lastSourceRange.clear();

#ifdef WITH_THREADS
    m_rnkHash.WaitAll();
#endif

    m_ranks.resize(m_lastFlushedLine + 1);
    int r = 0;
    while(!m_rankQueue.empty()) {
      m_ranks[m_rankQueue.top().second] = r++;
      m_rankQueue.pop();
    }

    m_lastFlushedLine = -1;
    m_lastFlushedSourceNum = 0;

    std::cerr << std::endl << std::endl;
  }
}


void PhraseTableCreator::AddEncodedLine(PackedItem& pi)
{
  m_queue.push(pi);
}

void PhraseTableCreator::FlushEncodedQueue(bool force)
{
  while(!m_queue.empty() && m_lastFlushedLine + 1 == m_queue.top().GetLine()) {
    PackedItem pi = m_queue.top();
    m_queue.pop();
    m_lastFlushedLine++;

    if(m_lastFlushedSourcePhrase != pi.GetSrc()) {
      if(m_lastCollection.size()) {
        std::stringstream targetPhraseCollection;
        for(std::vector<std::string>::iterator it =
              m_lastCollection.begin(); it != m_lastCollection.end(); it++)
          targetPhraseCollection << *it;

        m_lastSourceRange.push_back(MakeSourceKey(m_lastFlushedSourcePhrase));
        m_encodedTargetPhrases->push_back(targetPhraseCollection.str());

        m_lastFlushedSourceNum++;
        if(m_lastFlushedSourceNum % 100000 == 0)
          std::cerr << ".";
        if(m_lastFlushedSourceNum % 5000000 == 0)
          std::cerr << "[" << m_lastFlushedSourceNum << "]" << std::endl;

        m_lastCollection.clear();
      }
    }

    if(m_lastSourceRange.size() == (1ul << m_orderBits)) {
      m_srcHash.AddRange(m_lastSourceRange);
      m_srcHash.SaveLastRange();
      m_srcHash.DropLastRange();
      m_lastSourceRange.clear();
    }

    m_lastFlushedSourcePhrase = pi.GetSrc();
    if(m_coding == PREnc) {
      if(m_lastCollection.size() <= pi.GetRank())
        m_lastCollection.resize(pi.GetRank() + 1);
      m_lastCollection[pi.GetRank()] = pi.GetTrg();
    } else {
      m_lastCollection.push_back(pi.GetTrg());
    }
  }

  if(force) {
    if(!m_lastSourceRange.size() || m_lastSourceRange.back() != m_lastFlushedSourcePhrase)
      m_lastSourceRange.push_back(MakeSourceKey(m_lastFlushedSourcePhrase));

    if(m_lastCollection.size()) {
      std::stringstream targetPhraseCollection;
      for(std::vector<std::string>::iterator it =
            m_lastCollection.begin(); it != m_lastCollection.end(); it++)
        targetPhraseCollection << *it;

      m_encodedTargetPhrases->push_back(targetPhraseCollection.str());
      m_lastCollection.clear();
    }

    m_srcHash.AddRange(m_lastSourceRange);
    m_lastSourceRange.clear();

#ifdef WITH_THREADS
    m_srcHash.WaitAll();
#endif

    m_srcHash.SaveLastRange();
    m_srcHash.DropLastRange();
    m_srcHash.FinalizeSave();

    m_lastFlushedLine = -1;
    m_lastFlushedSourceNum = 0;

    std::cerr << std::endl << std::endl;
  }
}

void PhraseTableCreator::AddCompressedCollection(PackedItem& pi)
{
  m_queue.push(pi);
}

void PhraseTableCreator::FlushCompressedQueue(bool force)
{
  if(force || m_queue.size() > 10000) {
    while(!m_queue.empty() && m_lastFlushedLine + 1 == m_queue.top().GetLine()) {
      PackedItem pi = m_queue.top();
      m_queue.pop();
      m_lastFlushedLine++;

      m_compressedTargetPhrases->push_back(pi.GetTrg());

      if((pi.GetLine()+1) % 100000 == 0)
        std::cerr << ".";
      if((pi.GetLine()+1) % 5000000 == 0)
        std::cerr << "[" << (pi.GetLine()+1) << "]" << std::endl;
    }
  }

  if(force) {
    m_lastFlushedLine = -1;
    std::cerr << std::endl << std::endl;
  }
}

//****************************************************************************//

size_t RankingTask::m_lineNum = 0;
#ifdef WITH_THREADS
boost::mutex RankingTask::m_mutex;
boost::mutex RankingTask::m_fileMutex;
#endif

RankingTask::RankingTask(InputFileStream& inFile, PhraseTableCreator& creator)
  : m_inFile(inFile), m_creator(creator) {}

void RankingTask::operator()()
{
  size_t lineNum = 0;

  std::vector<std::string> lines;
  size_t max_lines = 1000;
  lines.reserve(max_lines);

  {
#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_fileMutex);
#endif
    std::string line;
    while(lines.size() < max_lines && std::getline(m_inFile, line))
      lines.push_back(line);
    lineNum = m_lineNum;
    m_lineNum += lines.size();
  }

  std::vector<PackedItem> result;
  result.reserve(max_lines);

  while(lines.size()) {
    for(size_t i = 0; i < lines.size(); i++) {
      std::vector<std::string> tokens;
      Moses::TokenizeMultiCharSeparator(tokens, lines[i], m_creator.m_separator);

      for(std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
        *it = Moses::Trim(*it);

      if(tokens.size() < 4) {
        std::stringstream strme;
        strme << "Error: It seems the following line has a wrong format:" << std::endl;
        strme << "Line " << i << ": " << lines[i] << std::endl;
        UTIL_THROW2(strme.str());
      }

      if(tokens[3].size() <= 1 && m_creator.m_coding != PhraseTableCreator::None) {
        std::stringstream strme;
        strme << "Error: It seems the following line contains no alignment information, " << std::endl;
        strme << "but you are using ";
        strme << (m_creator.m_coding == PhraseTableCreator::PREnc ? "PREnc" : "REnc");
        strme << " encoding which makes use of alignment data. " << std::endl;
        strme << "Use -encoding None" << std::endl;
        strme << "Line " << i << ": " << lines[i] << std::endl;
        UTIL_THROW2(strme.str());
      }

      std::vector<float> scores = Tokenize<float>(tokens[2]);
      if(scores.size() != m_creator.m_numScoreComponent) {
        std::stringstream strme;
        strme << "Error: It seems the following line has a wrong number of scores ("
              << scores.size() << " != " << m_creator.m_numScoreComponent << ") :" << std::endl;
        strme << "Line " << i << ": " << lines[i] << std::endl;
        UTIL_THROW2(strme.str());
      }

      float sortScore = scores[m_creator.m_sortScoreIndex];

      std::string key1 = m_creator.MakeSourceKey(tokens[0]);
      std::string key2 = m_creator.MakeSourceTargetKey(tokens[0], tokens[1]);

      PackedItem packedItem(lineNum + i, key1, key2, 0, sortScore);
      result.push_back(packedItem);
    }
    lines.clear();

    {
#ifdef WITH_THREADS
      boost::mutex::scoped_lock lock(m_mutex);
#endif
      for(size_t i = 0; i < result.size(); i++)
        m_creator.AddRankedLine(result[i]);
      m_creator.FlushRankedQueue();
    }

    result.clear();
    lines.reserve(max_lines);
    result.reserve(max_lines);

#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_fileMutex);
#endif
    std::string line;
    while(lines.size() < max_lines && std::getline(m_inFile, line))
      lines.push_back(line);
    lineNum = m_lineNum;
    m_lineNum += lines.size();
  }
}

size_t EncodingTask::m_lineNum = 0;
#ifdef WITH_THREADS
boost::mutex EncodingTask::m_mutex;
boost::mutex EncodingTask::m_fileMutex;
#endif

EncodingTask::EncodingTask(InputFileStream& inFile, PhraseTableCreator& creator)
  : m_inFile(inFile), m_creator(creator) {}

void EncodingTask::operator()()
{
  size_t lineNum = 0;

  std::vector<std::string> lines;
  size_t max_lines = 1000;
  lines.reserve(max_lines);

  {
#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_fileMutex);
#endif
    std::string line;
    while(lines.size() < max_lines && std::getline(m_inFile, line))
      lines.push_back(line);
    lineNum = m_lineNum;
    m_lineNum += lines.size();
  }

  std::vector<PackedItem> result;
  result.reserve(max_lines);

  while(lines.size()) {
    for(size_t i = 0; i < lines.size(); i++) {
      std::vector<std::string> tokens;
      Moses::TokenizeMultiCharSeparator(tokens, lines[i], m_creator.m_separator);

      for(std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
        *it = Moses::Trim(*it);

      if(tokens.size() < 3) {
        std::stringstream strme;
        strme << "Error: It seems the following line has a wrong format:" << std::endl;
        strme << "Line " << i << ": " << lines[i] << std::endl;
        UTIL_THROW2(strme.str());
      }

      if(tokens.size() > 3 && tokens[3].size() <= 1 && m_creator.m_coding != PhraseTableCreator::None) {
        std::stringstream strme;
        strme << "Error: It seems the following line contains no alignment information, " << std::endl;
        strme << "but you are using ";
        strme << (m_creator.m_coding == PhraseTableCreator::PREnc ? "PREnc" : "REnc");
        strme << " encoding which makes use of alignment data. " << std::endl;
        strme << "Use -encoding None" << std::endl;
        strme << "Line " << i << ": " << lines[i] << std::endl;
        UTIL_THROW2(strme.str());
      }

      size_t ownRank = 0;
      if(m_creator.m_coding == PhraseTableCreator::PREnc)
        ownRank = m_creator.m_ranks[lineNum + i];

      std::string encodedLine = m_creator.EncodeLine(tokens, ownRank);

      PackedItem packedItem(lineNum + i, tokens[0], encodedLine, ownRank);
      result.push_back(packedItem);
    }
    lines.clear();

    {
#ifdef WITH_THREADS
      boost::mutex::scoped_lock lock(m_mutex);
#endif
      for(size_t i = 0; i < result.size(); i++)
        m_creator.AddEncodedLine(result[i]);
      m_creator.FlushEncodedQueue();
    }

    result.clear();
    lines.reserve(max_lines);
    result.reserve(max_lines);

#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_fileMutex);
#endif
    std::string line;
    while(lines.size() < max_lines && std::getline(m_inFile, line))
      lines.push_back(line);
    lineNum = m_lineNum;
    m_lineNum += lines.size();
  }
}

//****************************************************************************//

size_t CompressionTask::m_collectionNum = 0;
#ifdef WITH_THREADS
boost::mutex CompressionTask::m_mutex;
#endif

CompressionTask::CompressionTask(StringVector<unsigned char, unsigned long,
                                 MmapAllocator>& encodedCollections,
                                 PhraseTableCreator& creator)
  : m_encodedCollections(encodedCollections), m_creator(creator) {}

void CompressionTask::operator()()
{
  size_t collectionNum;
  {
#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_mutex);
#endif
    collectionNum = m_collectionNum;
    m_collectionNum++;
  }

  while(collectionNum < m_encodedCollections.size()) {
    std::string collection = m_encodedCollections[collectionNum];
    std::string compressedCollection
    = m_creator.CompressEncodedCollection(collection);

    std::string dummy;
    PackedItem packedItem(collectionNum, dummy, compressedCollection, 0);

#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_mutex);
#endif
    m_creator.AddCompressedCollection(packedItem);
    m_creator.FlushCompressedQueue();

    collectionNum = m_collectionNum;
    m_collectionNum++;
  }
}

//****************************************************************************//

PackedItem::PackedItem(long line, std::string sourcePhrase,
                       std::string packedTargetPhrase, size_t rank,
                       float score)
  : m_line(line), m_sourcePhrase(sourcePhrase),
    m_packedTargetPhrase(packedTargetPhrase), m_rank(rank),
    m_score(score) {}

long PackedItem::GetLine() const
{
  return m_line;
}

const std::string& PackedItem::GetSrc() const
{
  return m_sourcePhrase;
}

const std::string& PackedItem::GetTrg() const
{
  return m_packedTargetPhrase;
}

size_t PackedItem::GetRank() const
{
  return m_rank;
}

float PackedItem::GetScore() const
{
  return m_score;
}

}
