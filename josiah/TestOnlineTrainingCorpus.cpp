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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "OnlineTrainingCorpus.h"

using namespace Josiah;
using namespace std;

BOOST_AUTO_TEST_SUITE(online_training_corpus)
    
  class SourceFixture {
    public:
      SourceFixture() {
        sourceFile = string(tmpnam(NULL));
        size_t sourceSize = 20;
        string line = "one two three four five six seven eight nine ten";
        ofstream sourceHandle(sourceFile.c_str());
        for (size_t i = 0; i < sourceSize; ++i) {
          sourceHandle << line << " " << i << endl;
        }
      }
      
      ~SourceFixture() {
        BOOST_CHECK(!remove(sourceFile.c_str()));
      }
      
      string sourceFile;
  };
  
  BOOST_FIXTURE_TEST_CASE(ctor_validate_lines, SourceFixture) {
    //args are batch, epoch, max, mpi_size, mpi_rank
    OnlineTrainingCorpus(sourceFile,1,10,50,1,0);
    //epoch size not divisible by batch size
    BOOST_CHECK_THROW(OnlineTrainingCorpus(sourceFile,3,10,50,1,0), runtime_error);
    //max not divisible by shard
    BOOST_CHECK_THROW(OnlineTrainingCorpus(sourceFile,2,10,52,1,0), runtime_error);
    //for mpi, shard size should be divisible by batch size
    // This example should give a shard size of 5, which is not divisible 
    // by the batch size of 2
    BOOST_CHECK_THROW(OnlineTrainingCorpus(sourceFile, 2, 10,  50 , 2, 0), runtime_error);
  }
  
  BOOST_FIXTURE_TEST_CASE(batch_single_core, SourceFixture) {
    //args are batch, epoch,  max, mpi_size, mpi_rank
    OnlineTrainingCorpus corpus(sourceFile,4,20,120,1,0);
    size_t batchCount = 0;
    size_t lineCount = 0;
    multiset<size_t> linesSeen;
    while(corpus.HasMore()) {
      vector<string> lines;
      vector<size_t> lineNumbers;
      bool shouldMix;
      corpus.GetNextBatch(&lines, &lineNumbers, &shouldMix);
      BOOST_CHECK_EQUAL(lines.size(), (size_t)4);
      BOOST_CHECK_EQUAL(lineNumbers.size(), (size_t)4);
      
      ++batchCount;
      lineCount += lines.size();
      linesSeen.insert(lineNumbers.begin(), lineNumbers.end());
      BOOST_CHECK_EQUAL(shouldMix, lineCount % 20 == 0);
      //cerr << "lineCount " << lineCount << " " << shouldDump << endl;
    }
    
    BOOST_CHECK_EQUAL(lineCount, (size_t)120);
    BOOST_CHECK_EQUAL(batchCount, (size_t)30);
    //Each sentence should appear exactly 6 times
    for (multiset<size_t>::iterator i = linesSeen.begin(); i != linesSeen.end(); ++i) {
      BOOST_CHECK_EQUAL((size_t)linesSeen.count(*i), (size_t)6);
    }
    
  }
  
  BOOST_FIXTURE_TEST_CASE(batch_multi_core, SourceFixture) {
    //args are batch, epoch,  max, mpi_size, mpi_rank
    OnlineTrainingCorpus corpus(sourceFile,4,60,120,3,0);
    multiset<size_t> linesSeen;
    while(corpus.HasMore()) {
      vector<string> lines;
      vector<size_t> lineNumbers;
      bool shouldMix;
      corpus.GetNextBatch(&lines, &lineNumbers, &shouldMix);
      
      linesSeen.insert(lineNumbers.begin(), lineNumbers.end());
    }
    BOOST_CHECK_EQUAL(linesSeen.size(),(size_t)40);
    //Each sentence should appear exactly twice
    for (multiset<size_t>::iterator i = linesSeen.begin(); i != linesSeen.end(); ++i) {
      BOOST_CHECK_EQUAL((size_t)linesSeen.count(*i), (size_t)2);
    }
  }

  BOOST_FIXTURE_TEST_CASE(batch_zero, SourceFixture) {
    //set batch size to 0, no mpi
    //batch should be whole epoch
    OnlineTrainingCorpus corpus(sourceFile,0,20,60,1,0);
    //each line should be seen 3 times, and should mix after every batch
    multiset<size_t> linesSeen;
    while(corpus.HasMore()) {
      vector<string> lines;
      vector<size_t> lineNumbers;
      bool shouldMix;
      corpus.GetNextBatch(&lines,&lineNumbers,&shouldMix);
      BOOST_CHECK(shouldMix);
      linesSeen.insert(lineNumbers.begin(), lineNumbers.end());
    }
    BOOST_CHECK_EQUAL(linesSeen.size(), (size_t)60);
    for (multiset<size_t>::iterator i = linesSeen.begin(); i != linesSeen.end(); ++i) {
      BOOST_CHECK_EQUAL((size_t)linesSeen.count(*i), (size_t)3);
    }

    //try again, with mpi
    OnlineTrainingCorpus corpus2(sourceFile,0,20,80,3,0);
    //Each line should be seen 4 times, mix after every batch
    linesSeen.clear();
    while(corpus2.HasMore()) {
      vector<string> lines;
      vector<size_t> lineNumbers;
      bool shouldMix;
      corpus2.GetNextBatch(&lines,&lineNumbers,&shouldMix);
      BOOST_CHECK(shouldMix);
      linesSeen.insert(lineNumbers.begin(), lineNumbers.end());
    }
    BOOST_CHECK_EQUAL(linesSeen.size(), (size_t)80);
    for (multiset<size_t>::iterator i = linesSeen.begin(); i != linesSeen.end(); ++i) {
      BOOST_CHECK_EQUAL((size_t)linesSeen.count(*i), (size_t)4);
    }


  }
  


BOOST_AUTO_TEST_SUITE_END()

