// $Id: MainMT.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

/**
 * Moses main wrapper for executable for single-threaded and multi-threaded, simply calling decoder_main.
 **/
#include "moses/ExportInterface.h"
#include "moses/FF/NMT/NMT_Wrapper.h"

/** main function of the command line version of the decoder **/
int main(int argc, char** argv)
{
  
  testMe(
    "/work/nmt_model/state.pkl",
    "/work/nmt_model/min_en_de_model.npz",
    "/work/mosesdecoder2/moses/FF/NMT/wrapper",
    "/work/nmt_model/vocab/mini_en_de_.en.pkl",
    "/work/nmt_model/vocab/mini_en_de_.de.pkl");
  
  return decoder_main(argc, argv);
}

