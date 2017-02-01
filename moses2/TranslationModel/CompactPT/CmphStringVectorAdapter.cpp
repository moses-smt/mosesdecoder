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

#ifdef HAVE_CMPH

#include "CmphStringVectorAdapter.h"

namespace Moses2
{

void CmphStringVectorAdapterDispose(void *data, char *key, cmph_uint32 keylen)
{
  delete[] key;
}

void CmphStringVectorAdapterRewind(void *data)
{
  cmph_vector_t *cmph_vector = (cmph_vector_t *) data;
  cmph_vector->position = 0;
}

//************************************************************************//

cmph_io_adapter_t *CmphVectorAdapterNew(std::vector<std::string>& v)
{
  cmph_io_adapter_t * key_source = (cmph_io_adapter_t *) malloc(
                                     sizeof(cmph_io_adapter_t));
  cmph_vector_t * cmph_vector = (cmph_vector_t *) malloc(sizeof(cmph_vector_t));
  assert(key_source);
  assert(cmph_vector);

  cmph_vector->vector = (void *) &v;
  cmph_vector->position = 0;
  key_source->data = (void *) cmph_vector;
  key_source->nkeys = v.size();

  return key_source;
}

int CmphVectorAdapterRead(void *data, char **key, cmph_uint32 *keylen)
{
  cmph_vector_t *cmph_vector = (cmph_vector_t *) data;
  std::vector<std::string>* v = (std::vector<std::string>*) cmph_vector->vector;
  size_t size;
  *keylen = (*v)[cmph_vector->position].size();
  size = *keylen;
  *key = new char[size + 1];
  std::string temp = (*v)[cmph_vector->position];
  strcpy(*key, temp.c_str());
  cmph_vector->position = cmph_vector->position + 1;
  return (int) (*keylen);
}

void CmphVectorAdapterDispose(void *data, char *key, cmph_uint32 keylen)
{
  delete[] key;
}

void CmphVectorAdapterRewind(void *data)
{
  cmph_vector_t *cmph_vector = (cmph_vector_t *) data;
  cmph_vector->position = 0;
}

cmph_io_adapter_t* CmphVectorAdapter(std::vector<std::string>& v)
{
  cmph_io_adapter_t * key_source = CmphVectorAdapterNew(v);

  key_source->read = CmphVectorAdapterRead;
  key_source->dispose = CmphVectorAdapterDispose;
  key_source->rewind = CmphVectorAdapterRewind;
  return key_source;
}

}

#endif
