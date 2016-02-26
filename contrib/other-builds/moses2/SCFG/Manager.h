/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <queue>
#include <cstddef>
#include <string>
#include <deque>
#include "../Search/Manager.h"

namespace Moses2
{

namespace SCFG
{

class Manager : public Moses2::ManagerBase
{
public:
	Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId);

	virtual ~Manager();
	void Decode();

protected:
};

}
}

