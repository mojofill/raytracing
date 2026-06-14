#ifndef VOLUME_H
#define VOLUME_H

#include "cglm/cglm.h"

typedef struct HomogenousVolume {
    vec3 absorptionCoefficient; // range: [0, 1]
    float padA;
    vec3 scatteringCoefficient; // range: [0, 1]
    float padB;
    vec3 extinctionCoefficient; // can be calculated via absorption + scattering, but don't feel like recalculating every time
    float padC; 
    
    // needs a bounding box
    vec3 minXYZ; // (x0, y0, z0)
    float padD;
    vec3 maxXYZ; // (x1, y1, z1)
    float padE;  // these two vertices is all you need for a bounding box
} HomogenousVolume;

#endif