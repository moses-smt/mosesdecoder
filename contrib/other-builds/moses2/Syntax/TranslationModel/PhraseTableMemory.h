/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#pragma once

#include "../../TranslationModel/PhraseTableMemory.h"

namespace Moses2
{
namespace Syntax
{

class PhraseTableMemory : public Moses2::PhraseTableMemory
{
public:
	virtual void Load(System &system);

};

}
}


