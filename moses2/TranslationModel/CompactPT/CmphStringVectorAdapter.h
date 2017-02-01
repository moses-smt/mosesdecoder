// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_CmphStringVectorAdapterNew_h
#define moses_CmphStringVectorAdapterNew_h

#include <cassert>
#include <cstring>

#ifdef HAVE_CMPH
#include "cmph.h"

#include "StringVector.h"

namespace Moses2
{

typedef struct {
  void *vector;
  cmph_uint32 position;
} cmph_vector_t;

template<typename ValueT, typename PosT, template<typename > class Allocator>
cmph_io_adapter_t *CmphStringVectorAdapterNew(
  StringVector<ValueT, PosT, Allocator>& sv)
{
  cmph_io_adapter_t * key_source = (cmph_io_adapter_t *) malloc(
                                     sizeof(cmph_io_adapter_t));
  cmph_vector_t * cmph_vector = (cmph_vector_t *) malloc(sizeof(cmph_vector_t));
  assert(key_source);
  assert(cmph_vector);

  cmph_vector->vector = (void *) &sv;
  cmph_vector->position = 0;
  key_source->data = (void *) cmph_vector;
  key_source->nkeys = sv.size();

  return key_source;
}

template<typename ValueT, typename PosT, template<typename > class Allocator>
int CmphStringVectorAdapterRead(void *data, char **key, cmph_uint32 *keylen)
{
  cmph_vector_t *cmph_vector = (cmph_vector_t *) data;
  StringVector<ValueT, PosT, Allocator>* sv = (StringVector<ValueT, PosT,
      Allocator>*) cmph_vector->vector;
  size_t size;
  *keylen = (*sv)[cmph_vector->position].size();
  size = *keylen;
  *key = new char[size + 1];
  std::string temp = (*sv)[cmph_vector->position];
  std::strcpy(*key, temp.c_str());
  cmph_vector->position = cmph_vector->position + 1;
  return (int) (*keylen);
}

void CmphStringVectorAdapterDispose(void *data, char *key, cmph_uint32 keylen);

void CmphStringVectorAdapterRewind(void *data);

template<typename ValueT, typename PosT, template<typename > class Allocator>
cmph_io_adapter_t* CmphStringVectorAdapter(
  StringVector<ValueT, PosT, Allocator>& sv)
{
  cmph_io_adapter_t * key_source = CmphStringVectorAdapterNew(sv);

  key_source->read = CmphStringVectorAdapterRead<ValueT, PosT, Allocator>;
  key_source->dispose = CmphStringVectorAdapterDispose;
  key_source->rewind = CmphStringVectorAdapterRewind;
  return key_source;
}

//************************************************************************//

cmph_io_adapter_t *CmphVectorAdapterNew(std::vector<std::string>& v);

int CmphVectorAdapterRead(void *data, char **key, cmph_uint32 *keylen);

void CmphVectorAdapterDispose(void *data, char *key, cmph_uint32 keylen);

void CmphVectorAdapterRewind(void *data);

cmph_io_adapter_t* CmphVectorAdapter(std::vector<std::string>& v);

}

#endif

#endif
