#ifndef MAT_H
#define MAT_H

#include "cglm/cglm.h"

typedef struct Material {
    vec3 color;
    float reflectivity;
    vec3 emissionColor;
    float emissionStrength;
} Material;

#endif