#include <dc/pvr.h>
#include <dc/video.h>
#include <GL/gl.h>
#include <GL/glkos.h>
#include <stdio.h>
#include <stdlib.h>
#include "../opts.h"

extern size_t polycnt;
extern pvr_poly_hdr_t hdr;
extern int seed;

static inline int getnum(int *seed, int mn) {
    int num = (*seed & ((mn) - 1));
    *seed = *seed * 1664525 + 1013904223;
    return num;
}

static inline void get_xyc(int *seed, int *x, int *y, int *col) {
    *x = (*x + ((getnum(seed, 64)) - 32)) & 1023;
    *y = (*y + ((getnum(seed, 64)) - 32)) & 511;
    *col = getnum(seed, INT32_MAX) | 0xff000000;
}

static inline void get_xy(int *seed, int *x, int *y) {
    *x = (*x + ((getnum(seed, 64)) - 32)) & 1023;
    *y = (*y + ((getnum(seed, 64)) - 32)) & 511;
}

static inline void get_xc(int *seed, int *x, int *col) {
    *x = (*x + ((getnum(seed, 64)) - 32)) & 1023;
    *col = getnum(seed, INT32_MAX) | 0xff000000;
}

static inline void get_yc(int *seed, int *y, int *col) {
    *y = (*y + ((getnum(seed, 64)) - 32)) & 511;
    *col = getnum(seed, INT32_MAX) | 0xff000000;
}

void direct_tri_frame();
void deferred_tri_frame();
void gldc_tri_frame();
void direct_strip_frame();
void deferred_strip_frame();
void gldc_strip_frame();
void direct_quad_frame();
void deferred_quad_frame();
void gldc_quad_frame();
