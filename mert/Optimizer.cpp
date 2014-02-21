#include "Optimizer.h"

#include <cmath>
#include "util/exception.hh"
#include <vector>
#include <limits>
#include <map>
#include <cfloat>
#include <iostream>
#include <stdint.h>

#include "Point.h"
#include "Util.h"

using namespace std;

static const float MIN_FLOAT = -1.0 * numeric_limits<float>::max();
static const float MAX_FLOAT = numeric_limits<float>::max();

namespace
{

/**
 * Compute the intersection of 2 lines.
 */
inline float intersect(float m1, float b1, float m2, float b2)
{
  float isect = (b2 - b1) / (m1 - m2);
  if (!isfinite(isect)) {
    isect = MAX_FLOAT;
  }
  return isect;
}

} // namespace

namespace MosesTuning
{


Optimizer::Optimizer(unsigned Pd, const vector<unsigned>& i2O, const vector<bool>& pos, const vector<parameter_t>& start, unsigned int nrandom)
  : m_scorer(NULL), m_feature_data(), m_num_random_directions(nrandom), m_positive(pos)
{
  // Warning: the init vector is a full set of parameters, of dimension m_pdim!
  Point::m_pdim = Pd;

  UTIL_THROW_IF(start.size() != Pd, util::Exception, "Error");
  Point::m_dim = i2O.size();
  Point::m_opt_indices = i2O;
  if (Point::m_pdim > Point::m_dim) {
    for (unsigned int i = 0; i < Point::m_pdim; i++) {
      unsigned int j = 0;
      while (j < Point::m_dim && i != i2O[j])
        j++;

      // The index i wasnt found on m_opt_indices, it is a fixed index,
      // we use the value of the start vector.
      if (j == Point::m_dim)
        Point::m_fixed_weights[i] = start[i];
    }
  }
}

Optimizer::~Optimizer() {}

statscore_t Optimizer::GetStatScore(const Point& param) const
{
  vector<unsigned> bests;
  Get1bests(param, bests);
  statscore_t score = GetStatScore(bests);
  return score;
}

map<float,diff_t >::iterator AddThreshold(map<float,diff_t >& thresholdmap, float newt, const pair<unsigned,unsigned>& newdiff)
{
  map<float,diff_t>::iterator it = thresholdmap.find(newt);
  if (it != thresholdmap.end()) {
    // the threshold already exists!! this is very unlikely
    if (it->second.back().first == newdiff.first)
      // there was already a diff for this sentence, we change the 1 best;
      it->second.back().second = newdiff.second;
    else
      it->second.push_back(newdiff);
  } else {
    // normal case
    pair<map<float,diff_t>::iterator, bool> ins = thresholdmap.insert(threshold(newt, diff_t(1, newdiff)));
    UTIL_THROW_IF(!ins.second, util::Exception, "Error");                // we really inserted something
    it = ins.first;
  }
  return it;
}

statscore_t Optimizer::LineOptimize(const Point& origin, const Point& direction, Point& bestpoint) const
{
  // We are looking for the best Point on the line y=Origin+x*direction
  float min_int = 0.0001;
  //typedef pair<unsigned,unsigned> diff;//first the sentence that changes, second is the new 1best for this sentence
  //list<threshold> thresholdlist;

  map<float,diff_t> thresholdmap;
  thresholdmap[MIN_FLOAT] = diff_t();
  vector<unsigned> first1best;       // the vector of nbests for x=-inf
  for (unsigned int S = 0; S < size(); S++) {
    map<float,diff_t >::iterator previnserted = thresholdmap.begin();
    // First, we determine the translation with the best feature score
    // for each sentence and each value of x.
    //cerr << "Sentence " << S << endl;
    multimap<float, unsigned> gradient;
    vector<float> f0;
    f0.resize(m_feature_data->get(S).size());
    for (unsigned j = 0; j < m_feature_data->get(S).size(); j++) {
      // gradient of the feature function for this particular target sentence
      gradient.insert(pair<float, unsigned>(direction * (m_feature_data->get(S,j)), j));
      // compute the feature function at the origin point
      f0[j] = origin * m_feature_data->get(S, j);
    }
    // Now let's compute the 1best for each value of x.

    //    vector<pair<float,unsigned> > onebest;


    multimap<float,unsigned>::iterator gradientit = gradient.begin();
    multimap<float,unsigned>::iterator highest_f0 = gradient.begin();

    float smallest = gradientit->first;//smallest gradient
    // Several candidates can have the lowest slope (e.g., for word penalty where the gradient is an integer).

    gradientit++;
    while (gradientit != gradient.end() && gradientit->first == smallest) {
      //   cerr<<"ni"<<gradientit->second<<endl;;
      //cerr<<"fos"<<f0[gradientit->second]<<" "<<f0[index]<<" "<<index<<endl;
      if (f0[gradientit->second] > f0[highest_f0->second])
        highest_f0 = gradientit;//the highest line is the one with he highest f0
      gradientit++;
    }

    gradientit = highest_f0;
    first1best.push_back(highest_f0->second);

    // Now we look for the intersections points indicating a change of 1 best.
    // We use the fact that the function is convex, which means that the gradient can only go up.
    while (gradientit != gradient.end()) {
      map<float,unsigned>::iterator leftmost = gradientit;
      float m = gradientit->first;
      float b = f0[gradientit->second];
      multimap<float,unsigned>::iterator gradientit2 = gradientit;
      gradientit2++;
      float leftmostx = MAX_FLOAT;
      for (; gradientit2 != gradient.end(); gradientit2++) {
        //cerr<<"--"<<d++<<' '<<gradientit2->first<<' '<<gradientit2->second<<endl;
        // Look for all candidate with a gradient bigger than the current one, and
        // find the one with the leftmost intersection.
        float curintersect;
        if (m != gradientit2->first) {
          curintersect = intersect(m, b, gradientit2->first, f0[gradientit2->second]);
          //cerr << "curintersect: " << curintersect << " leftmostx: " << leftmostx << endl;
          if (curintersect<=leftmostx) {
            // We have found an intersection to the left of the leftmost we had so far.
            // We might have curintersect==leftmostx for example is 2 candidates are the same
            // in that case its better its better to update leftmost to gradientit2 to avoid some recomputing later.
            leftmostx = curintersect;
            leftmost = gradientit2; // this is the new reference
          }
        }
      }
      if (leftmost == gradientit) {
        // We didn't find any more intersections.
        // The rightmost bestindex is the one with the highest slope.

        // They should be equal but there might be.
    	UTIL_THROW_IF(abs(leftmost->first-gradient.rbegin()->first) >= 0.0001,
    			  util::Exception, "Error");
        // A small difference due to rounding error
        break;
      }
      // We have found the next intersection!

      pair<unsigned,unsigned> newd(S, leftmost->second);//new onebest for Sentence S is leftmost->second

      if (leftmostx-previnserted->first < min_int) {
        // Require that the intersection Point be at least min_int to the right of the previous
        // one (for this sentence). If not, we replace the previous intersection Point with
        // this one.
        // Yes, it can even happen that the new intersection Point is slightly to the left of
        // the old one, because of numerical imprecision. We do not check that we are to the
        // right of the penultimate point also. It this happen the 1best the interval will
        // be wrong we are going to replace previnsert by the new one because we do not want to keep
        // 2 very close threshold: if the minima is there it could be an artifact.

        map<float,diff_t>::iterator tit = thresholdmap.find(leftmostx);
        if (tit == previnserted) {
          // The threshold is the same as before can happen if 2 candidates are the same for example.
          UTIL_THROW_IF(previnserted->second.back().first != newd.first,
        		  util::Exception,
        		  "Error");
          previnserted->second.back()=newd; // just replace the 1 best for sentence S
          // previnsert doesn't change
        } else {

          if (tit == thresholdmap.end()) {
            thresholdmap[leftmostx]=previnserted->second; // We keep the diffs at previnsert
            thresholdmap.erase(previnserted); // erase old previnsert
            previnserted = thresholdmap.find(leftmostx); // point previnsert to the new threshold
            previnserted->second.back()=newd; // We update the diff for sentence S
            // Threshold already exists but is not the previous one.
          } else {
            // We append the diffs in previnsert to tit before destroying previnsert.
            tit->second.insert(tit->second.end(),previnserted->second.begin(),previnserted->second.end());
            UTIL_THROW_IF(tit->second.back().first != newd.first,
            		util::Exception,
            		"Error");
            tit->second.back()=newd;    // change diff for sentence S
            thresholdmap.erase(previnserted); // erase old previnsert
            previnserted = tit;  // point previnsert to the new threshold
          }
        }

        UTIL_THROW_IF(previnserted == thresholdmap.end(),
        		util::Exception,
        		"Error");
      } else { //normal insertion process
        previnserted = AddThreshold(thresholdmap, leftmostx, newd);
      }
      gradientit = leftmost;
    } // while (gradientit!=gradient.end()){
  }   // loop on S

  // Now the thresholdlist is up to date: it contains a list of all the parameter_ts where
  // the function changed its value, along with the nbest list for the interval after each threshold.

  map<float,diff_t >::iterator thrit;
  if (verboselevel() > 6) {
    cerr << "Thresholds:(" << thresholdmap.size() << ")" << endl;
    for (thrit = thresholdmap.begin(); thrit != thresholdmap.end(); thrit++) {
      cerr << "x: " << thrit->first << " diffs";
      for (size_t j = 0; j < thrit->second.size(); ++j) {
        cerr << " " <<thrit->second[j].first << "," << thrit->second[j].second;
      }
      cerr << endl;
    }
  }

  // Last thing to do is compute the Stat score (i.e., BLEU) and find the minimum.
  thrit = thresholdmap.begin();
  ++thrit;       // first diff corrrespond to MIN_FLOAT and first1best
  diffs_t diffs;
  for (; thrit != thresholdmap.end(); thrit++)
    diffs.push_back(thrit->second);
  vector<statscore_t> scores = GetIncStatScore(first1best, diffs);

  thrit = thresholdmap.begin();
  statscore_t bestscore = MIN_FLOAT;
  float bestx = MIN_FLOAT;

  // We skipped the first el of thresholdlist but GetIncStatScore return 1 more for first1best.
  UTIL_THROW_IF(scores.size() != thresholdmap.size(),
		  util::Exception,
		  "Error");
  for (unsigned int sc = 0; sc != scores.size(); sc++) {
    //cerr << "x=" << thrit->first << " => " << scores[sc] << endl;

    //enforce positivity
    Point respoint = origin + direction * thrit->first;
    bool is_valid = true;
    for (unsigned int k=0; k < respoint.getdim(); k++) {
      if (m_positive[k] && respoint[k] <= 0.0)
        is_valid = false;
    }

    if (is_valid && scores[sc] > bestscore) {
      // This is the score for the interval [lit2->first, (lit2+1)->first]
      // unless we're at the last score, when it's the score
      // for the interval [lit2->first,+inf].
      bestscore = scores[sc];

      // If we're not in [-inf,x1] or [xn,+inf], then just take the value
      // if x which splits the interval in half. For the rightmost interval,
      // take x to be the last interval boundary + 0.1, and for the leftmost
      // interval, take x to be the first interval boundary - 1000.
      // These values are taken from cmert.
      float leftx = thrit->first;
      if (thrit == thresholdmap.begin()) {
        leftx = MIN_FLOAT;
      }
      ++thrit;
      float rightx = MAX_FLOAT;
      if (thrit != thresholdmap.end()) {
        rightx = thrit->first;
      }
      --thrit;
      //cerr << "leftx: " << leftx << " rightx: " << rightx << endl;
      if (leftx == MIN_FLOAT) {
        bestx = rightx-1000;
      } else if (rightx == MAX_FLOAT) {
        bestx = leftx + 0.1;
      } else {
        bestx = 0.5 * (rightx + leftx);
      }
      //cerr << "x = " << "set new bestx to: " << bestx << endl;
    }
    ++thrit;
  }

  if (abs(bestx) < 0.00015) {
    // The origin of the line is the best point! We put it back at 0
    // so we do not propagate rounding erros.
    bestx = 0.0;

    // Finally, we manage to extract the best score;
    // now we convert bestx (position on the line) to a point.
    if (verboselevel() > 4)
      cerr << "best point on line at origin" << endl;
  }
  if (verboselevel() > 3) {
//    cerr<<"end Lineopt, bestx="<<bestx<<endl;
  }
  bestpoint = direction * bestx + origin;
  bestpoint.SetScore(bestscore);
  return bestscore;
}

void Optimizer::Get1bests(const Point& P, vector<unsigned>& bests) const
{
  UTIL_THROW_IF(m_feature_data == NULL, util::Exception, "Error");
  bests.clear();
  bests.resize(size());

  for (unsigned i = 0; i < size(); i++) {
    float bestfs = MIN_FLOAT;
    unsigned idx = 0;
    unsigned j;
    for (j = 0; j < m_feature_data->get(i).size(); j++) {
      float curfs = P * m_feature_data->get(i, j);
      if (curfs > bestfs) {
        bestfs = curfs;
        idx = j;
      }
    }
    bests[i]=idx;
  }

}

statscore_t Optimizer::Run(Point& P) const
{
  if (!m_feature_data) {
    cerr << "error trying to optimize without Features loaded" << endl;
    exit(2);
  }
  if (!m_scorer) {
    cerr << "error trying to optimize without a Scorer loaded" << endl;
    exit(2);
  }
  if (m_scorer->getReferenceSize() != m_feature_data->size()) {
    cerr << "error length mismatch between feature file and score file" << endl;
    exit(2);
  }

  P.SetScore(GetStatScore(P));

  if (verboselevel () > 2) {
    cerr << "Starting point: " << P << " => " << P.GetScore() << endl;
  }
  statscore_t score = TrueRun(P);

  // just in case its not done in TrueRun
  P.SetScore(score);
  if (verboselevel() > 2) {
    cerr << "Ending point: " << P << " => " << score << endl;
  }
  return score;
}


vector<statscore_t> Optimizer::GetIncStatScore(const vector<unsigned>& thefirst, const vector<vector <pair<unsigned,unsigned> > >& thediffs) const
{
  UTIL_THROW_IF(m_scorer == NULL, util::Exception, "Error");

  vector<statscore_t> theres;

  m_scorer->score(thefirst, thediffs, theres);
  return theres;
}


statscore_t SimpleOptimizer::TrueRun(Point& P) const
{
  statscore_t prevscore = 0;
  statscore_t bestscore = MIN_FLOAT;
  Point best;

  // If P is already defined and provides a score,
  // We must improve over this score.
  if (P.GetScore() > bestscore) {
    bestscore = P.GetScore();
    best = P;
  }

  int nrun = 0;
  do {
    ++nrun;
    if (verboselevel() > 2 && nrun > 1)
      cerr << "last diff=" << bestscore-prevscore << " nrun " << nrun << endl;
    prevscore = bestscore;

    Point  linebest;

    for (unsigned int d = 0; d < Point::getdim() + m_num_random_directions; d++) {
      if (verboselevel() > 4) {
        //	cerr<<"minimizing along direction "<<d<<endl;
        cerr << "starting point: " << P << " => " << prevscore << endl;
      }
      Point direction;
      if (d < Point::getdim()) { // regular updates along one dimension
        for (unsigned int i = 0; i < Point::getdim(); i++)
          direction[i]=0.0;
        direction[d]=1.0;
      } else { // random direction update
        direction.Randomize();
      }
      statscore_t curscore = LineOptimize(P, direction, linebest);//find the minimum on the line
      if (verboselevel() > 5) {
        cerr << "direction: " << d << " => " << curscore << endl;
        cerr << "\tending point: "<< linebest << " => " << curscore << endl;
      }
      if (curscore > bestscore) {
        bestscore = curscore;
        best = linebest;
        if (verboselevel() > 3) {
          cerr << "new best dir:" << d << " (" << nrun << ")" << endl;
          cerr << "new best Point " << best << " => "  << curscore << endl;
        }
      }
    }
    P = best; //update the current vector with the best point on all line tested
    if (verboselevel() > 3)
      cerr << nrun << "\t" << P << endl;
  } while (bestscore-prevscore > kEPS);

  if (verboselevel() > 2) {
    cerr << "end Powell Algo, nrun=" << nrun << endl;
    cerr << "last diff=" << bestscore-prevscore << endl;
    cerr << "\t" << P << endl;
  }
  return bestscore;
}

statscore_t RandomDirectionOptimizer::TrueRun(Point& P) const
{
  statscore_t prevscore = P.GetScore();

  // do specified number of random direction optimizations
  unsigned int nrun = 0;
  unsigned int nrun_no_change = 0;
  for (; nrun_no_change < m_num_random_directions; nrun++, nrun_no_change++) {
    // choose a random direction in which to optimize
    Point direction;
    direction.Randomize();

    //find the minimum on the line
    statscore_t score = LineOptimize(P, direction, P);
    if (verboselevel() > 4) {
      cerr << "direction: " << direction << " => " << score;
      cerr << " (" <<  (score-prevscore) << ")" << endl;
      cerr << "\tending point: " <<  P << " => " << score << endl;
    }

    if (score-prevscore > kEPS)
      nrun_no_change = 0;
    prevscore = score;
  }

  if (verboselevel() > 2) {
    cerr << "end Powell Algo, nrun=" << nrun << endl;
  }
  return prevscore;
}


statscore_t RandomOptimizer::TrueRun(Point& P) const
{
  P.Randomize();
  statscore_t score = GetStatScore(P);
  P.SetScore(score);
  return score;
}

}
