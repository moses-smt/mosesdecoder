#include "Hildreth.h"

using namespace Moses;
using namespace std;

namespace Mira {

vector<FValue> Hildreth::optimise (const vector<FVector>& a, const vector<FValue>& b) {

  size_t i;
  int max_iter = 10000;
  float eps = 0.00000001;
  float zero = 0.000000000001;

  vector<FValue> alpha ( b.size() );
  vector<FValue> F ( b.size() );
  vector<FValue> kkt ( b.size() );

  float max_kkt = -1e100;

  size_t K = b.size();

  float A[K][K];
  bool is_computed[K];
  for ( i = 0; i < K; i++ )
  {
    A[i][i] = a[i].inner_product(a[i]);
    is_computed[i] = false;
  }

  int max_kkt_i = -1;


  for ( i = 0; i < b.size(); i++ )
  {
    F[i] = b[i];
    kkt[i] = F[i];
    if ( kkt[i] > max_kkt )
    {
      max_kkt = kkt[i];
      max_kkt_i = i;
    }
  }

  int iter = 0;
  FValue diff_alpha;
  FValue try_alpha;
  FValue add_alpha;

  while ( max_kkt >= eps && iter < max_iter )
  {

    diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
    try_alpha = alpha[max_kkt_i] + diff_alpha;
    add_alpha = 0.0;

    if ( try_alpha < 0.0 )
      add_alpha = -1.0 * alpha[max_kkt_i];
    else
      add_alpha = diff_alpha;

    alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;

    if ( !is_computed[max_kkt_i] )
    {
      for ( i = 0; i < K; i++ )
      {
        A[i][max_kkt_i] = a[i].inner_product(a[max_kkt_i] ); // for version 1
        //A[i][max_kkt_i] = 0; // for version 1
        is_computed[max_kkt_i] = true;
      }
    }

    for ( i = 0; i < F.size(); i++ )
    {
      F[i] -= add_alpha * A[i][max_kkt_i];
      kkt[i] = F[i];
      if ( alpha[i] > zero )
        kkt[i] = abs ( F[i] );
    }
    max_kkt = -1e100;
    max_kkt_i = -1;
    for ( i = 0; i < F.size(); i++ )
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }

    iter++;
  }

  return alpha;
}

vector<FValue> Hildreth::optimise (const vector<FVector>& a, const vector<FValue>& b, FValue C) {

  size_t i;
  int max_iter = 10000;
  FValue eps = 0.00000001;
  FValue zero = 0.000000000001;

  vector<FValue> alpha ( b.size() );
  vector<FValue> F ( b.size() );
  vector<FValue> kkt ( b.size() );

  float max_kkt = -1e100;

  size_t K = b.size();

  float A[K][K];
  bool is_computed[K];
  for ( i = 0; i < K; i++ )
  {
    A[i][i] = a[i].inner_product(a[i]);
    is_computed[i] = false;
  }

  int max_kkt_i = -1;


  for ( i = 0; i < b.size(); i++ )
  {
    F[i] = b[i];
    kkt[i] = F[i];
    if ( kkt[i] > max_kkt )
    {
      max_kkt = kkt[i];
      max_kkt_i = i;
    }
  }

  int iter = 0;
  FValue diff_alpha;
  FValue try_alpha;
  FValue add_alpha;

  while ( max_kkt >= eps && iter < max_iter )
  {

    diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
    try_alpha = alpha[max_kkt_i] + diff_alpha;
    add_alpha = 0.0;

    if ( try_alpha < 0.0 )
      add_alpha = -1.0 * alpha[max_kkt_i];
    else if (try_alpha > C)
			add_alpha = C - alpha[max_kkt_i];
    else
      add_alpha = diff_alpha;

    alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;

    if ( !is_computed[max_kkt_i] )
    {
      for ( i = 0; i < K; i++ )
      {
        A[i][max_kkt_i] = a[i].inner_product(a[max_kkt_i] ); // for version 1
        //A[i][max_kkt_i] = 0; // for version 1
        is_computed[max_kkt_i] = true;
      }
    }

    for ( i = 0; i < F.size(); i++ )
    {
      F[i] -= add_alpha * A[i][max_kkt_i];
      kkt[i] = F[i];
      if (alpha[i] > C - zero)
				kkt[i]=-kkt[i];
			else if (alpha[i] > zero)
				kkt[i] = abs(F[i]);

    }
    max_kkt = -1e100;
    max_kkt_i = -1;
    for ( i = 0; i < F.size(); i++ )
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }

    iter++;
  }

  return alpha;
}

  vector<FValue> Hildreth::optimise (const vector<ScoreComponentCollection>& a, const vector<FValue>& b) {

    size_t i;
    int max_iter = 10000;
    float eps = 0.00000001;
    float zero = 0.000000000001;

    vector<FValue> alpha ( b.size() );
    vector<FValue> F ( b.size() );
    vector<FValue> kkt ( b.size() );

    float max_kkt = -1e100;

    size_t K = b.size();

    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].InnerProduct(a[i]);
      is_computed[i] = false;
    }

    int max_kkt_i = -1;


    for ( i = 0; i < b.size(); i++ )
    {
      F[i] = b[i];
      kkt[i] = F[i];
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }
    }

    int iter = 0;
    FValue diff_alpha;
    FValue try_alpha;
    FValue add_alpha;

    while ( max_kkt >= eps && iter < max_iter )
    {

      diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
      try_alpha = alpha[max_kkt_i] + diff_alpha;
      add_alpha = 0.0;

      if ( try_alpha < 0.0 )
        add_alpha = -1.0 * alpha[max_kkt_i];
      else
        add_alpha = diff_alpha;

      alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;

      if ( !is_computed[max_kkt_i] )
      {
        for ( i = 0; i < K; i++ )
        {
          A[i][max_kkt_i] = a[i].InnerProduct(a[max_kkt_i] ); // for version 1
          //A[i][max_kkt_i] = 0; // for version 1
          is_computed[max_kkt_i] = true;
        }
      }

      for ( i = 0; i < F.size(); i++ )
      {
        F[i] -= add_alpha * A[i][max_kkt_i];
        kkt[i] = F[i];
        if ( alpha[i] > zero )
          kkt[i] = abs ( F[i] );
      }
      max_kkt = -1e100;
      max_kkt_i = -1;
      for ( i = 0; i < F.size(); i++ )
        if ( kkt[i] > max_kkt )
        {
          max_kkt = kkt[i];
          max_kkt_i = i;
        }

      iter++;
    }

    return alpha;
  }

  vector<FValue> Hildreth::optimise (const vector<ScoreComponentCollection>& a, const vector<FValue>& b, FValue C) {

    size_t i;
    int max_iter = 10000;
    FValue eps = 0.00000001;
    FValue zero = 0.000000000001;

    vector<FValue> alpha ( b.size() );
    vector<FValue> F ( b.size() );
    vector<FValue> kkt ( b.size() );

    float max_kkt = -1e100;

    size_t K = b.size();

    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].InnerProduct(a[i]);
      is_computed[i] = false;
    }

    int max_kkt_i = -1;


    for ( i = 0; i < b.size(); i++ )
    {
      F[i] = b[i];
      kkt[i] = F[i];
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }
    }

    int iter = 0;
    FValue diff_alpha;
    FValue try_alpha;
    FValue add_alpha;

    while ( max_kkt >= eps && iter < max_iter )
    {

      diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
      try_alpha = alpha[max_kkt_i] + diff_alpha;
      add_alpha = 0.0;

      if ( try_alpha < 0.0 )
        add_alpha = -1.0 * alpha[max_kkt_i];
      else if (try_alpha > C)
				add_alpha = C - alpha[max_kkt_i];
      else
        add_alpha = diff_alpha;

      alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;

      if ( !is_computed[max_kkt_i] )
      {
        for ( i = 0; i < K; i++ )
        {
          A[i][max_kkt_i] = a[i].InnerProduct(a[max_kkt_i] ); // for version 1
          //A[i][max_kkt_i] = 0; // for version 1
          is_computed[max_kkt_i] = true;
        }
      }

      for ( i = 0; i < F.size(); i++ )
      {
        F[i] -= add_alpha * A[i][max_kkt_i];
        kkt[i] = F[i];
        if (alpha[i] > C - zero)
					kkt[i]=-kkt[i];
				else if (alpha[i] > zero)
					kkt[i] = abs(F[i]);

      }
      max_kkt = -1e100;
      max_kkt_i = -1;
      for ( i = 0; i < F.size(); i++ )
        if ( kkt[i] > max_kkt )
        {
          max_kkt = kkt[i];
          max_kkt_i = i;
        }

      iter++;
    }

    return alpha;
  }
}
