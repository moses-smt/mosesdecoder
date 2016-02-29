/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <vector>
#include <sstream>
#include "System.h"
#include "ManagerBase.h"
#include "TargetPhrases.h"
#include "Phrase.h"
#include "InputPaths.h"
#include "InputPathBase.h"
#include "TranslationModel/PhraseTable.h"
#include "legacy/Range.h"
#include "Sentence.h"

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
	GetHypoRecycle().Clear();
}

void ManagerBase::InitPools()
{
	m_pool = &system.GetManagerPool();
	m_systemPool = &system.GetSystemPool();
	m_hypoRecycle = &system.GetHypoRecycler();
}

void ManagerBase::ParseInput(bool addBOSEOS)
{
	FactorCollection &vocab = system.GetVocab();

	m_input = Sentence::CreateFromString(GetPool(), vocab, system, m_inputStr, m_translationId, addBOSEOS);
}


}

