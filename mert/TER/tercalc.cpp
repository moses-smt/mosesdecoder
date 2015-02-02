/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
//
// C++ Implementation: tercalc
//
// Description:
//
//
// Author:  <>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "tercalc.h"
using namespace std;
using namespace Tools;
namespace TERCpp
{

terCalc::terCalc()
{
  TAILLE_PERMUT_MAX = 50;
  infinite = 999999.0;
  shift_cost = 1.0;
  insert_cost = 1.0;
  delete_cost = 1.0;
  substitute_cost = 1.0;
  match_cost = 0.0;
  NBR_SEGS_EVALUATED = 0;
  NBR_PERMUTS_CONSID = 0;
  NBR_BS_APPELS = 0;
  TAILLE_BEAM = 20;
  DIST_MAX_PERMUT = 50;
  PRINT_DEBUG = false;
  hypSpans.clear();
  refSpans.clear();
}


terAlignment terCalc::WERCalculation ( vector< string > hyp , vector< string > ref )
{

  return minimizeDistanceEdition ( hyp, ref, hypSpans );

}

terAlignment terCalc::TER ( std::vector< int > hyp, std::vector< int > ref )
{
  stringstream s;
  s.str ( "" );
  string stringRef ( "" );
  string stringHyp ( "" );
  for ( vector<int>::iterator l_it = ref.begin(); l_it != ref.end(); l_it++ ) {
    if ( l_it == ref.begin() ) {
      s << ( *l_it );
    } else {
      s << " " << ( *l_it );
    }
  }
  stringRef = s.str();
  s.str ( "" );
  for ( vector<int>::iterator l_itHyp = hyp.begin(); l_itHyp != hyp.end(); l_itHyp++ ) {
    if ( l_itHyp == hyp.begin() ) {
      s << ( *l_itHyp );
    } else {
      s << " " << ( *l_itHyp );
    }
  }
  stringHyp = s.str();
  s.str ( "" );
  return TER ( stringToVector ( stringRef , " " ), stringToVector ( stringHyp , " " ) );
}


hashMapInfos terCalc::createConcordMots ( vector<string> hyp, vector<string> ref )
{
  hashMap tempHash;
  hashMapInfos retour;
  for ( int i = 0; i < ( int ) hyp.size(); i++ ) {
    tempHash.addHasher ( hyp.at ( i ), "" );
  }
  bool cor[ref.size() ];
  for ( int i = 0; i < ( int ) ref.size(); i++ ) {
    if ( tempHash.trouve ( ( string ) ref.at ( i ) ) ) {
      cor[i] = true;
    } else {
      cor[i] = false;
    }
  }
  for ( int start = 0; start < ( int ) ref.size(); start++ ) {
    if ( cor[start] ) {
      for ( int end = start; ( ( end < ( int ) ref.size() ) && ( end - start <= TAILLE_PERMUT_MAX ) && ( cor[end] ) ); end++ ) {
        vector<string> ajouter = subVector ( ref, start, end + 1 );
        string ajouterString = vectorToString ( ajouter );
        vector<int> values = retour.getValue ( ajouterString );
        values.push_back ( start );
        if ( values.size() > 1 ) {
          retour.setValue ( ajouterString, values );
        } else {
          retour.addValue ( ajouterString, values );
        }
      }
    }
  }
  return retour;
}

bool terCalc::trouverIntersection ( vecInt refSpan, vecInt hypSpan )
{
  if ( ( refSpan.at ( 1 ) >= hypSpan.at ( 0 ) ) && ( refSpan.at ( 0 ) <= hypSpan.at ( 1 ) ) ) {
    return true;
  }
  return false;
}


terAlignment terCalc::minimizeDistanceEdition ( vector<string> hyp, vector<string> ref, vector<vecInt> curHypSpans )
{
  double current_best = infinite;
  double last_best = infinite;
  int first_good = 0;
  int current_first_good = 0;
  int last_good = -1;
  int cur_last_good = 0;
  int last_peak = 0;
  int cur_last_peak = 0;
  int i, j;
  double cost, icost, dcost;
  double score;



  NBR_BS_APPELS++;


  for ( i = 0; i <= ( int ) ref.size(); i++ ) {
    for ( j = 0; j <= ( int ) hyp.size(); j++ ) {
      S[i][j] = -1.0;
      P[i][j] = '0';
    }
  }
  S[0][0] = 0.0;
  for ( j = 0; j <= ( int ) hyp.size(); j++ ) {
    last_best = current_best;
    current_best = infinite;
    first_good = current_first_good;
    current_first_good = -1;
    last_good = cur_last_good;
    cur_last_good = -1;
    last_peak = cur_last_peak;
    cur_last_peak = 0;
    for ( i = first_good; i <= ( int ) ref.size(); i++ ) {
      if ( i > last_good ) {
        break;
      }
      if ( S[i][j] < 0 ) {
        continue;
      }
      score = S[i][j];
      if ( ( j < ( int ) hyp.size() ) && ( score > last_best + TAILLE_BEAM ) ) {
        continue;
      }
      if ( current_first_good == -1 ) {
        current_first_good = i ;
      }
      if ( ( i < ( int ) ref.size() ) && ( j < ( int ) hyp.size() ) ) {
        if ( ( int ) refSpans.size() ==  0 || ( int ) hypSpans.size() ==  0 || trouverIntersection ( refSpans.at ( i ), curHypSpans.at ( j ) ) ) {
          if ( ( int ) ( ref.at ( i ).compare ( hyp.at ( j ) ) ) == 0 ) {
            cost = match_cost + score;
            if ( ( S[i+1][j+1] == -1 ) || ( cost < S[i+1][j+1] ) ) {
              S[i+1][j+1] = cost;
              P[i+1][j+1] = 'A';
            }
            if ( cost < current_best ) {
              current_best = cost;
            }
            if ( current_best == cost ) {
              cur_last_peak = i + 1;
            }
          } else {
            cost = substitute_cost + score;
            if ( ( S[i+1][j+1] < 0 ) || ( cost < S[i+1][j+1] ) ) {
              S[i+1][j+1] = cost;
              P[i+1][j+1] = 'S';
              if ( cost < current_best ) {
                current_best = cost;
              }
              if ( current_best == cost ) {
                cur_last_peak = i + 1 ;
              }
            }
          }
        }
      }
      cur_last_good = i + 1;
      if ( j < ( int ) hyp.size() ) {
        icost = score + insert_cost;
        if ( ( S[i][j+1] < 0 ) || ( S[i][j+1] > icost ) ) {
          S[i][j+1] = icost;
          P[i][j+1] = 'I';
          if ( ( cur_last_peak <  i ) && ( current_best ==  icost ) ) {
            cur_last_peak = i;
          }
        }
      }
      if ( i < ( int ) ref.size() ) {
        dcost =  score + delete_cost;
        if ( ( S[ i+1][ j] < 0.0 ) || ( S[i+1][j] > dcost ) ) {
          S[i+1][j] = dcost;
          P[i+1][j] = 'D';
          if ( i >= last_good ) {
            last_good = i + 1 ;
          }
        }
      }
    }
  }


  int tracelength = 0;
  i = ref.size();
  j = hyp.size();
  while ( ( i > 0 ) || ( j > 0 ) ) {
    tracelength++;
    if ( P[i][j] == 'A' ) {
      i--;
      j--;
    } else if ( P[i][j] == 'S' ) {
      i--;
      j--;
    } else if ( P[i][j] == 'D' ) {
      i--;
    } else if ( P[i][j] == 'I' ) {
      j--;
    } else {
      cerr << "ERROR : terCalc::minimizeDistanceEdition : Invalid path : " << P[i][j] << endl;
      exit ( -1 );
    }
  }
  vector<char> path ( tracelength );
  i = ref.size();
  j = hyp.size();
  while ( ( i > 0 ) || ( j > 0 ) ) {
    path[--tracelength] = P[i][j];
    if ( P[i][j] == 'A' ) {
      i--;
      j--;
    } else if ( P[i][j] == 'S' ) {
      i--;
      j--;
    } else if ( P[i][j] == 'D' ) {
      i--;
    } else if ( P[i][j] == 'I' ) {
      j--;
    }
  }
  terAlignment to_return;
  to_return.numWords = ref.size();
  to_return.alignment = path;
  to_return.numEdits = S[ref.size() ][hyp.size() ];
  to_return.hyp = hyp;
  to_return.ref = ref;
  to_return.averageWords = (int)ref.size();
  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::minimizeDistanceEdition : to_return :" << endl << to_return.toString() << endl << "END DEBUG" << endl;
  }
  return to_return;

}
terAlignment terCalc::TER ( vector<string> hyp, vector<string> ref )
{
  hashMapInfos rloc = createConcordMots ( hyp, ref );
  terAlignment cur_align = minimizeDistanceEdition ( hyp, ref, hypSpans );
  vector<string> cur = hyp;
  cur_align.hyp = hyp;
  cur_align.ref = ref;
  cur_align.aftershift = hyp;
  double edits = 0;
//         int numshifts = 0;

  vector<terShift> allshifts;

//     cerr << "Initial Alignment:" << endl << cur_align.toString() <<endl;
  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::TER : cur_align :" << endl << cur_align.toString() << endl << "END DEBUG" << endl;
  }
  while ( true ) {
    bestShiftStruct returns;
    returns = findBestShift ( cur, hyp, ref, rloc, cur_align );
    if ( returns.m_empty ) {
      break;
    }
    terShift bestShift = returns.m_best_shift;
    cur_align = returns.m_best_align;
    edits += bestShift.cost;
    bestShift.alignment = cur_align.alignment;
    bestShift.aftershift = cur_align.aftershift;
    allshifts.push_back ( bestShift );
    cur = cur_align.aftershift;
  }
  terAlignment to_return;
  to_return = cur_align;
  to_return.allshifts = allshifts;
  to_return.numEdits += edits;
  NBR_SEGS_EVALUATED++;
  return to_return;
}
bestShiftStruct terCalc::findBestShift ( vector<string> cur, vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment med_align )
{
  bestShiftStruct to_return;
  bool anygain = false;
  bool herr[ ( int ) hyp.size() ];
  bool rerr[ ( int ) ref.size() ];
  int ralign[ ( int ) ref.size() ];
  calculateTerAlignment ( med_align, herr, rerr, ralign );
  vector<vecTerShift> poss_shifts;

  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::findBestShift (after the calculateTerAlignment call) :" << endl;
    cerr << "indices: ";
    for (int l_i=0; l_i < ( int ) ref.size() ; l_i++) {
      cerr << l_i << "\t";
    }
    cerr << endl;
    cerr << "hyp : \t"<<vectorToString(hyp ,"\t") << endl;
    cerr << "cur : \t"<<vectorToString(cur ,"\t") << endl;
    cerr << "ref : \t"<<vectorToString(ref ,"\t") << endl;
    cerr << "herr   : "<<vectorToString(herr,"\t",( int ) hyp.size()) << " | " << ( int ) hyp.size() <<endl;
    cerr << "rerr   : "<<vectorToString(rerr,"\t",( int ) ref.size()) << " | " << ( int ) ref.size() <<endl;
    cerr << "ralign : "<< vectorToString(ralign,"\t",( int ) ref.size()) << " | " << ( int ) ref.size() << endl;
    cerr << "END DEBUG " << endl;
  }
  poss_shifts = calculerPermutations ( cur, ref, rloc, med_align, herr, rerr, ralign );
  double curerr = med_align.numEdits;
  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::findBestShift :" << endl;
    cerr << "Possible Shifts:" << endl;
    for ( int i = ( int ) poss_shifts.size() - 1; i >= 0; i-- ) {
      for ( int j = 0; j < ( int ) ( poss_shifts.at ( i ) ).size(); j++ ) {
        cerr << " [" << i << "] " << ( ( poss_shifts.at ( i ) ).at ( j ) ).toString() << endl;
      }
    }
    cerr << endl;
    cerr << "END DEBUG " << endl;
  }
// 	exit(0);
  double cur_best_shift_cost = 0.0;
  terAlignment cur_best_align = med_align;
  terShift cur_best_shift;



  for ( int i = ( int ) poss_shifts.size() - 1; i >= 0; i-- ) {
    if ( PRINT_DEBUG ) {
      cerr << "BEGIN DEBUG : terCalc::findBestShift :" << endl;
      cerr << "Considering shift of length " << i << " (" << ( poss_shifts.at ( i ) ).size() << ")" << endl;
      cerr << "END DEBUG " << endl;
    }
    /* Consider shifts of length i+1 */
    double curfix = curerr - ( cur_best_shift_cost + cur_best_align.numEdits );
    double maxfix = ( 2 * ( 1 + i ) );
    if ( ( curfix > maxfix ) || ( ( cur_best_shift_cost != 0 ) && ( curfix == maxfix ) ) ) {
      break;
    }

    for ( int s = 0; s < ( int ) ( poss_shifts.at ( i ) ).size(); s++ ) {
      curfix = curerr - ( cur_best_shift_cost + cur_best_align.numEdits );
      if ( ( curfix > maxfix ) || ( ( cur_best_shift_cost != 0 ) && ( curfix == maxfix ) ) ) {
        break;
      }
      terShift curshift = ( poss_shifts.at ( i ) ).at ( s );
      if ( PRINT_DEBUG ) {
        cerr << "BEGIN DEBUG : terCalc::findBestShift :" << endl;
        cerr << "cur : "<< join(" ",cur) << endl;
        cerr << "curshift : "<< curshift.toString() << endl;

      }
      alignmentStruct shiftReturns = permuter ( cur, curshift );
      vector<string> shiftarr = shiftReturns.nwords;
      vector<vecInt> curHypSpans = shiftReturns.aftershift;

      if ( PRINT_DEBUG ) {
        cerr << "shiftarr : "<< join(" ",shiftarr) << endl;
// 		    cerr << "curHypSpans : "<< curHypSpans.toString() << endl;
        cerr << "END DEBUG " << endl;
      }
      terAlignment curalign = minimizeDistanceEdition ( shiftarr, ref, curHypSpans );

      curalign.hyp = hyp;
      curalign.ref = ref;
      curalign.aftershift = shiftarr;


      double gain = ( cur_best_align.numEdits + cur_best_shift_cost ) - ( curalign.numEdits + curshift.cost );

      // 		if (DEBUG) {
      // 	string testeuh=terAlignment join(" ", shiftarr);
      if ( PRINT_DEBUG ) {
        cerr << "BEGIN DEBUG : terCalc::findBestShift :" << endl;
        cerr << "Gain for " << curshift.toString() << " is " << gain << ". (result: [" << curalign.join ( " ", shiftarr ) << "]" << endl;
        cerr << "Details of gains : gain = ( cur_best_align.numEdits + cur_best_shift_cost ) - ( curalign.numEdits + curshift.cost )"<<endl;
        cerr << "Details of gains : gain = ("<<cur_best_align.numEdits << "+" << cur_best_shift_cost << ") - (" << curalign.numEdits << "+" <<  curshift.cost << ")"<<endl;
        cerr << "" << curalign.toString() << "\n" << endl;
        cerr << "END DEBUG " << endl;
      }
      // 		}
      //
      if ( ( gain > 0 ) || ( ( cur_best_shift_cost == 0 ) && ( gain == 0 ) ) ) {
        anygain = true;
        cur_best_shift = curshift;
        cur_best_shift_cost = curshift.cost;
        cur_best_align = curalign;
        //           if (DEBUG)
        if ( PRINT_DEBUG ) {
          cerr << "BEGIN DEBUG : terCalc::findBestShift :" << endl;
          cerr << "Tmp Choosing shift: " << cur_best_shift.toString() << " gives:\n" << cur_best_align.toString()  << "\n" << endl;
          cerr << "END DEBUG " << endl;
        }
      }
    }
  }
  if ( anygain ) {
    to_return.m_best_shift = cur_best_shift;
    to_return.m_best_align = cur_best_align;
    to_return.m_empty = false;
  } else {
    to_return.m_empty = true;
  }
  return to_return;
}

void terCalc::calculateTerAlignment ( terAlignment align, bool* herr, bool* rerr, int* ralign )
{
  int hpos = -1;
  int rpos = -1;
  if ( PRINT_DEBUG ) {

    cerr << "BEGIN DEBUG : terCalc::calculateTerAlignment : " << endl << align.toString() << endl;
    cerr << "END DEBUG " << endl;
  }
  for ( int i = 0; i < ( int ) align.alignment.size(); i++ ) {
    herr[i] = false;
    rerr[i] = false;
    ralign[i] = -1;
  }
  for ( int i = 0; i < ( int ) align.alignment.size(); i++ ) {
    char sym = align.alignment[i];
    if ( sym == 'A' ) {
      hpos++;
      rpos++;
      herr[hpos] = false;
      rerr[rpos] = false;
      ralign[rpos] = hpos;
    } else if ( sym == 'S' ) {
      hpos++;
      rpos++;
      herr[hpos] = true;
      rerr[rpos] = true;
      ralign[rpos] = hpos;
    } else if ( sym == 'I' ) {
      hpos++;
      herr[hpos] = true;
    } else if ( sym == 'D' ) {
      rpos++;
      rerr[rpos] = true;
      ralign[rpos] = hpos+1;
    } else {
      cerr << "ERROR : terCalc::calculateTerAlignment : Invalid mini align sequence " << sym << " at pos " << i << endl;
      exit ( -1 );
    }
  }
}

vector<vecTerShift> terCalc::calculerPermutations ( vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment align, bool* herr, bool* rerr, int* ralign )
{
  vector<vecTerShift> to_return;
  if ( ( TAILLE_PERMUT_MAX <= 0 ) || ( DIST_MAX_PERMUT <= 0 ) ) {
    return to_return;
  }

  vector<vecTerShift> allshifts ( TAILLE_PERMUT_MAX + 1 );
  for ( int start = 0; start < ( int ) hyp.size(); start++ ) {
    string subVectorHypString = vectorToString ( subVector ( hyp, start, start + 1 ) );
    if ( ! rloc.trouve ( subVectorHypString ) ) {
      continue;
    }

    bool ok = false;
    vector<int> mtiVec = rloc.getValue ( subVectorHypString );
    vector<int>::iterator mti = mtiVec.begin();
    while ( mti != mtiVec.end() && ( ! ok ) ) {
      int moveto = ( *mti );
      mti++;
      if ( ( start != ralign[moveto] ) && ( ( ralign[moveto] - start ) <= DIST_MAX_PERMUT ) && ( ( start - ralign[moveto] - 1 ) <= DIST_MAX_PERMUT ) ) {
        ok = true;
      }
    }
    if ( ! ok ) {
      continue;
    }
    ok = true;
    for ( int end = start; ( ok && ( end < ( int ) hyp.size() ) && ( end < start + TAILLE_PERMUT_MAX ) ); end++ ) {
      /* check if cand is good if so, add it */
      vector<string> cand = subVector ( hyp, start, end + 1 );
      ok = false;
      if ( ! ( rloc.trouve ( vectorToString ( cand ) ) ) ) {
        continue;
      }

      bool any_herr = false;

      for ( int i = 0; ( ( i <= ( end - start ) ) && ( ! any_herr ) ); i++ ) {
        if ( herr[start+i] ) {
          any_herr = true;
        }
      }
      if ( any_herr == false ) {
        ok = true;
        continue;
      }

      vector<int> movetoitVec;
      movetoitVec = rloc.getValue ( ( string ) vectorToString ( cand ) );
// 		cerr << "CANDIDATE " << ( string ) vectorToString ( cand )  <<" PLACED : " << ( string ) vectorToString ( movetoitVec," ") << endl;
      vector<int>::iterator movetoit = movetoitVec.begin();
      while ( movetoit != movetoitVec.end() ) {
        int moveto = ( *movetoit );
        movetoit++;
        if ( ! ( ( ralign[moveto] != start ) && ( ( ralign[moveto] < start ) || ( ralign[moveto] > end ) ) && ( ( ralign[moveto] - start ) <= DIST_MAX_PERMUT ) && ( ( start - ralign[moveto] ) <= DIST_MAX_PERMUT ) ) ) {
          continue;
        }
        ok = true;

        /* check to see if there are any errors in either string
        (only move if this is the case!)
        */

        bool any_rerr = false;
        for ( int i = 0; ( i <= end - start ) && ( ! any_rerr ); i++ ) {
          if ( rerr[moveto+i] ) {
            any_rerr = true;
          }
        }
        if ( ! any_rerr ) {
          continue;
        }
        for ( int roff = -1; roff <= ( end - start ); roff++ ) {
          terShift topush;
          bool topushNull = true;
          if ( ( roff == -1 ) && ( moveto == 0 ) ) {
            if ( PRINT_DEBUG ) {

              cerr << "BEGIN DEBUG : terCalc::calculerPermutations 01 : " << endl << "Consider making " << start << "..." << end << " (" << vectorToString(cand," ")<< ") moveto: " << moveto << " roff: " << roff << " ralign[mt+roff]: -1" << endl << "END DEBUG" << endl;
            }
            terShift t01 ( start, end, -1, -1 );
            topush = t01;
            topushNull = false;
          } else if  ( ( start != ralign[moveto+roff] ) && ( ( roff == 0 ) || ( ralign[moveto+roff] != ralign[moveto] ) ) ) {
            int newloc = ralign[moveto+roff];
            if ( PRINT_DEBUG ) {

              cerr << "BEGIN DEBUG : terCalc::calculerPermutations 02 : " << endl << "Consider making " << start << "..." << end << " (" << vectorToString(cand," ")<< ") moveto: " << moveto << " roff: " << roff << " ralign[mt+roff]: " << newloc << endl << "END DEBUG" << endl;
            }
            terShift t02 ( start, end, moveto + roff, newloc );
            topush = t02;
            topushNull = false;
          }
          if ( !topushNull ) {
            topush.shifted = cand;
            topush.cost  = shift_cost;
            if ( PRINT_DEBUG ) {

              cerr << "BEGIN DEBUG : terCalc::calculerPermutations 02 : " << endl;
              cerr << "start : " << start << endl;
              cerr << "end : " << end << endl;
              cerr << "end - start : " << end - start << endl;
              cerr << "END DEBUG " << endl;
            }
            ( allshifts.at ( end - start ) ).push_back ( topush );
          }
        }
      }
    }
  }
  to_return.clear();
  for ( int i = 0; i < TAILLE_PERMUT_MAX + 1; i++ ) {
    to_return.push_back ( ( vecTerShift ) allshifts.at ( i ) );
  }
  return to_return;
}


alignmentStruct terCalc::permuter ( vector<string> words, terShift s )
{
  return permuter ( words, s.start, s.end, s.newloc );
}


alignmentStruct terCalc::permuter ( vector<string> words, int start, int end, int newloc )
{
  int c = 0;
  vector<string> nwords ( words );
  vector<vecInt> spans ( ( int ) hypSpans.size() );
  alignmentStruct to_return;
  if ( PRINT_DEBUG ) {

    if ( ( int ) hypSpans.size() > 0 ) {
      cerr << "BEGIN DEBUG : terCalc::permuter :" << endl << "word length: " << ( int ) words.size() << " span length: " << ( int ) hypSpans.size() << endl ;
    } else {
      cerr << "BEGIN DEBUG : terCalc::permuter :" << endl << "word length: " << ( int ) words.size() << " span length: null" << endl ;
    }
    cerr << "BEGIN DEBUG : terCalc::permuter :" << endl << join(" ",words) << " start: " << start << " end: " << end << " newloc "<< newloc << endl << "END DEBUG " << endl;
  }
  if (newloc >=  ( int ) words.size()) {
    if ( PRINT_DEBUG ) {
      cerr << "WARNING: Relocation over the size of the hypothesis, replacing at the end of it."<<endl;
    }
    newloc =  ( int ) words.size()-1;
  }

// 		}

  if ( newloc == -1 ) {
    for ( int i = start; i <= end; i++ ) {
      nwords.at ( c++ ) = words.at ( i );
      if ( ( int ) hypSpans.size() > 0 ) {
        spans.at ( c - 1 ) = hypSpans.at ( i );
      }
    }
    for ( int i = 0; i <= start - 1; i++ ) {
      nwords.at ( c++ ) = words.at ( i );
      if ( ( int ) hypSpans.size() > 0 ) {
        spans.at ( c - 1 ) = hypSpans.at ( i );
      }
    }
    for ( int i = end + 1; i < ( int ) words.size(); i++ ) {
      nwords.at ( c++ ) = words.at ( i );
      if ( ( int ) hypSpans.size() > 0 ) {
        spans.at ( c - 1 ) = hypSpans.at ( i );
      }
    }
  } else {
    if ( newloc < start ) {

      for ( int i = 0; i < newloc; i++ ) {
        nwords.at ( c++ ) = words.at ( i );
        if ( ( int ) hypSpans.size() > 0 ) {
          spans.at ( c - 1 ) = hypSpans.at ( i );
        }
      }
      for ( int i = start; i <= end; i++ ) {
        nwords.at ( c++ ) = words.at ( i );
        if ( ( int ) hypSpans.size() > 0 ) {
          spans.at ( c - 1 ) = hypSpans.at ( i );
        }
      }
      for ( int i = newloc ; i < start ; i++ ) {
        nwords.at ( c++ ) = words.at ( i );
        if ( ( int ) hypSpans.size() > 0 ) {
          spans.at ( c - 1 ) = hypSpans.at ( i );
        }
      }
      for ( int i = end + 1; i < ( int ) words.size(); i++ ) {
        nwords.at ( c++ ) = words.at ( i );
        if ( ( int ) hypSpans.size() > 0 ) {
          spans.at ( c - 1 ) = hypSpans.at ( i );
        }
      }
    } else {
      if ( newloc > end ) {
        for ( int i = 0; i <= start - 1; i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = end + 1; i <= newloc; i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = start; i <= end; i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = newloc + 1; i < ( int ) words.size(); i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
      } else {
        // we are moving inside of ourselves
        for ( int i = 0; i <= start - 1; i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = end + 1; ( i < ( int ) words.size() ) && ( i <= ( end + ( newloc - start ) ) ); i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = start; i <= end; i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
        for ( int i = ( end + ( newloc - start ) + 1 ); i < ( int ) words.size(); i++ ) {
          nwords.at ( c++ ) = words.at ( i );
          if ( ( int ) hypSpans.size() > 0 ) {
            spans.at ( c - 1 ) = hypSpans.at ( i );
          }
        }
      }
    }
  }
  NBR_PERMUTS_CONSID++;

  if ( PRINT_DEBUG ) {
    cerr << "nwords" << join(" ",nwords) << endl;
// 	    cerr << "spans" << spans. << endl;
  }

  to_return.nwords = nwords;
  to_return.aftershift = spans;
  return to_return;
}
void terCalc::setDebugMode ( bool b )
{
  PRINT_DEBUG = b;
}

}
