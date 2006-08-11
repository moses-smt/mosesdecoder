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

#include <iostream>
#include <assert.h>
#include "mempool.h"
#include "htable.h"

//bitwise rotation of unsigned integers

#define rot_right(a,k) (((a) >> k ) | ((a) << (32-(k))))
#define rot_left(a,k) (((a) << k ) | ((a) >> (32-(k))))

using namespace std;

htable::htable(int n,int kl,HTYPE ht,size_t (*klf)(const char* )){
  
  if (ht!=STRPTR && ht!=STR && kl==0){
    cerr << "htable: key length must be specified for non-string entries!";
    exit(1);
  } 
    
  memory=new mempool( sizeof(entry) , BlockSize );

  table = new entry* [ size=n ];

  memset(table,0,sizeof(entry *) * n );
  
  keylen=(ht==INT || ht==INTPTR?kl/sizeof(int):kl);

  htype=ht;
  
  keys = accesses = collisions = 0;

  keylenfunc=(klf?klf:&strlen);    
   
}


char *htable::search(char *item, HT_ACTION action)

{
  address       h;
  entry        *q,**p;
  int i;

  //if (action == HT_FIND) 
  accesses++;
  
  h = Hash(item);
  
  i=(h % size);
  
  //cout << "htable::search() hash i=" << i << "\n";
  
  p = &table[h % size];

  q=*p;

  /*
  ** Follow collision chain
  */
  
  while (q != NULL && Comp((char *)q->key,(char *)item))
    {
      p = (entry **)&q->next;
      q=*p;
      //if (action == HT_FIND) 
      collisions++;
    }
  
  if (
      q != NULL                 /* found        */
      ||
      action == HT_FIND            /* not found, search only       */
      ||
      (q = (entry *)memory->alloc())
      ==
      NULL                      /* not found, no room   */
      )

    return((q!=NULL)?(char *)q->key:(char *)NULL);

  *p = q;                       /* link into chain      */
  /*
  ** Initialize new element
  */
  
  q->key = item;
  q->next = NULL;
  keys++;

  return((char *)q->key);
}


char *htable::scan(HT_ACTION action){

  char *k;
  
  if (action == HT_INIT)
    {
      scan_i=0;scan_p=table[0];
      return NULL;
    }

  // if scan_p==NULL go to the first non null pointer
  while ((scan_p==NULL) && (++scan_i<size)) scan_p=table[scan_i];

  if (scan_p!=NULL)
    {
      k=scan_p->key;
      scan_p=(entry *)scan_p->next;
      return k;
    };
   
  return NULL;
}


void htable::map(ostream& co,int cols){
  
  entry *p;
  char* img=new char[cols+1];
  
  img[cols]='\0';
  memset(img,'.',cols);
  
  co << "htable memory map: . (0 items), - (<5), # (>5)\n";
  
  for (int i=0; i<size;i++)
  {
    int n=0;p=table[i];
    
    while(p!=NULL){
      n++;
      p=(entry *)p->next;
    };
    
    if (i && (i % cols)==0){
      co << img << "\n";
      memset(img,'.',cols);
    }
    
    if (n>0)
      img[i % cols]=n<=5?'-':'#';
    
  }
  
  img[size % cols]='\0';
  co << img << "\n";
	
	delete []img;
}


void htable::stat(){
  cout << "htable class statistics\n";
  cout << "size " << size 
       << " keys " << keys
       << " acc " << accesses 
       << " coll " << collisions 
       << " used memory " << used()/1024 << "Kb\n";
}

htable::~htable()
{
  delete [] table;
  delete memory;
}


address htable::HashStr(char *key)
{
  char *Key=(htype==STRPTR? *(char **)key:key);
  int  length=(keylen?keylen:keylenfunc(Key));
  
  //cerr << "hash: " << Key << " length:" << length << "\n";

  register address h=0;
  register int i;

  for (i=0,h=0;i<length;i++)
    h = h * Prime1 ^ (Key[i] - ' ');
  h %= Prime2;

  return h;
}

//Herbert Glarner's "HSH 11/13" hash function.
/*
address htable::HashInt(char *key){
  
int *Key=(htype==INTPTR? *(int **)key:(int *)key);
  
address state=12345,h=0;
register int i,j;

int p=7;  //precision=8 * sizeof(int)-1, in general must be >=7
  
 for (i=0,h=0;i<keylen;i++){    
   h = h ^ ((address) Key[i]);
   for (j=0;j<p;j++){
     state = rot_left(state,11);   //state = state left-rotate 11 bits
     h = rot_left(h,13);           //h = h left-rotate 13 bits
     h ^= state ;                 //h = h xor state
     h = rot_left(h,(state & (address)31)); //h = h left-rotate (state mod 32) bits
     h = rot_left(h, (h & (address)31));    //h = h left-rotate (h mod 32) bits
   }
 }

 return h;
}

*/

address htable::HashInt(char *key)
{
  int *Key=(htype==INTPTR? *(int **)key:(int *)key);
   
  
  address  h;
  register int i;
  
  //Thomas Wang's 32 bit Mix Function
  for (i=0,h=0;i<keylen;i++){    
    h+=Key[i];
    h += ~(h << 15);
    h ^=  (h >> 10);
    h +=  (h << 3);
    h ^=  (h >> 6);
    h += ~(h << 11);
    h ^=  (h >> 16);    
  };
    
  return h;
}


int htable::CompStr(char *key1, char *key2)
{
  assert(key1 && key2);
 
  char *Key1=(htype==STRPTR?*(char **)key1:key1);
  char *Key2=(htype==STRPTR?*(char **)key2:key2);

  assert(Key1 && Key2);

  int length1=(keylen?keylen:keylenfunc(Key1));
  int length2=(keylen?keylen:keylenfunc(Key2));

  if (length1!=length2) return 1;
  
  register int i;
  
  for (i=0;i<length1;i++)
    if (Key1[i]!=Key2[i]) return 1;
    return 0;
}

int htable::CompInt(char *key1, char *key2)
{
  assert(key1 && key2);
  
  int *Key1=(htype==INTPTR?*(int **)key1:(int*)key1);
  int *Key2=(htype==INTPTR?*(int **)key2:(int*)key2);
  
  assert(Key1 && Key2);
      
  register int i;
  
  for (i=0;i<keylen;i++)
    if (Key1[i]!=Key2[i]) return 1;
  return 0;
}


/*
main(){

const int n=1000;
  
htable *ht=new htable(1000/5);

  char w[n][20];
  char *c;

  for (int i=0;i<n;i++) 
    {
      sprintf(w[i],"ciao%d",i);
      ht->search((char *)&w[i],HT_ENTER);
    }

  for (int i=0;i<n;i++) 
  if (ht->search((char *)&w[i],HT_FIND))
      cout << w[i] << " trovato\n" ;
    else
      cout << w[i] << " non trovato\n";

      ht->stat();
  
  delete ht;
  htable *ht2=new htable(n);
  for (int i=0;i<n;i++) 
    ht2->search((char *)&w[i],HT_ENTER);
  
  ht2->scan(INIT);
  cout << "elenco:\n";
  while ((c=ht2->scan(CONT))!=NULL)
  cout << *(char **) c << "\n";
  
  ht2->map();
}
*/







