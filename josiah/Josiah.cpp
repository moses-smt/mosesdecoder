/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include <iostream>

#include <boost/program_options.hpp>

#include "Decoder.h"
#include "Gibbler.h"
#include "GibbsOperator.h"

using namespace std;
using namespace Josiah;
using namespace Moses;

/**
  * Output probabilities of derivations or translations of a given set of source sentences.
 **/
int main(int argc, char** argv) {
  //TODO: Get these from command line
  std::string mosesini = "josiah/model/moses.ini";
  std::string inputfile = "source";
  int mosesdbg = 2;
  size_t iterations = 10;
  
  //set up moses
  initMoses(mosesini,mosesdbg);
  Decoder* decoder = new MosesDecoder();
  
  
  Sampler sampler;
  sampler.AddOperator(new MergeSplitOperator());
  sampler.AddOperator(new TranslationSwapOperator());
  sampler.AddCollector(new PrintSampleCollector());
  sampler.SetIterations(iterations);
  
  TranslationOptionCollection* toc;
  Hypothesis* hypothesis;
  decoder->decode("das ist recht gut , afrika",hypothesis,toc);
  sampler.Run(hypothesis,toc);
  
  
  delete decoder;
}
