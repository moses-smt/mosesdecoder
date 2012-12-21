// $Id: $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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

#include <stdexcept>
#include <iostream>

#include "DecodeGraph.h"
#include "DecodeStep.h"
#include "DummyScoreProducers.h"
#include "GlobalLexicalModel.h"
#include "GlobalLexicalModelUnlimited.h"
#include "WordTranslationFeature.h"
#include "PhrasePairFeature.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "TranslationSystem.h"
#include "Util.h"

using namespace std;

namespace Moses {
  
  const string TranslationSystem::DEFAULT = "default";

    TranslationSystem::TranslationSystem(const std::string& id, 
                      const WordPenaltyProducer* wpProducer,
                      const UnknownWordPenaltyProducer* uwpProducer,
                      const DistortionScoreProducer* distortionProducer)
    {
    }
    

};
