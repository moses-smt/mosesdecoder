//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

//-----------------------------------------------------------------------------

__device__ void MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed, void * out );

__device__ void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );

__device__ void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );

__global__ void hash_data(const void* key, int len, void* out);


//-----------------------------------------------------------------------------

#endif // _MURMURHASH3_H_
