/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <contrib/other-builds/moses2/ManagerBase.h>

#include <boost/foreach.hpp>
#include <vector>
#include <sstream>
#include "System.h"
#include "TargetPhrases.h"
#include "Phrase.h"
#include "InputPaths.h"
#include "InputPath.h"
#include "TranslationModel/PhraseTable.h"
#include "legacy/Range.h"
#include "PhraseBased/Sentence.h"

using namespace std;

namespace Moses2
{
ManagerBase::ManagerBase(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId)
:system(sys)
,task(task)
,m_inputStr(inputStr)
,m_translationId(translationId)
{}

ManagerBase::~ManagerBase()
{
	GetPool().Reset();
}

void ManagerBase::InitPools()
{
	m_pool = &system.GetManagerPool();
	m_systemPool = &system.GetSystemPool();
}

void ManagerBase::ParseInput()
{
	FactorCollection &vocab = system.GetVocab();

	m_input = Sentence::CreateFromString(GetPool(), vocab, system, m_inputStr, m_translationId);
}



}

