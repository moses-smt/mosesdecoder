/******************************************************************************
 IrstLM: IRST Language Model Toolkit
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/

/*
 IrstLM: IRST Language Model Toolkit 
 Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy
 
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
 */

#ifndef MF_HTABLE_H
#define MF_HTABLE_H

#include <iostream>

#define Prime1                 37
#define Prime2                 1048583
#define BlockSize              100


// Fast arithmetic, relying on powers of 2,
// and on pre-processor concatenation property

typedef struct{
  char*                 key;
  char*                next;  // secret from user
}entry;

typedef unsigned int address;

typedef enum {HT_FIND,    //!< search: find an entry
	      HT_ENTER,   //!< search: enter an entry 
	      HT_INIT,    //!< scan: start scan
	      HT_CONT     //!< scan: continue scan
} HT_ACTION;

typedef enum {STR,     //!< string 
	      STRPTR   //!< pointer to string
}HTYPE;

//! Hash Table for strings

class htable {
  int        size;            //!< table size
  int      keylen;            //!< key length
  HTYPE     htype;            //!< type of entry pointer
  entry   **table;            //!< hash table  
  int      scan_i;            //!< scan support 
  entry   *scan_p;            //!< scan support 
  // statistics
  long       keys;            //!< # of entries 
  long   accesses;            //!< # of accesses
  long collisions;            //!< # of collisions

  mempool  *memory;           //!<  memory pool

  size_t (*keylenfunc)(const char*);          //!< function computing key length              

 public:

  //! Creates an hash table
  htable(int n,int kl=0,HTYPE ht=STRPTR,size_t (*klf)(const char* )=NULL);

  //! Destroys an and hash table
  ~htable();

  //! Computes the hash function
  address Hash(char *key);

  //! Compares the keys of two entries
  int Comp(char *Key1,char *Key2);

  //! Searches for an item
  char *search(char *item, HT_ACTION action);

  //! Scans the content
  char *scan(HT_ACTION action);

  //! Prints statistics
  void stat();
  
  //! Print a map of memory use
  void map(std::ostream& co=std::cout, int cols=80);

  //! Returns amount of used memory
  int used(){return 
	       size * sizeof(entry **) + 
	       memory->used();};
};



#endif



