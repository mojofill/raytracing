#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "cglm/cglm.h"
#include "material/material.h"

typedef struct Triangle {
    vec3 v0;
    float padA;
    vec3 v1;
    float padB;
    vec3 v2;
    float padC;
    Material mat;
} Triangle;

#endif