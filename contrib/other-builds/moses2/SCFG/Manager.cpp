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

  for (int startPos = size - 1; startPos >= 0; --startPos) {
    InitActiveChart(startPos);

    for (int endPos = startPos + 1; endPos < size + 1; ++endPos) {
      SubPhrase sub = m_input->GetSubPhrase(startPos, endPos - startPos);
      cerr << "sub=" << sub << endl;
      Decode(startPos, endPos);
    }
  }
}

void Manager::InitActiveChart(size_t pos)
{

   InputPath &path = *m_inputPaths.GetMatrix().GetValue(pos, 0);
   cerr << "pos=" << pos << " path=" << path << endl;
   size_t numPt = system.mappings.size();
   cerr << "numPt=" << numPt << endl;

   for (size_t i = 0; i < numPt; ++i) {
     const PhraseTable &pt = *system.mappings[i];
     cerr << "START InitActiveChart" << endl;
     pt.InitActiveChart(path);
     cerr << "FINISHED InitActiveChart" << endl;
   }
}

void Manager::Decode(size_t startPos, size_t endPos)
{
  InputPath &path = *m_inputPaths.GetMatrix().GetValue(startPos, endPos - startPos);

  size_t numPt = system.mappings.size();
  cerr << "numPt=" << numPt << endl;

  for (size_t i = 0; i < numPt; ++i) {
    const PhraseTable &pt = *system.mappings[i];
    pt.Lookup(GetPool(), system, path);
  }
}

}
}

