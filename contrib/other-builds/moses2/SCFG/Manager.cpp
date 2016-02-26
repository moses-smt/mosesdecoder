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

using namespace std;

namespace Moses2
{
namespace SCFG
{

Manager::Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId)
:ManagerBase(sys, task, inputStr, translationId)
{}

Manager::~Manager() {

}


void Manager::Decode()
{
	// init pools etc
	InitPools();
	ParseInput();

}

}
}

