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

namespace Moses {

/**
 * Enables the configuration of multiple translation systems.
**/
class TranslationSystem {

    public:
        TranslationSystem(const std::string& config);
        
        const std::string& GetId() const {return m_id;}
        
        //list of identifiers to indicate which tables should be used.
        const std::vector<size_t>& GetReorderingTableIds() const {return m_reorderingTableIds;}
        const std::vector<size_t>& GetPhraseTableIds() const {return m_phraseTableIds;}
        const std::vector<size_t>& GetGenerationTableIds() const {return m_generationTableIds;}
        const std::vector<size_t>& GetLanguageModelIds() const {return m_languageModelIds;}

        
        
        
    private:
        std::string m_id;
        std::vector<size_t> m_reorderingTableIds;
        std::vector<size_t> m_phraseTableIds;
        std::vector<size_t> m_generationTableIds;
        std::vector<size_t> m_languageModelIds;
};




}
#endif

