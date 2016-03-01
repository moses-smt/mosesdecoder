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
#include "../Sentence.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

Manager::Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId)
:ManagerBase(sys, task, inputStr, translationId)
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
	m_stacks.Init(*this, size);
	cerr << "CREATED m_stacks" << endl;

	for (int startPos = size; startPos >= 0; --startPos) {
		for (int endPos = startPos + 1; endPos < size + 1; ++endPos) {
			SubPhrase sub = m_input->GetSubPhrase(startPos, endPos - 1);
			cerr << "sub=" << sub << endl;

		}
	}
}

}
}

