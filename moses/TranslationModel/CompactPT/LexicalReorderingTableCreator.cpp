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

#include <sstream>
#include "LexicalReorderingTableCreator.h"
#include "ThrowingFwrite.h"
#include "moses/Util.h"
#include "util/file.hh"
#include "util/exception.hh"

namespace Moses
{

LexicalReorderingTableCreator::LexicalReorderingTableCreator(
  std::string inPath, std::string outPath, std::string tempfilePath,
  size_t orderBits, size_t fingerPrintBits, bool multipleScoreTrees,
  size_t quantize
#ifdef WITH_THREADS
  , size_t threads
#endif
)
  : m_inPath(inPath), m_outPath(outPath), m_tempfilePath(tempfilePath),
    m_orderBits(orderBits), m_fingerPrintBits(fingerPrintBits),
    m_numScoreComponent(0), m_multipleScoreTrees(multipleScoreTrees),
    m_quantize(quantize), m_separator(" ||| "),
    m_hash(m_orderBits, m_fingerPrintBits), m_lastFlushedLine(-1)
#ifdef WITH_THREADS
    , m_threads(threads)
#endif
{
  PrintInfo();

  m_outFile = std::fopen(m_outPath.c_str(), "w");

  std::cerr << "Pass 1/2: Creating phrase index + Counting scores" << std::endl;
  m_hash.BeginSave(m_outFile);


  if(tempfilePath.size()) {
    MmapAllocator<unsigned char> allocEncoded(util::FMakeTemp(tempfilePath));
    m_encodedScores = new StringVector<unsigned char, unsigned long, MmapAllocator>(allocEncoded);
  } else {
    m_encodedScores = new StringVector<unsigned char, unsigned long, MmapAllocator>();
  }

  EncodeScores();

  std::cerr << "Intermezzo: Calculating Huffman code sets" << std::endl;
  CalcHuffmanCodes();

  std::cerr << "Pass 2/2: Compressing scores" << std::endl;


  if(tempfilePath.size()) {
    MmapAllocator<unsigned char> allocCompressed(util::FMakeTemp(tempfilePath));
    m_compressedScores = new StringVector<unsigned char, unsigned long, MmapAllocator>(allocCompressed);
  } else {
    m_compressedScores = new StringVector<unsigned char, unsigned long, MmapAllocator>();
  }
  CompressScores();

  std::cerr << "Saving to " << m_outPath << std::endl;
  Save();
  std::cerr << "Done" << std::endl;
  std::fclose(m_outFile);
}

void LexicalReorderingTableCreator::PrintInfo()
{
  std::cerr << "Used options:" << std::endl;
  std::cerr << "\tText reordering table will be read from: " << m_inPath << std::endl;
  std::cerr << "\tOutput reordering table will be written to: " << m_outPath << std::endl;
  std::cerr << "\tStep size for source landmark phrases: 2^" << m_orderBits << "=" << (1ul << m_orderBits) << std::endl;
  std::cerr << "\tPhrase fingerprint size: " << m_fingerPrintBits << " bits / P(fp)=" << (float(1)/(1ul << m_fingerPrintBits)) << std::endl;
  std::cerr << "\tSingle Huffman code set for score components: " << (m_multipleScoreTrees ? "no" : "yes") << std::endl;
  std::cerr << "\tUsing score quantization: ";
  if(m_quantize)
    std::cerr << m_quantize << " best" << std::endl;
  else
    std::cerr << "no" << std::endl;

#ifdef WITH_THREADS
  std::cerr << "\tRunning with " << m_threads << " threads" << std::endl;
#endif
  std::cerr << std::endl;
}

LexicalReorderingTableCreator::~LexicalReorderingTableCreator()
{
  for(size_t i = 0; i < m_scoreTrees.size(); i++) {
    delete m_scoreTrees[i];
    delete m_scoreCounters[i];
  }

  delete m_encodedScores;
  delete m_compressedScores;
}


void LexicalReorderingTableCreator::EncodeScores()
{
  InputFileStream inFile(m_inPath);

#ifdef WITH_THREADS
  boost::thread_group threads;
  for (size_t i = 0; i < m_threads; ++i) {
    EncodingTaskReordering* et = new EncodingTaskReordering(inFile, *this);
    threads.create_thread(*et);
  }
  threads.join_all();
#else
  EncodingTaskReordering* et = new EncodingTaskReordering(inFile, *this);
  (*et)();
  delete et;
#endif
  FlushEncodedQueue(true);
}

void LexicalReorderingTableCreator::CalcHuffmanCodes()
{
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
  std::cerr << std::endl;
}

void LexicalReorderingTableCreator::CompressScores()
{
#ifdef WITH_THREADS
  boost::thread_group threads;
  for (size_t i = 0; i < m_threads; ++i) {
    CompressionTaskReordering* ct = new CompressionTaskReordering(*m_encodedScores, *this);
    threads.create_thread(*ct);
  }
  threads.join_all();
#else
  CompressionTaskReordering* ct = new CompressionTaskReordering(*m_encodedScores, *this);
  (*ct)();
  delete ct;
#endif
  FlushCompressedQueue(true);
}

void LexicalReorderingTableCreator::Save()
{
  ThrowingFwrite(&m_numScoreComponent, sizeof(m_numScoreComponent), 1, m_outFile);
  ThrowingFwrite(&m_multipleScoreTrees, sizeof(m_multipleScoreTrees), 1, m_outFile);
  for(size_t i = 0; i < m_scoreTrees.size(); i++)
    m_scoreTrees[i]->Save(m_outFile);

  m_compressedScores->save(m_outFile);
}

std::string LexicalReorderingTableCreator::MakeSourceTargetKey(std::string &source, std::string &target)
{
  std::string key = source + m_separator;
  if(!target.empty())
    key += target + m_separator;
  return key;
}

std::string LexicalReorderingTableCreator::EncodeLine(std::vector<std::string>& tokens)
{
  std::string scoresString = tokens.back();
  std::stringstream scoresStream;

  std::vector<float> scores;
  Tokenize<float>(scores, scoresString);

  if(!m_numScoreComponent) {
    m_numScoreComponent = scores.size();
    m_scoreCounters.resize(m_multipleScoreTrees ? m_numScoreComponent : 1);
    for(std::vector<ScoreCounter*>::iterator it = m_scoreCounters.begin();
        it != m_scoreCounters.end(); it++)
      *it = new ScoreCounter();
    m_scoreTrees.resize(m_multipleScoreTrees ? m_numScoreComponent : 1);
  }

  if(m_numScoreComponent != scores.size()) {
    std::stringstream strme;
    strme << "Error: Wrong number of scores detected ("
          << scores.size() << " != " << m_numScoreComponent << ") :" << std::endl;
    strme << "Line: " << tokens[0] << " ||| ... ||| " << scoresString << std::endl;
    UTIL_THROW2(strme.str());
  }

  size_t c = 0;
  float score;
  while(c < m_numScoreComponent) {
    score = scores[c];
    score = FloorScore(TransformScore(score));
    scoresStream.write((char*)&score, sizeof(score));

    m_scoreCounters[m_multipleScoreTrees ? c : 0]->Increase(score);
    c++;
  }

  return scoresStream.str();
}

void LexicalReorderingTableCreator::AddEncodedLine(PackedItem& pi)
{
  m_queue.push(pi);
}

void LexicalReorderingTableCreator::FlushEncodedQueue(bool force)
{
  if(force || m_queue.size() > 10000) {
    while(!m_queue.empty() && m_lastFlushedLine + 1 == m_queue.top().GetLine()) {
      PackedItem pi = m_queue.top();
      m_queue.pop();
      m_lastFlushedLine++;

      m_lastRange.push_back(pi.GetSrc());
      m_encodedScores->push_back(pi.GetTrg());

      if((pi.GetLine()+1) % 100000 == 0)
        std::cerr << ".";
      if((pi.GetLine()+1) % 5000000 == 0)
        std::cerr << "[" << (pi.GetLine()+1) << "]" << std::endl;

      if(m_lastRange.size() == (1ul << m_orderBits)) {
        m_hash.AddRange(m_lastRange);
        m_hash.SaveLastRange();
        m_hash.DropLastRange();
        m_lastRange.clear();
      }
    }
  }

  if(force) {
    m_lastFlushedLine = -1;

    if(!m_lastRange.empty()) {
      m_hash.AddRange(m_lastRange);
      m_lastRange.clear();
    }

#ifdef WITH_THREADS
    m_hash.WaitAll();
#endif

    m_hash.SaveLastRange();
    m_hash.DropLastRange();
    m_hash.FinalizeSave();

    std::cerr << std::endl << std::endl;
  }
}

std::string LexicalReorderingTableCreator::CompressEncodedScores(std::string &encodedScores)
{
  std::stringstream encodedScoresStream(encodedScores);
  encodedScoresStream.unsetf(std::ios::skipws);

  std::string compressedScores;
  BitWrapper<> compressedScoresStream(compressedScores);

  size_t currScore = 0;
  float score;
  encodedScoresStream.read((char*) &score, sizeof(score));

  while(encodedScoresStream) {
    size_t index = currScore % m_scoreTrees.size();

    if(m_quantize)
      score = m_scoreCounters[index]->LowerBound(score);

    m_scoreTrees[index]->Put(compressedScoresStream, score);
    encodedScoresStream.read((char*) &score, sizeof(score));
    currScore++;
  }

  return compressedScores;
}

void LexicalReorderingTableCreator::AddCompressedScores(PackedItem& pi)
{
  m_queue.push(pi);
}

void LexicalReorderingTableCreator::FlushCompressedQueue(bool force)
{
  if(force || m_queue.size() > 10000) {
    while(!m_queue.empty() && m_lastFlushedLine + 1 == m_queue.top().GetLine()) {
      PackedItem pi = m_queue.top();
      m_queue.pop();
      m_lastFlushedLine++;

      m_compressedScores->push_back(pi.GetTrg());

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

size_t EncodingTaskReordering::m_lineNum = 0;
#ifdef WITH_THREADS
boost::mutex EncodingTaskReordering::m_mutex;
boost::mutex EncodingTaskReordering::m_fileMutex;
#endif

EncodingTaskReordering::EncodingTaskReordering(InputFileStream& inFile, LexicalReorderingTableCreator& creator)
  : m_inFile(inFile), m_creator(creator) {}

void EncodingTaskReordering::operator()()
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

      std::string encodedLine = m_creator.EncodeLine(tokens);

      std::string f = tokens[0];

      std::string e;
      if(tokens.size() > 2)
        e = tokens[1];

      PackedItem packedItem(lineNum + i, m_creator.MakeSourceTargetKey(f, e),
                            encodedLine, i);
      result.push_back(packedItem);
    }

    {
#ifdef WITH_THREADS
      boost::mutex::scoped_lock lock(m_mutex);
#endif
      for(size_t i = 0; i < result.size(); i++)
        m_creator.AddEncodedLine(result[i]);
      m_creator.FlushEncodedQueue();
    }

    lines.clear();
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

size_t CompressionTaskReordering::m_scoresNum = 0;
#ifdef WITH_THREADS
boost::mutex CompressionTaskReordering::m_mutex;
#endif

CompressionTaskReordering::CompressionTaskReordering(StringVector<unsigned char, unsigned long,
    MmapAllocator>& encodedScores,
    LexicalReorderingTableCreator& creator)
  : m_encodedScores(encodedScores), m_creator(creator)
{ }

void CompressionTaskReordering::operator()()
{
  size_t scoresNum;
  {
#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_mutex);
#endif
    scoresNum = m_scoresNum;
    m_scoresNum++;
  }

  while(scoresNum < m_encodedScores.size()) {
    std::string scores = m_encodedScores[scoresNum];
    std::string compressedScores
    = m_creator.CompressEncodedScores(scores);

    std::string dummy;
    PackedItem packedItem(scoresNum, dummy, compressedScores, 0);

#ifdef WITH_THREADS
    boost::mutex::scoped_lock lock(m_mutex);
#endif
    m_creator.AddCompressedScores(packedItem);
    m_creator.FlushCompressedQueue();

    scoresNum = m_scoresNum;
    m_scoresNum++;
  }
}

}
