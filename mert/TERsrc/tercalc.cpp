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
  MAX_SHIFT_SIZE = 50;
  INF = 999999.0;
  shift_cost = 1.0;
  insert_cost = 1.0;
  delete_cost = 1.0;
  substitute_cost = 1.0;
  match_cost = 0.0;
  NUM_SEGMENTS_SCORED = 0;
  NUM_SHIFTS_CONSIDERED = 0;
  NUM_BEAM_SEARCH_CALLS = 0;
  BEAM_WIDTH = 20;
  MAX_SHIFT_DIST = 50;
  PRINT_DEBUG = false;
}


// 	terCalc::~terCalc()
// 	{
// 	}
//     size_t* terCalc::hashVec ( vector<string> s )
//     {
//         size_t retour[ ( int ) s.size() ];
//         int i=0;
//         for ( i=0; i< ( int ) s.size(); i++ )
//         {
//             boost::hash<std::string> hasher;
//             retour[i]=hasher ( s.at ( i ) );
//         }
//         return retour;
//     }


int terCalc::WERCalculation ( size_t * ref, size_t * hyp )
{
  int retour;
  int REFSize = sizeof ( ref ) + 1;
  int HYPSize = sizeof ( hyp ) + 1;
  int WER[REFSize][HYPSize];
  int i = 0;
  int j = 0;
  for ( i = 0; i < REFSize; i++ ) {
    WER[i][0] = ( int ) i;
  }
  for ( j = 0; j < HYPSize; j++ ) {
    WER[0][j] = ( int ) j;
  }
  for ( j = 1; j < HYPSize; j++ ) {
    for ( i = 1; i < REFSize; i++ ) {
      if ( i == 1 ) {
        cerr << endl;
      }
      if ( ref[i-1] == hyp[j-1] ) {
        WER[i][j] = WER[i-1][j-1];
        cerr << "- ";
        cerr << WER[i][j] << "-\t";
      } else {
        if ( ( ( WER[i-1][ j] + 1 ) < ( WER[i][j-1] + 1 ) ) && ( ( WER[i-1][j] + 1 ) < ( WER[i-1][j-1] + 1 ) ) ) {
          WER[i][j] = ( WER[i-1][j] + 1 );
// 						cerr << "D ";
          cerr << WER[i][j] << "D\t";
        } else {
          if ( ( ( WER[i][j-1] + 1 ) < ( WER[i-1][j] + 1 ) ) && ( ( WER[i][j-1] + 1 ) < ( WER[i-1][j-1] + 1 ) ) ) {
            WER[i][j] = ( WER[i][j-1] + 1 );
// 							cerr << "I ";
            cerr << WER[i][j] << "I\t";
          } else {
            WER[i][j] = ( WER[i-1][j-1] + 1 );
// 							cerr << "S ";
            cerr << WER[i][j] << "S\t";
          }
        }
      }
    }
  }
  cerr << endl;
  retour = WER[i-1][j-1];
  cerr << "i : " << i - 1 << "\tj : " << j - 1 << endl;
  return retour;
}
int terCalc::WERCalculation ( std::vector< int > ref, std::vector< int > hyp )
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
  return WERCalculation ( stringToVector ( stringRef, " " ), stringToVector ( stringHyp , " " ) );
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

int terCalc::WERCalculation ( vector<string> ref, vector<string> hyp )
{
  int retour;
  int REFSize = ( int ) ref.size() + 1;
  int HYPSize = ( int ) hyp.size() + 1;
  int WER[REFSize][HYPSize];
  char WERchar[REFSize][HYPSize];
  int i = 0;
  int j = 0;
  for ( i = 0; i < REFSize; i++ ) {
    WER[i][0] = ( int ) i;
  }
  for ( j = 0; j < HYPSize; j++ ) {
    WER[0][j] = ( int ) j;
  }
  for ( j = 1; j < HYPSize; j++ ) {
    for ( i = 1; i < REFSize; i++ ) {
// 				if (i==1)
// 				{
// 					cerr << endl;
// 				}
      if ( ref[i-1] == hyp[j-1] ) {
        WER[i][j] = WER[i-1][j-1];
// 					cerr << "- ";
// 					cerr << WER[i][j]<< "-\t";
        WERchar[i][j] = '-';
      } else {
        if ( ( ( WER[i-1][ j] + 1 ) < ( WER[i][j-1] + 1 ) ) && ( ( WER[i-1][j] + 1 ) < ( WER[i-1][j-1] + 1 ) ) ) {
          WER[i][j] = ( WER[i-1][j] + 1 );
// 						cerr << "D ";
// 						cerr << WER[i][j]<< "D\t";
          WERchar[i][j] = 'D';
        } else {
          if ( ( ( WER[i][j-1] + 1 ) < ( WER[i-1][j] + 1 ) ) && ( ( WER[i][j-1] + 1 ) < ( WER[i-1][j-1] + 1 ) ) ) {
            WER[i][j] = ( WER[i][j-1] + 1 );
// 							cerr << "I ";
// 							cerr << WER[i][j]<< "I\t";
            WERchar[i][j] = 'I';
          } else {
            WER[i][j] = ( WER[i-1][j-1] + 1 );
// 							cerr << "S ";
// 							cerr << WER[i][j]<< "S\t";
            WERchar[i][j] = 'S';
          }
        }
      }
    }
  }
  cerr << endl;
  retour = WER[REFSize-1][HYPSize-1];
  cerr << "i : " << i - 1 << "\tj : " << j - 1 << endl;
  j = HYPSize - 1;
  i = REFSize - 1;
  int k;
  stringstream s;
// 		WERalignment local[HYPSize];
  if ( HYPSize > REFSize ) {
    k = HYPSize;
  } else {
    k = REFSize;
  }
  WERalignment local;
  while ( j > 0 && i > 0 ) {
    cerr << "indice i : " << i << "\t";
    cerr << "indice j : " << j << endl;
    if ( ( j == HYPSize - 1 ) && ( i == REFSize - 1 ) ) {
      alignmentElement localInfos;
      s << WER[i][j];
      localInfos.push_back ( s.str() );
      s.str ( "" );
      s << WERchar[i][j];
      localInfos.push_back ( s.str() );
      s.str ( "" );
      local.push_back ( localInfos );
// // 				i--;
// 				j--;
    }
// 			else
    {
      if ( ( ( WER[i-1][j-1] ) <= ( WER[i-1][j] ) ) && ( ( WER[i-1][j-1] ) <= ( WER[i][j-1] ) ) ) {
        alignmentElement localInfos;
        s << WER[i-1][j-1];
        localInfos.push_back ( s.str() );
        s.str ( "" );
        s << WERchar[i-1][j-1];
        localInfos.push_back ( s.str() );
        s.str ( "" );
        local.push_back ( localInfos );
        i--;
        j--;
      } else {
        if ( ( ( WER[i][j-1] ) <= ( WER[i-1][j] ) ) && ( ( WER[i][j-1] ) <= ( WER[i-1][j-1] ) ) ) {
          alignmentElement localInfos;
          s << WER[i][j-1];
          localInfos.push_back ( s.str() );
          s.str ( "" );
          s << WERchar[i][j-1];
          localInfos.push_back ( s.str() );
          s.str ( "" );
          local.push_back ( localInfos );
          j--;
        } else {
          alignmentElement localInfos;
          s << WER[i-1][j];
          localInfos.push_back ( s.str() );
          s.str ( "" );
          s << WERchar[i-1][j];
          localInfos.push_back ( s.str() );
          s.str ( "" );
          local.push_back ( localInfos );
          i--;
        }
      }
    }
  }

  for ( j = 1; j < HYPSize; j++ ) {
    for ( i = 1; i < REFSize; i++ ) {
      cerr << WERchar[i][j] << " ";
    }
    cerr << endl;
  }
  cerr << endl;
  for ( j = 1; j < HYPSize; j++ ) {
    for ( i = 1; i < REFSize; i++ ) {
      cerr << WER[i][j] << " ";
    }
    cerr << endl;
  }

  cerr << "=================" << endl;
// 		k=local.size()-1;
// 		while (k>0)
// 		{
// 			alignmentElement localInfos;
// 			localInfos=local.at(k-1);

// 			l_WERalignment.push_back(localInfos);
// 			cerr << (string)localInfos.at(1)+"\t";
  k--;
// 		}
// 		cerr<<endl;
  k = local.size() - 1;
  int l = 0;
  int m = 0;
  while ( k > 0 ) {
    alignmentElement localInfos;
    localInfos = local.at ( k - 1 );
    if ( ( int ) ( localInfos.at ( 1 ).compare ( "D" ) ) == 0 || l > HYPSize - 1 ) {
      localInfos.push_back ( "***" );
    } else {
      localInfos.push_back ( hyp.at ( l ) );
      l++;
    }
    if ( ( int ) ( localInfos.at ( 1 ).compare ( "I" ) ) == 0 || m > REFSize - 1 ) {
      localInfos.push_back ( "***" );
    } else {
      localInfos.push_back ( ref.at ( m ) );
      m++;
    }
// 			cerr << vectorToString(localInfos)<<endl;
// 			cerr <<localInfos.at(0)<<"\t"<<localInfos.at(1)<<"\t"<<localInfos.at(2)<<"\t"<<localInfos.at(3)<<endl;
    l_WERalignment.push_back ( localInfos );
// 			cerr << (string)localInfos.at(1)+"\t";
    k--;
  }
  cerr << endl;
  /*		k=local.size()-1;
  		while (k>0)
  		{
  			alignmentElement localInfos;
  			localInfos=local.at(k-1);
  // 			l_WERalignment.push_back(localInfos);
  			cerr << (string)localInfos.at(0)+"\t";
  			k--;
  		}
  		cerr<<endl;*/
  k = 0;
// 		k=l_WERalignment.size()-1;
  m = 0;
  while ( k < ( int ) l_WERalignment.size() ) {
    alignmentElement localInfos;
    localInfos = l_WERalignment.at ( k );
    cerr << localInfos.at ( 0 ) << "\t" << localInfos.at ( 1 ) << "\t" << localInfos.at ( 2 ) << "\t" << localInfos.at ( 3 ) << endl;
    /*			if ((int)(localInfos.at(1).compare("I"))==0)
    			{
    				cerr << "***\t";
    			}
    			else
    			{
    // 				if (m<ref.size())
    				{
    					cerr <<  ref.at(m) << "\t";
    				}
    				m++;
    			}
    			*/
    k++;
  }
  cerr << endl;
  /*		k=local.size()-1;
  		l=0;
  		while (k>0)
  		{
  			alignmentElement localInfos;
  			localInfos=local.at(k-1);
  // 			l_WERalignment.push_back(localInfos);
  			if ((int)(localInfos.at(1).compare("D"))==0)
  			{
  				cerr << "***\t";
  			}
  			else
  			{
  				cerr <<  hyp.at(l) << "\t";
  				l++;
  			}
  			k--;
  		}
  		cerr<<endl;*/
  cerr << "=================" << endl;
  return retour;
}

// 	string terCalc::vectorToString(vector<string> vec)
// 	{
// 		string retour("");
// 		for (vector<string>::iterator vecIter=vec.begin();vecIter!=vec.end(); vecIter++)
// 		{
// 			retour+=(*vecIter)+"\t";
// 		}
// 		return retour;
// 	}

// 	vector<string> terCalc::subVector(vector<string> vec, int start, int end)
// 	{
// 		if (start>end)
// 		{
// 			cerr << "ERREUR : terCalc::subVector : end > start"<<endl;
// 			exit(0);
// 		}
// 		vector<string> retour;
// 		for (int i=start; ((i<end) && (i< vec.size())); i++)
// 		{
// 			retour.push_back(vec.at(i));
// 		}
// 		return retour;
// 	}

hashMapInfos terCalc::BuildWordMatches ( vector<string> hyp, vector<string> ref )
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
      for ( int end = start; ( ( end < ( int ) ref.size() ) && ( end - start <= MAX_SHIFT_SIZE ) && ( cor[end] ) ); end++ ) {
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

bool terCalc::spanIntersection ( vecInt refSpan, vecInt hypSpan )
{
  if ( ( refSpan.at ( 1 ) >= hypSpan.at ( 0 ) ) && ( refSpan.at ( 0 ) <= hypSpan.at ( 1 ) ) ) {
    return true;
  }
  return false;
}


terAlignment terCalc::MinEditDist ( vector<string> hyp, vector<string> ref, vector<vecInt> curHypSpans )
{
  double current_best = INF;
  double last_best = INF;
  int first_good = 0;
  int current_first_good = 0;
  int last_good = -1;
  int cur_last_good = 0;
  int last_peak = 0;
  int cur_last_peak = 0;
  int i, j;
  double cost, icost, dcost;
  double score;

//         int hwsize = hyp.size()-1;
//         int rwsize = ref.size()-1;
  NUM_BEAM_SEARCH_CALLS++;
// 		if ((ref.size()+1 > sizeof(S)) || (hyp.size()+1 > sizeof(S)))
// 		{
// 			int max = ref.size();
// 			if (hyp.size() > ref.size()) max = hyp.size();
// 			max += 26; // we only need a +1 here, but let's pad for future use
// 			S = new double[max][max];
// 			P = new char[max][max];
// 		}
  for ( i = 0; i <= ( int ) ref.size(); i++ ) {
    for ( j = 0; j <= ( int ) hyp.size(); j++ ) {
      S[i][j] = -1.0;
      P[i][j] = '0';
    }
  }
  S[0][0] = 0.0;
  for ( j = 0; j <= ( int ) hyp.size(); j++ ) {
    last_best = current_best;
    current_best = INF;
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
      if ( ( j < ( int ) hyp.size() ) && ( score > last_best + BEAM_WIDTH ) ) {
        continue;
      }
      if ( current_first_good == -1 ) {
        current_first_good = i ;
      }
      if ( ( i < ( int ) ref.size() ) && ( j < ( int ) hyp.size() ) ) {
        if ( ( int ) refSpans.size() ==  0 || ( int ) hypSpans.size() ==  0 || spanIntersection ( refSpans.at ( i ), curHypSpans.at ( j ) ) ) {
          if ( ( int ) ( ref.at ( i ).compare ( hyp.at ( j ) ) ) == 0 ) {
            cost = match_cost + score;
            if ( ( S[i+1][j+1] == -1 ) || ( cost < S[i+1][j+1] ) ) {
              S[i+1][j+1] = cost;
              P[i+1][j+1] = ' ';
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
    if ( P[i][j] == ' ' ) {
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
      cerr << "ERROR : terCalc::MinEditDist : Invalid path : " << P[i][j] << endl;
      exit ( -1 );
    }
  }
  vector<char> path ( tracelength );
  i = ref.size();
  j = hyp.size();
  while ( ( i > 0 ) || ( j > 0 ) ) {
    path[--tracelength] = P[i][j];
    if ( P[i][j] == ' ' ) {
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
  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::MinEditDist : to_return :" << endl << to_return.toString() << endl << "END DEBUG" << endl;
  }
  return to_return;

}
terAlignment terCalc::TER ( vector<string> hyp, vector<string> ref )
{
  hashMapInfos rloc = BuildWordMatches ( hyp, ref );
  terAlignment cur_align = MinEditDist ( hyp, ref, hypSpans );
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
    returns = CalcBestShift ( cur, hyp, ref, rloc, cur_align );
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
  NUM_SEGMENTS_SCORED++;
  return to_return;
}
bestShiftStruct terCalc::CalcBestShift ( vector<string> cur, vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment med_align )
{
  bestShiftStruct to_return;
  bool anygain = false;
  bool herr[ ( int ) hyp.size() ];
  bool rerr[ ( int ) ref.size() ];
  int ralign[ ( int ) ref.size() ];
  FindAlignErr ( med_align, herr, rerr, ralign );
  vector<vecTerShift> poss_shifts;
  poss_shifts = GatherAllPossShifts ( cur, ref, rloc, med_align, herr, rerr, ralign );
  double curerr = med_align.numEdits;
  if ( PRINT_DEBUG ) {
    cerr << "BEGIN DEBUG : terCalc::CalcBestShift :" << endl;
    cerr << "Possible Shifts:" << endl;
    for ( int i = ( int ) poss_shifts.size() - 1; i >= 0; i-- ) {
      for ( int j = 0; j < ( int ) ( poss_shifts.at ( i ) ).size(); j++ ) {
        cerr << " [" << i << "] " << ( ( poss_shifts.at ( i ) ).at ( j ) ).toString() << endl;
      }
    }
    cerr << endl;
    cerr << "END DEBUG " << endl;
  }
  double cur_best_shift_cost = 0.0;
  terAlignment cur_best_align = med_align;
  terShift cur_best_shift;



  for ( int i = ( int ) poss_shifts.size() - 1; i >= 0; i-- ) {
    if ( PRINT_DEBUG ) {
      cerr << "BEGIN DEBUG : terCalc::CalcBestShift :" << endl;
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

      alignmentStruct shiftReturns = PerformShift ( cur, curshift );
      vector<string> shiftarr = shiftReturns.nwords;
      vector<vecInt> curHypSpans = shiftReturns.aftershift;

      terAlignment curalign = MinEditDist ( shiftarr, ref, curHypSpans );

      curalign.hyp = hyp;
      curalign.ref = ref;
      curalign.aftershift = shiftarr;

      double gain = ( cur_best_align.numEdits + cur_best_shift_cost ) - ( curalign.numEdits + curshift.cost );

      // 		if (DEBUG) {
      // 	string testeuh=terAlignment join(" ", shiftarr);
      if ( PRINT_DEBUG ) {
        cerr << "BEGIN DEBUG : terCalc::CalcBestShift :" << endl;
        cerr << "Gain for " << curshift.toString() << " is " << gain << ". (result: [" << curalign.join ( " ", shiftarr ) << "]" << endl;
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
          cerr << "BEGIN DEBUG : terCalc::CalcBestShift :" << endl;
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

void terCalc::FindAlignErr ( terAlignment align, bool* herr, bool* rerr, int* ralign )
{
  int hpos = -1;
  int rpos = -1;
  if ( PRINT_DEBUG ) {

    cerr << "BEGIN DEBUG : terCalc::FindAlignErr : " << endl << align.toString() << endl;
    cerr << "END DEBUG " << endl;
  }
  for ( int i = 0; i < ( int ) align.alignment.size(); i++ ) {
    char sym = align.alignment[i];
    if ( sym == ' ' ) {
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
      ralign[rpos] = hpos;
    } else {
      cerr << "ERROR : terCalc::FindAlignErr : Invalid mini align sequence " << sym << " at pos " << i << endl;
      exit ( -1 );
    }
  }
}

vector<vecTerShift> terCalc::GatherAllPossShifts ( vector<string> hyp, vector<string> ref, hashMapInfos rloc, terAlignment align, bool* herr, bool* rerr, int* ralign )
{
  vector<vecTerShift> to_return;
  // Don't even bother to look if shifts can't be done
  if ( ( MAX_SHIFT_SIZE <= 0 ) || ( MAX_SHIFT_DIST <= 0 ) ) {
// 			terShift[][] to_return = new terShift[0][];
    return to_return;
  }

  vector<vecTerShift> allshifts ( MAX_SHIFT_SIZE + 1 );

// 		ArrayList[] allshifts = new ArrayList[MAX_SHIFT_SIZE+1];
// 		for (int i = 0; i < allshifts.length; i++)
// 		{
// 			allshifts[i] = new ArrayList();
// 		}

// 		List hyplist = Arrays.asList(hyp);
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
      if ( ( start != ralign[moveto] ) && ( ( ralign[moveto] - start ) <= MAX_SHIFT_DIST ) && ( ( start - ralign[moveto] - 1 ) <= MAX_SHIFT_DIST ) ) {
        ok = true;
      }
    }
    if ( ! ok ) {
      continue;
    }
    ok = true;
    for ( int end = start; ( ok && ( end < ( int ) hyp.size() ) && ( end < start + MAX_SHIFT_SIZE ) ); end++ ) {
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
      vector<int>::iterator movetoit = movetoitVec.begin();
      while ( movetoit != movetoitVec.end() ) {
        int moveto = ( *movetoit );
        movetoit++;
        if ( ! ( ( ralign[moveto] != start ) && ( ( ralign[moveto] < start ) || ( ralign[moveto] > end ) ) && ( ( ralign[moveto] - start ) <= MAX_SHIFT_DIST ) && ( ( start - ralign[moveto] ) <= MAX_SHIFT_DIST ) ) ) {
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

              cerr << "BEGIN DEBUG : terCalc::GatherAllPossShifts 01 : " << endl << "Consider making " << start << "..." << end << " moveto: " << moveto << " roff: " << roff << " ralign[mt+roff]: -1" << endl << "END DEBUG" << endl;
            }
            terShift t01 ( start, end, -1, -1 );
            topush = t01;
            topushNull = false;
          } else if ( ( start != ralign[moveto+roff] ) && ( ( roff == 0 ) || ( ralign[moveto+roff] != ralign[moveto] ) ) ) {
            int newloc = ralign[moveto+roff];
            if ( PRINT_DEBUG ) {

              cerr << "BEGIN DEBUG : terCalc::GatherAllPossShifts 02 : " << endl << "Consider making " << start << "..." << end << " moveto: " << moveto << " roff: " << roff << " ralign[mt+roff]: " << newloc << endl << "END DEBUG" << endl;
            }
            terShift t02 ( start, end, moveto + roff, newloc );
            topush = t02;
            topushNull = false;
          }
          if ( !topushNull ) {
            topush.shifted = cand;
            topush.cost  = shift_cost;
            if ( PRINT_DEBUG ) {

              cerr << "BEGIN DEBUG : terCalc::GatherAllPossShifts 02 : " << endl;
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
// 		vector<vecTerShift> to_return;
  to_return.clear();
// 		terShift[][] to_return = new terShift[MAX_SHIFT_SIZE+1][];
  for ( int i = 0; i < MAX_SHIFT_SIZE + 1; i++ ) {
// 		to_return[i] = (terShift[]) allshifts[i].toArray(new terShift[0]);
    to_return.push_back ( ( vecTerShift ) allshifts.at ( i ) );
  }
  return to_return;
}


alignmentStruct terCalc::PerformShift ( vector<string> words, terShift s )
{
  return PerformShift ( words, s.start, s.end, s.newloc );
}


alignmentStruct terCalc::PerformShift ( vector<string> words, int start, int end, int newloc )
{
  int c = 0;
  vector<string> nwords ( words );
  vector<vecInt> spans ( ( int ) hypSpans.size() );
  alignmentStruct toreturn;
// ON EST ICI
// 		if((int)hypSpans.size()>0) spans = new TERintpair[(int)hypSpans.size()];
// 		if(DEBUG) {
  if ( PRINT_DEBUG ) {

    if ( ( int ) hypSpans.size() > 0 ) {
      cerr << "BEGIN DEBUG : terCalc::PerformShift :" << endl << "word length: " << ( int ) words.size() << " span length: " << ( int ) hypSpans.size() << endl << "END DEBUG " << endl;
    } else {
      cerr << "BEGIN DEBUG : terCalc::PerformShift :" << endl << "word length: " << ( int ) words.size() << " span length: null" << endl << "END DEBUG " << endl;
    }
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
      for ( int i = 0; i <= newloc; i++ ) {
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
      for ( int i = newloc + 1; i <= start - 1; i++ ) {
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
  NUM_SHIFTS_CONSIDERED++;

  toreturn.nwords = nwords;
  toreturn.aftershift = spans;
  return toreturn;
}
void terCalc::setDebugMode ( bool b )
{
  PRINT_DEBUG = b;
}

}
