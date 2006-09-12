#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "data.h"
#include "point.h"
#include "score.h"

int verbose = 2;

float min_interval = 1e-3;

typedef struct {
  float x;
  int cand;
  int *delta_comps;
} intersection_t;

intersection_t *new_intersection(float x, int cand, int *comps1, int *comps2) {
  intersection_t *inter;
  int i;
  inter = malloc(sizeof(intersection_t));
  inter->x = x;
  inter->cand = cand; // this is not used but sometimes it's handy
  inter->delta_comps = malloc(comps_n * sizeof(int));
  for (i=0; i<comps_n; i++)
    inter->delta_comps[i] = comps1[i]-comps2[i];
  return inter;
}

void intersection_delete(intersection_t *inter) {
  free(inter->delta_comps);
  free(inter);
}

int compare_intersections(intersection_t **i1, intersection_t **i2) {
  if ((*i1)->x == (*i2)->x)
    return 0;
  else if ((*i1)->x < (*i2)->x)
    return -1;
  else
    return 1;
}

float slow_bleu(data_t *data, point_t *point) {
  int sent_i, cand_i, cand_n, i;
  candidate_t *cands;
  float p, best_p;
  int best;
  int *comps;
  float score;
  int ties, totalties;

  comps = calloc(comps_n, sizeof(int));

  totalties = 0;

  for (sent_i = 0; sent_i < data->sents_n; sent_i++) {
    cands = data->sents[sent_i];
    cand_n = data->cands_n[sent_i];

    ties = 0;

    best = 0;
    best_p = point_dotproduct(point, cands[0].features);
    for (cand_i = 1; cand_i < cand_n; cand_i++) {
      p = point_dotproduct(point, cands[cand_i].features);
      if (p > best_p) {
	best_p = p;
	best = cand_i;
	ties = 0;
      } else if (p == best_p) {
	ties++;
      }
    }    
    totalties += ties;
    comps_addto(comps, cands[best].comps);
  }
  //point_print(point, stderr, 1);
  //fprintf(stderr, "\n");
  //fprintf(stderr, "slow bleu => %f\n", compute_score(comps));
  score = compute_score(comps);
  free(comps);
  return score;
}

/* Global optimization along a line (Och, 2004) */
point_t *line_optimize(data_t *data, point_t *origin, point_t *dir) {
  int sent_i, cand_i, cand_n, intersection_i;
  candidate_t *cands;
  static intersection_t **intersections = NULL;
  intersection_t *inter;
  static int intersection_max;
  int intersection_n = 0;
  int prev, leftmost;
  float x, leftmost_x, prev_x, best_x;
  float score, best_score;
  int *comps;
  point_t *point;
  int first;

  if (!origin->has_score)
    point_set_score(origin, slow_bleu(data, origin));

  if (verbose >= 2) {
    fprintf(stderr, "starting point: ");
    point_print(origin, stderr, 1);
    fprintf(stderr, "\n  direction: ");
    point_print(dir, stderr, 1);
    fprintf(stderr, "\n");
  }

  comps = calloc(comps_n, sizeof(int));

  if (intersections == NULL) {
    intersection_max = 10;
    intersections = malloc(intersection_max*sizeof(intersection_t *));
  }

  for (sent_i = 0; sent_i < data->sents_n; sent_i++) {
    cands = data->sents[sent_i];
    cand_n = data->cands_n[sent_i];

    if (verbose >= 3)
      fprintf(stderr, "sentence %d\n", sent_i);

    if (cand_n < 1)
      continue;

    /* calculate slopes and intercepts */
    for (cand_i = 0; cand_i < cand_n; cand_i++) {
      cands[cand_i].m = point_dotproduct(dir, cands[cand_i].features);
      cands[cand_i].b = point_dotproduct(origin, cands[cand_i].features);
    }

    /* find intersection points */

    /* find best candidate for x -> -inf */
    prev = -1;
    for (cand_i = 0; cand_i < cand_n; cand_i++)
      if (prev < 0 || 
	  cands[cand_i].m < cands[prev].m || 
	  cands[cand_i].m == cands[prev].m && cands[prev].b < cands[cand_i].b)
	prev = cand_i;

    if (verbose >= 3) {
      fprintf(stderr, "x->-inf cand %d\n", prev);
    }

    comps_addto(comps, cands[prev].comps);

    first = 1;
    while (1) {
      // find leftmost intersection
      leftmost = -1;
      for (cand_i = 0; cand_i < cand_n; cand_i++) {
	if (cands[prev].m == cands[cand_i].m) {
	  if (cands[cand_i].b > cands[cand_i].b)
	    fprintf(stderr, "two parallel lines and discarding the higher -- this shouldn't happen\n");
	  continue; // no intersection
	}

	/* optimization: piecewise linear function must be concave up.
	   Maybe it would be still faster to sort by slope beforehand */
	if (cands[cand_i].m < cands[prev].m)
	  continue;

	x = -(cands[prev].b-cands[cand_i].b)/(cands[prev].m-cands[cand_i].m);

	if (leftmost < 0 || x < leftmost_x) {
	  leftmost = cand_i;
	  leftmost_x = x;
	}
      }

      if (leftmost < 0)
	break; // no more intersections

      /* Require that the intersection point be at least min_interval
	 to the right of the previous one. If not, we replace the
	 previous intersection point with this one. Yes, it can even
	 happen that the new intersection point is slightly to the
	 left of the old one, because of numerical imprecision. We
	 don't check that the new point is also min_interval to the
	 right of the penultimate one. In that case, the points would
	 switch places in the sort, resulting in a bogus score for
	 that inteval. */

      if (first || leftmost_x - prev_x > min_interval) {
	if (intersection_n == intersection_max) {
	  intersection_max *= 2;
	  intersections = realloc(intersections, intersection_max*sizeof(intersection_t));
	  if (intersections == NULL)
	    fprintf(stderr, "couldn't realloc intersections\n");
	}
	intersections[intersection_n++] = new_intersection(leftmost_x, leftmost, cands[leftmost].comps, cands[prev].comps);
      } else {
	// replace the old one
	inter = new_intersection(leftmost_x, leftmost, cands[leftmost].comps, cands[prev].comps);
	comps_addto(inter->delta_comps, intersections[intersection_n-1]->delta_comps);
	intersection_delete(intersections[intersection_n-1]);
	intersections[intersection_n-1] = inter;
      }

      if (verbose >= 3)
	fprintf(stderr, "found intersection point: %f, right cand %d\n", leftmost_x, leftmost);
      prev = leftmost;
      prev_x = leftmost_x;
      first = 0;
    }
  } 

  best_score = compute_score(comps);
  //fprintf(stderr, "x->-inf => %f\n", best_score);

  if (intersection_n == 0)
    best_x = 0.0;
  else {
    qsort(intersections, intersection_n, sizeof(intersection_t *), (int(*)(const void *, const void *))compare_intersections);
    best_x = intersections[0]->x - 1000.0; // whatever
  }
  for (intersection_i = 0; intersection_i < intersection_n; intersection_i++) {
    comps_addto(comps, intersections[intersection_i]->delta_comps);
    score = compute_score(comps);
    //fprintf(stderr, "x=%f => %f\n", intersections[intersection_i]->x, score);
    if (score > best_score) {
      best_score = score;
      if (intersection_i+1 < intersection_n)
	// what if interval is zero-width?
	best_x = 0.5*(intersections[intersection_i]->x + intersections[intersection_i+1]->x);
      else
	best_x = intersections[intersection_i]->x + 0.1; // whatever
    }
  }
  //fprintf(stderr, "best_x = %f\n", best_x);
  point = point_copy(dir);
  point_multiplyby(point, best_x);
  point_addto(point, origin);
  point_set_score(point, best_score);

  if (verbose >= 2) {
    fprintf(stderr, "  ending point: ");
    point_print(point, stderr, 1);
    fprintf(stderr, "\n");
    //check_comps(data, point, comps);
  }

  for (intersection_i = 0; intersection_i < intersection_n; intersection_i++)
    intersection_delete(intersections[intersection_i]);
  free(comps);

  if (best_score < origin->score) {
    /* this can happen in the case of a tie between two candidates with different bleu component scores. just trash the point and return the starting point */
    point_delete(point);
    return point_copy(origin);
  }

  return point;
}

point_t *optimize_powell(data_t *data, point_t *point) {
  int i;
  point_t **u, **p;
  float biggestwin, totalwin, extrapolatedwin;
  int biggestwin_i;
  point_t *point_e;

  u = malloc(dim*sizeof(point_t *));
  p = malloc(dim*sizeof(point_t *));

  point = point_copy(point);
  if (!point->has_score)
    point_set_score(point, slow_bleu(data, point));

  for (i=0; i<dim; i++) {
    u[i] = new_point();
    u[i]->weights[i] = 1.0;
  }

  while (1) {
    p[0] = line_optimize(data, point, u[0]);
    biggestwin_i = 0;
    biggestwin = p[0]->score - point->score;
    for (i=1; i<dim; i++) {
      p[i] = line_optimize(data, p[i-1], u[i]);
      if (p[i]->score - p[i-1]->score > biggestwin) {
	biggestwin_i = i;
	biggestwin = p[i]->score - p[i-1]->score;
      }
    }

    totalwin = p[dim-1]->score - point->score;

    if (totalwin < 0.000001)
      break;

    // last point minus first point
    point_multiplyby(point, -1.0);
    point_addto(point, p[dim-1]);

    point_e = point_copy(point);
    point_addto(point_e, p[dim-1]);
    point_set_score(point_e, slow_bleu(data, point_e));
    extrapolatedwin = point_e->score - point->score; // point->score is the original point
    
    if (extrapolatedwin > 0 && 
	2*(2*totalwin - extrapolatedwin) * 
	powf(totalwin - biggestwin, 2.0f) <
	powf(extrapolatedwin, 2.0f)*biggestwin) {
      // replace dominant direction vector with sum vector
      point_delete(u[biggestwin_i]);
      point_normalize(point);
      u[biggestwin_i] = point;
    }
      
    point_delete(point_e);

    // optimization continues with last point
    point = p[dim-1];
    
    for (i=0; i<dim-1; i++)
      if (i != biggestwin_i)
	point_delete(p[i]);
  }

  for (i=0; i<dim; i++)
    point_delete(u[i]);

  free(u);
  free(p);

  point_normalize(point);
  return point;
}

point_t *optimize_koehn(data_t *data, point_t *point) {
  point_t *dir, **newpoints;
  int dir_i;
  int best_dir = -1;
  dir = new_point();
  newpoints = malloc(dim*sizeof(point_t *));

  point = point_copy(point);

  while (1) {
    for (dir_i = 0; dir_i < dim; dir_i++) {
      dir->weights[dir_i] = 1.0;
      newpoints[dir_i] = line_optimize(data, point, dir);
      if (best_dir < 0 || newpoints[dir_i]->score > newpoints[best_dir]->score)
	best_dir = dir_i;
      dir->weights[dir_i] = 0.0;
    }
    if (point->has_score && newpoints[best_dir]->score - point->score < 0.000001)
      break;
    
    point_delete(point);
    point = newpoints[best_dir];

    // discard the other points
    for (dir_i = 0; dir_i < dim; dir_i++)
      if (dir_i != best_dir)
	point_delete(newpoints[dir_i]);
  }
    
  point_delete(dir);
  free(newpoints);

  point_normalize(point);
  return point;
}

void usage(void) {
  fprintf(stderr, "usage: mert -d <dimensions>\n");
  exit(1);
}

int main (int argc, char **argv) {
  int point_i;
  int points_n = 20;
  point_t *min, *max;
  data_t *data;
  point_t *bestpoint, *newpoint, *startpoint;
  int i, c;
  FILE *fp;

  while ((c = getopt(argc, argv, "d:n:")) != -1) {
    switch (c) {
    case 'd':
      dim = strtol(optarg, NULL, 10);
      break;
    case 'n':
      points_n = strtol(optarg, NULL, 10);
      break;
    default:
      usage();
    }
  }
  argc -= optind;
  argv += optind;

  if (dim < 0)
    usage();

  if ((data = read_data()) == NULL) exit(1);

  fp = fopen("init.opt", "r");
  if ((min = read_point(fp)) == NULL) exit(1);
  if ((max = read_point(fp)) == NULL) exit(1);
  if ((startpoint = read_point(fp)) == NULL) exit(1);
  fclose(fp);

  bestpoint = NULL;
  for (point_i=0; point_i<points_n; point_i++) {
    fprintf(stderr, "*** point %d ***\n", point_i);
    if (point_i == 0)
      newpoint = startpoint;
    else
      newpoint = random_point(min, max);
    newpoint = optimize_koehn(data, newpoint);
    if (bestpoint == NULL || newpoint->score > bestpoint->score)
      bestpoint = newpoint; // who cares about the leak
  }
  fprintf(stderr, "Best point: ");
  point_print(bestpoint, stderr, 1);
  fprintf(stderr, "\n");

  fp = fopen("weights.txt", "w");
  point_print(bestpoint, fp, 0);
  fprintf(fp, "\n");
  fclose(fp);
}
