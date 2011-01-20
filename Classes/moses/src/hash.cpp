// $Id: hash.cpp 400 2006-07-31 02:46:59Z hieuhoang1972 $

#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/* the key */
/* the length of the key */
/* the previous hash, or an arbitrary value */
unsigned int quick_hash(register const char *k, register unsigned int length, register unsigned int initval)
{
   register unsigned int a,b,c,len;

   /* Set up the internal state */
   len = length;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = initval;         /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
      b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
      c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((unsigned int)k[10]<<24);
   case 10: c+=((unsigned int)k[9]<<16);
   case 9 : c+=((unsigned int)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((unsigned int)k[7]<<24);
   case 7 : b+=((unsigned int)k[6]<<16);
   case 6 : b+=((unsigned int)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((unsigned int)k[3]<<24);
   case 3 : a+=((unsigned int)k[2]<<16);
   case 2 : a+=((unsigned int)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}

