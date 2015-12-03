// $Id: LatticeMBRGrid.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2010 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
            this list of conditions and the following disclaimer in the documentation
            and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors
            may be used to endorse or promote products derived from this software
            without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/
/**
* Lattice MBR grid search. Enables a grid search through the four parameters (p,r,scale and prune) used in lattice MBR.
  See 'Lattice Minimum Bayes-Risk Decoding for Statistical Machine Translation by Tromble, Kumar, Och and Macherey,
    EMNLP 2008 for details of the parameters.

  The grid search is controlled by specifying comma separated lists for the lmbr parameters (-lmbr-p, -lmbr-r,
  -lmbr-pruning-factor and -mbr-scale). All other parameters are passed through to moses. If any of the lattice mbr
  parameters are missing, then they are set to their default values. Output is of the form:
   sentence-id ||| p r prune scale ||| translation-hypothesis
**/

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <set>

#include "IOWrapper.h"
#include "LatticeMBR.h"
#include "Manager.h"
#include "Timer.h"
#include "StaticData.h"

#include "util/exception.hh"

using namespace std;
using namespace Moses;
using namespace MosesCmd;

//keys
enum gridkey {lmbr_p,lmbr_r,lmbr_prune,lmbr_scale};

namespace MosesCmd
{

class Grid
{
public:
  /** Add a parameter with key, command line argument, and default value */
  void addParam(gridkey key, const string& arg, float defaultValue) {
    m_args[arg] = key;
    UTIL_THROW_IF2(m_grid.find(key) != m_grid.end(), "Duplicate parameter " << arg);
    m_grid[key].push_back(defaultValue);
  }

  /** Parse the arguments, removing those that define the grid and returning a copy of the rest */
  void parseArgs(int& argc, char**& argv) {
    char** newargv = new char*[argc+1]; //Space to add mbr parameter
    int newargc = 0;
    for (int i = 0; i < argc; ++i) {
      bool consumed = false;
      for (map<string,gridkey>::const_iterator argi = m_args.begin(); argi != m_args.end(); ++argi) {
        if (!strcmp(argv[i], argi->first.c_str())) {
          ++i;
          if (i >= argc) {
            cerr << "Error: missing parameter for " << argi->first << endl;
            throw runtime_error("Missing parameter");
          } else {
            string value = argv[i];
            gridkey key = argi->second;
            if (m_grid[key].size() != 1) {
              throw runtime_error("Duplicate grid argument");
            }
            m_grid[key].clear();
            char delim = ',';
            string::size_type lastpos = value.find_first_not_of(delim);
            string::size_type pos = value.find_first_of(delim,lastpos);
            while (string::npos != pos || string::npos != lastpos) {
              float param = atof(value.substr(lastpos, pos-lastpos).c_str());
              if (!param) {
                cerr << "Error: Illegal grid parameter for " << argi->first << endl;
                throw runtime_error("Illegal grid parameter");
              }
              m_grid[key].push_back(param);
              lastpos = value.find_first_not_of(delim,pos);
              pos = value.find_first_of(delim,lastpos);
            }
            consumed = true;
          }
          if (consumed) break;
        }
      }
      if (!consumed) {
        newargv[newargc] = new char[strlen(argv[i]) + 1];
        strcpy(newargv[newargc],argv[i]);
        ++newargc;
      }
    }
    argc = newargc;
    argv = newargv;
  }

  /** Get the grid for a particular key.*/
  const vector<float>& getGrid(gridkey key) const {
    map<gridkey,vector<float> >::const_iterator iter = m_grid.find(key);
    assert (iter != m_grid.end());
    return iter->second;

  }

private:
  map<gridkey,vector<float> > m_grid;
  map<string,gridkey> m_args;
};

} // namespace

int main(int argc, char* argv[])
{
  cerr << "Lattice MBR Grid search" << endl;

  Grid grid;
  grid.addParam(lmbr_p, "-lmbr-p", 0.5);
  grid.addParam(lmbr_r, "-lmbr-r", 0.5);
  grid.addParam(lmbr_prune, "-lmbr-pruning-factor",30.0);
  grid.addParam(lmbr_scale, "-mbr-scale",1.0);

  grid.parseArgs(argc,argv);

  Parameter* params = new Parameter();
  if (!params->LoadParam(argc,argv)) {
    params->Explain();
    exit(1);
  }
  ResetUserTime();
  if (!StaticData::LoadDataStatic(params, argv[0])) {
    exit(1);
  }

  StaticData& staticData = const_cast<StaticData&>(StaticData::Instance());
  staticData.SetUseLatticeMBR(true);
  IOWrapper* ioWrapper = GetIOWrapper(staticData);

  if (!ioWrapper) {
    throw runtime_error("Failed to initialise IOWrapper");
  }
  size_t nBestSize = staticData.GetMBRSize();

  if (nBestSize <= 0) {
    throw new runtime_error("Non-positive size specified for n-best list");
  }

  size_t lineCount = 0;
  InputType* source = NULL;

  const vector<float>& pgrid = grid.getGrid(lmbr_p);
  const vector<float>& rgrid = grid.getGrid(lmbr_r);
  const vector<float>& prune_grid = grid.getGrid(lmbr_prune);
  const vector<float>& scale_grid = grid.getGrid(lmbr_scale);

  while(ReadInput(*ioWrapper,staticData.GetInputType(),source)) {
    ++lineCount;
    Sentence sentence;
    const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);
    Manager manager(*source,staticData.GetSearchAlgorithm(), &system);
    manager.ProcessSentence();
    TrellisPathList nBestList;
    manager.CalcNBest(nBestSize, nBestList,true);
    //grid search
    for (vector<float>::const_iterator pi = pgrid.begin(); pi != pgrid.end(); ++pi) {
      float p = *pi;
      staticData.SetLatticeMBRPrecision(p);
      for (vector<float>::const_iterator ri = rgrid.begin(); ri != rgrid.end(); ++ri) {
        float r = *ri;
        staticData.SetLatticeMBRPRatio(r);
        for (vector<float>::const_iterator prune_i = prune_grid.begin(); prune_i != prune_grid.end(); ++prune_i) {
          size_t prune = (size_t)(*prune_i);
          staticData.SetLatticeMBRPruningFactor(prune);
          for (vector<float>::const_iterator scale_i = scale_grid.begin(); scale_i != scale_grid.end(); ++scale_i) {
            float scale = *scale_i;
            staticData.SetMBRScale(scale);
            cout << lineCount << " ||| " << p << " " << r << " " << prune << " " << scale << " ||| ";
            vector<Word> mbrBestHypo = doLatticeMBR(manager,nBestList);
            OutputBestHypo(mbrBestHypo, lineCount, staticData.GetReportSegmentation(),
                           staticData.GetReportAllFactors(),cout);
          }
        }

      }
    }


  }

}
