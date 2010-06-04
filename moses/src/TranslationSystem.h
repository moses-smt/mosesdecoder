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

#ifndef moses_TranslationSystem_h
#define moses_TranslationSystem_h

#include <string>
#include <vector>

#include "LMList.h"

namespace Moses {

  class DecodeGraph;
  class LexicalReordering;

/**
 * Enables the configuration of multiple translation systems.
**/
class TranslationSystem {

    public:
      /** Creates a system with the given configuration */
      TranslationSystem(const std::string& config,
                        const std::vector<DecodeGraph*>& allDecoderGraphs,
                        const std::vector<LexicalReordering*>& allReorderingTables,
                        const LMList& allLMs);
      
      /** Creates a default system */
      TranslationSystem(const std::vector<DecodeGraph*>& allDecoderGraphs,
                        const std::vector<LexicalReordering*>& allReorderingTables,
                        const LMList& allLMs);
        
        const std::string& GetId() const {return m_id;}
        
        //Lists of tables relevant to this system.
        const std::vector<LexicalReordering*>& GetReorderModels() const {return m_reorderingTables;}
        const std::vector<DecodeGraph*>& GetDecodeGraphs() const {return m_decodeGraphs;}
        const LMList& GetLanguageModels() const {return m_languageModels;}
        
        static const  std::string DEFAULT;

        
        
        
    private:
        std::string m_id;
        std::vector<DecodeGraph*> m_decodeGraphs;
        std::vector<LexicalReordering*> m_reorderingTables;
        LMList m_languageModels;
};




}
#endif

