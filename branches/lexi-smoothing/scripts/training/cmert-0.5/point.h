#ifndef POINT_H
#define POINT_H

typedef struct {
  float *weights;
  int has_score;
  float score;
} point_t;

extern int dim;

point_t *new_point();
void point_set_score(point_t *point, float score);
void point_delete(point_t *point);
point_t *point_copy(point_t *point);
point_t *random_point(point_t *min, point_t *max);
float point_dotproduct(point_t *point, float *y);
void point_multiplyby(point_t *point, float k);
void point_normalize(point_t *point);
void point_addto(point_t *point1, point_t *point2);
#include <stdio.h>
point_t *read_point(FILE *fp);
void point_print(point_t *point, FILE *fp, int with_score);

#endif
