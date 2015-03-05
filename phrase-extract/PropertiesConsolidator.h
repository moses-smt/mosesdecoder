/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) University of Edinburgh

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


#pragma once

#include <string>
#include <map>


namespace MosesTraining
{

class PropertiesConsolidator
{
public:

  PropertiesConsolidator() : m_sourceLabelsFlag(false) {};

  void ActivateSourceLabelsProcessing(const std::string &sourceLabelSetFile);
  void ActivatePartsOfSpeechProcessing(const std::string &partsOfSpeechFile);

  std::string ProcessPropertiesString(const std::string &propertiesString) const;

private:

  bool m_sourceLabelsFlag;
  std::map<std::string,size_t> m_sourceLabels;
  bool m_partsOfSpeechFlag;
  std::map<std::string,size_t> m_partsOfSpeechVocabulary;

};

}  // namespace MosesTraining

