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
 
 
#pragma once

#include <string>
#include <vector>


namespace Josiah {

/**
  * Used to manage the training corpus - batching,sharding etc.
 **/
class OnlineTrainingCorpus {
  
  public:
    OnlineTrainingCorpus(
      const std::string& sourceFile,
      size_t batchLines, //Size of batches
      size_t epochLines, //Total lines in each epoch. These will be split into shards
      size_t maxLines, //Total lines to be processed
      int mpiSize,
      int mpiRank);
    
    //NB: maxLines must be divisible by epochLines
    // epochLines must be divisible by batchLines.
    
    /** Next batch of sentences. Flags indicate whether we should mix or dump at the end of this batch*/
    void GetNextBatch(std::vector<std::string>* lines, 
                      std::vector<std::size_t>* lineNumbers,
                      bool* shouldMix);
    
    bool HasMore() const;
    
  private:
    void InitEpoch();
    
    size_t m_batchLines;
    size_t m_epochLines;
    size_t m_maxLines;
    int m_mpiSize;
    int m_mpiRank;
    size_t m_totalLines; 
    
    std::vector<std::string> m_lines;
    std::vector<size_t> m_lineIds;
    std::vector<size_t> m_shard;
    size_t m_lineIdIndex;
};

}
