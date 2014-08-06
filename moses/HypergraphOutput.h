// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

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

#ifndef moses_Hypergraph_Output_h
#define moses_Hypergraph_Output_h

/**
* Manage the output of hypergraphs.
**/

namespace Moses {

class Manager;

class HypergraphOutput {

public:
  /** Initialise output directory and create weights file */
  HypergraphOutput(size_t precision);

  /** Write this hypergraph to file */
  void Write(const Manager& manager) const;

private:
  size_t m_precision;
  std::string m_hypergraphDir;
  std::string m_compression;
  bool m_appendSuffix;
};

}
#endif
