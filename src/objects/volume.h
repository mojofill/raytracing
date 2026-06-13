#ifndef VOLUME_H
#define VOLUME_H

#include "cglm/cglm.h"

typedef struct HomogenousVolume {
    float absorptionCoefficient; // range: [0, 1]
    float scatteringCoefficient; // range: [0, 1]
    float extinctionCoefficient; // can be calculated via absorption + scattering, but don't feel like recalculating every time
    float padA; // std430 padding
    
    // needs a bounding box
    vec3 minXYZ; // (x0, y0, z0)
    float padB;
    vec3 maxXYZ; // (x1, y1, z1)
    float padC;  // these two vertices is all you need for a bounding box
} HomogenousVolume;

#endif