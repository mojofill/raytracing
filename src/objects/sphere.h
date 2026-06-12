#ifndef SPHERE_H
#define SPHERE_H

#include "cglm/cglm.h"
#include "material/material.h"

typedef struct Sphere {
    vec3 position;
    float radius;
    Material mat;
} Sphere;

#endif