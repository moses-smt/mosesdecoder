/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 University of Edinburgh
 
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

#include <fstream>
#include <stdexcept> 

#ifdef MPI_ENABLED
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
namespace mpi=boost::mpi;
#endif
 
#include "OnlineTrainingCorpus.h"
#include "Selector.h"
#include "Utils.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  OnlineTrainingCorpus::OnlineTrainingCorpus (
      const std::string& sourceFile,
      size_t batchLines,
      size_t epochLines,
      size_t maxLines,
      int mpiSize,
      int mpiRank):
  m_batchLines(batchLines),
  m_epochLines(epochLines),
  m_maxLines(maxLines),
  m_mpiSize(mpiSize),
  m_mpiRank(mpiRank),
  m_totalLines(0)
  {
    if (batchLines && epochLines % batchLines != 0) {
      throw runtime_error("Size of epoch should be divisible by batch size");
    }
    if (maxLines % epochLines != 0) {
      throw runtime_error("Maximum lines should be divisible by epoch size");
    }
    if (batchLines > 1) {
      if (epochLines % mpiSize != 0) {
        throw runtime_error("When using batching, shards should be of equal size");
      }
      size_t shardLines = epochLines / mpiSize;
      if (shardLines % batchLines != 0) {
          throw runtime_error("Shard size should be divisible by batch size");
      }
    }
    
    //Load the source file
    ifstream in(sourceFile.c_str());
    if (!in) {
      throw runtime_error("Unable to open: " + sourceFile);
    }
    string line;
    while (getline(in,line)) {
      m_lines.push_back(line);
    }
    
    //Line ids
    for (size_t i = 0; i < m_lines.size(); ++i) {
      m_lineIds.push_back(i); 
    }
    m_lineIdIndex = 0;
    RandomIndex rand;
    random_shuffle(m_lineIds.begin(),m_lineIds.end(),rand);
    InitEpoch();
    
  }
  
  void OnlineTrainingCorpus::InitEpoch() {
    //sentence ids in this epoch
    vector<size_t> epoch;
    if (m_mpiRank == 0) {
      while (epoch.size() < m_epochLines) {
        epoch.push_back(m_lineIds[m_lineIdIndex]);
        ++m_lineIdIndex;
        if (m_lineIdIndex >= m_lineIds.size()) m_lineIdIndex = 0;
      }
    }
    
    //split into shards
    m_shard.clear();
#ifdef MPI_ENABLED  
    mpi::communicator world;
    mpi::broadcast(world,epoch,0);
#endif
    if (m_batchLines) {
      float shard_size = m_epochLines / (float)m_mpiSize;
      VERBOSE(1, "Shard size: " << shard_size << endl);
      size_t shard_start = (size_t)(shard_size *m_mpiRank);
      size_t shard_end = (size_t)(shard_size * (m_mpiRank+1));
      if (m_mpiRank == m_mpiSize-1) shard_end = m_epochLines;
      VERBOSE(1, "Rank: " << m_mpiRank << " Shard start: " << shard_start << " Shard end: " << shard_end << endl);
      for (size_t i = shard_start; i < shard_end; ++i) {
        m_shard.push_back(epoch[i]);
      }
    } else {
      //each core gets whole epoch as a shard
      m_shard.insert(m_shard.begin(),epoch.begin(),epoch.end()); 
      VERBOSE(1,"Shard contains whole epoch" << endl);
    }
  }
  
  /** Next batch of sentences. Flags indicate whether we should mix or dump at the end of this batch*/
  void OnlineTrainingCorpus::GetNextBatch(
      std::vector<std::string>* lines, 
      std::vector<std::size_t>* lineNumbers,
      bool* shouldMix) 
  {
    lines->clear();
    lineNumbers->clear();
    while (lines->size() < m_batchLines || 
      (m_batchLines == 0 && lines->size() < m_epochLines)) {
      lineNumbers->push_back(m_shard.back());
      lines->push_back(m_lines[m_shard.back()]);
      m_shard.pop_back();
      VERBOSE(1,"Add to batch: " << lineNumbers->back() << " rank: " << m_mpiRank << endl);
    }
    if (m_shard.empty()) {
      *shouldMix = true;
      InitEpoch();
    } else {
      *shouldMix = false;
    }
    VERBOSE(1, "Mix?: " << *shouldMix << " rank: " << m_mpiRank << endl);
    if (m_batchLines) {
      m_totalLines += (m_batchLines*m_mpiSize);
    } else {
      m_totalLines += m_epochLines;
    }
    VERBOSE(1,"Total lines: " << m_totalLines <<  " rank: " << m_mpiRank << endl);
  }
    
  bool OnlineTrainingCorpus::HasMore() const {
    return m_totalLines < m_maxLines;
  }
  
  
}
