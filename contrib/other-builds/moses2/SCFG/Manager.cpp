/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <vector>
#include <sstream>
#include "Manager.h"
#include "InputPath.h"
#include "../Sentence.h"
#include "../System.h"
#include "../TranslationModel/PhraseTable.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

Manager::Manager(System &sys, const TranslationTask &task,
    const std::string &inputStr, long translationId) :
    ManagerBase(sys, task, inputStr, translationId)
{

}

Manager::~Manager()
{

}

void Manager::Decode()
{
  // init pools etc
  cerr << "START InitPools()" << endl;
  InitPools();
  cerr << "START ParseInput()" << endl;
  ParseInput(true);

  size_t size = GetInput().GetSize();
  cerr << "size=" << size << endl;

  m_inputPaths.Init(GetInput(), *this);
  cerr << "CREATED m_inputPaths" << endl;

  m_stacks.Init(*this, size);
  cerr << "CREATED m_stacks" << endl;

  for (int startPos = size; startPos >= 0; --startPos) {
    InitActiveChart(startPos);

    for (int endPos = startPos + 1; endPos < size + 1; ++endPos) {
      SubPhrase sub = m_input->GetSubPhrase(startPos, endPos - startPos);
      cerr << "sub=" << sub << endl;

    }
  }
}

void Manager::InitActiveChart(size_t pos)
{
  /*
   InputPath &path = static_cast<InputPath&>(m_inputPaths.GetInputPath(pos, pos));
   size_t numPt = system.mappings.size();

   for (size_t i = 0; i < numPt; ++i) {
   const PhraseTable &pt = *system.mappings[i];
   cerr << "START InitActiveChart" << endl;
   pt.InitActiveChart(path);
   cerr << "FINISHED InitActiveChart" << endl;
   }
   */
}

}
}

