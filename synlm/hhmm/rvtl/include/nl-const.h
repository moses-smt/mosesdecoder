///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This file is part of ModelBlocks. Copyright 2009, ModelBlocks developers. //
//                                                                           //
//    ModelBlocks is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by   //
//    the Free Software Foundation, either version 3 of the License, or      //
//    (at your option) any later version.                                    //
//                                                                           //
//    ModelBlocks is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//    GNU General Public License for more details.                           //
//                                                                           //
//    You should have received a copy of the GNU General Public License      //
//    along with ModelBlocks.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                           //
//    ModelBlocks developers designate this particular file as subject to    //
//    the "Moses" exception as provided by ModelBlocks developers in         //
//    the LICENSE file that accompanies this code.                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef __NL_CONST_H_
#define __NL_CONST_H_

//#include <getopt.h>

///////////////////////////////////////////////////////////////////////////////
// type defs...

typedef short int16 ;
typedef int16 Sample ;
//typedef int   Mag ;
//typedef int   Gam ;

///////////////////////////////////////////////////////////////////////////////
// Misc consts...

int max(int i,int j) {return (i>j)?i:j;}
int min(int i,int j) {return (i<j)?i:j;}

inline size_t rotLeft (const size_t& n, const size_t& i) { return (n << i) | (n >> (sizeof(size_t) - i)); }
inline size_t rotRight(const size_t& n, const size_t& i) { return (n >> i) | (n << (sizeof(size_t) - i)); }

//inline float abs ( float a ) { return (a>=0)?a:-a; }
/*

///////////////////////////////////////////////////////////////////////////////
// Basic phone recognition consts...
static const int NUM_SAMPLES_PER_FRAME = 512;
#ifdef OLD_Q
static const int FRAME_RATE_IN_SAMPLES = 160;  //// 80
#else
static const int FRAME_RATE_IN_SAMPLES = 256; //// 160;  //// 80
#endif

static const int LOG_NUM_FREQUENCIES = 8;
static const int LOG_NUM_QUEFRENCIES = LOG_NUM_FREQUENCIES;
static const int NUM_FREQUENCIES = 1<<LOG_NUM_FREQUENCIES;
static const int NUM_QUEFRENCIES = 1<<LOG_NUM_QUEFRENCIES;

///////////////////////////////////////////////////////////////////////////////
// Output format globals
static bool OUTPUT_QUIET = false;

///////////////////////////////////////////////////////////////////////////////
// H/O consts...

static int LOG_MAX_SIGNS = 13;  // NOTE: bit limit: LOG_MAX_SIGNS + 3*LOG_MAX_ENTS < 31
static int MAX_SIGNS     = 1<<LOG_MAX_SIGNS;
static int MAX_IVS       = 100;

///////////////////////////////////////////////////////////////////////////////
// H sign recognition consts...

static double INSERT_PENALTY   = 1.0;   // MULTIPLICATIVE
static int    MAX_FANOUT       = 150;
static const int MAX_BOOLS     = 2;
static const int MAX_TRUTHVALS = 3;

///////////////////////////////////////////////////////////////////////////////
// H sem recognition consts...

static int LOG_MAX_ENTS = 6;
static int MAX_ENTS     = 1<<LOG_MAX_ENTS;
static int MAX_CONTEXTS = 100;
static int MAX_RELNS    = 100;
static int MAX_CATS     = 1000;

///////////////////////////////////////////////////////////////////////////////
// Reader consts...

static int MAX_READER_FIELDS    = 50; //62442; //20;
static int LENGTH_READER_FIELDS = 1024; //512; //256;

///////////////////////////////////////////////////////////////////////////////
// HMM consts...

//static const int BEAM_WIDTH = 4095;
static int BEAM_WIDTH = 63; //255;
//static const int BEAM_WIDTH = 1023;

///////////////////////////////////////////////////////////////////////////////

static const int NUM_MFCC_FILTERS = 40;
static const int NUM_CEPSTRUM = 13;
static const int WEIGHT_SIZE = 8;
static const int MFCC_SIZE = 3 * NUM_CEPSTRUM;
static const float MIN_FREQUENCY = 0; //130.0;
static const float MAX_FREQUENCY = 8000.0; //Max allowed freq in signal is 16000Hz
static const int MEAN_SIZE = (WEIGHT_SIZE * MFCC_SIZE);
//Use a diagonal matrix for now
//static const int COVARIANCE_SIZE = (MEAN_SIZE *  MFCC_SIZE);
static const int COVARIANCE_SIZE = MEAN_SIZE;
static const int MAX_NUM_FRAMES = 10000;
static const float PREEMPASIZE_FACTOR = 0.97;
static const int NUM_FFT_POINTS = NUM_SAMPLES_PER_FRAME;
static const int SAMPLING_RATE = 16000;

static const bool DEBUG_MODE = false;

*/

#endif /*__NL_CONST_H_*/
