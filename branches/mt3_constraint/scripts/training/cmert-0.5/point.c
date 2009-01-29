// $Id$
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "point.h"

int dim = -1;

point_t *new_point() {
  point_t *point;
  point = malloc(sizeof(point_t));
  point->score = 0.0;
  point->weights = calloc(dim, sizeof(float));
  point->has_score = 0;
  return point;
}

void point_set_score(point_t *point, float score) {
  point->has_score = 1;
  point->score = score;
}

void point_delete(point_t *point) {
  free(point->weights);
  free(point);
}

point_t *random_point(point_t *min, point_t *max) {
  int i;
  point_t *point = new_point();
  for (i=0; i<dim; i++)
    point->weights[i] = min->weights[i] + (float)random()/RAND_MAX * (max->weights[i]-min->weights[i]);
  return point;
}

point_t *point_copy(point_t *point) {
  point_t *newpoint;
  int i;
  newpoint = new_point();
  newpoint->score = point->score;
  newpoint->has_score = point->has_score;
  for (i=0; i<dim; i++)
    newpoint->weights[i] = point->weights[i];
  return newpoint;
}

float point_dotproduct(point_t *point, float *y) {
  float result;
  int i;
  result = 0.0;
  for (i=0; i<dim; i++)
    result += point->weights[i] * y[i];
  return result;
}

/* Destructive operations */
void point_multiplyby(point_t *point, float k) {
  int i;
  for (i=0; i<dim; i++)
    point->weights[i] *= k;
}

void point_addto(point_t *point1, point_t *point2) {
  int i;
  for (i=0; i<dim; i++)
    point1->weights[i] += point2->weights[i];
}

void point_normalize(point_t *point) {
  int i;
  float norm = 0.0;
  for (i=0; i<dim; i++)
    //norm += point->weights[i] * point->weights[i];
    norm += fabs(point->weights[i]);
  // norm = sqrt(norm);
  for (i=0; i<dim; i++)
    point->weights[i] /= norm;
}

void point_print(point_t *point, FILE *fp, int with_score) {
  int i;
  fprintf(fp, "%f", point->weights[0]);
  for (i=1; i<dim; i++)
    fprintf(fp, " %f", point->weights[i]);
  if (point->has_score && with_score)
    fprintf(fp, " => %f", point->score);
}

point_t *read_point(FILE *fp) {
  static char buf[1000];
  char *tok, *s;
  int field;
  point_t *point;

  point = new_point();

  fgets(buf, sizeof(buf), fp);
  s = buf;
  field = 0;
  while ((tok = strsep(&s, " \t\n")) != NULL) {
    if (!*tok) // empty token
      continue;
    if (field >= dim) {
      fprintf(stderr, "read_point(): too many fields in line\n");
      return NULL;
    } else
      point->weights[field] = strtod(tok, NULL);
    field++;
  }
  if (field < dim) {
    fprintf(stderr, "read_point(): wrong number of fields in line\n");
    return NULL;
  }
  return point;
}
