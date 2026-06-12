#ifndef CAMERA_H
#define CAMERA_H

#include "cglm/cglm.h"

// this is for std430 alignment
// in glsl every vec3 gets padded automatically to a vec4
// so adding padding here works out, padA just becomnes the .z component in the vec4
typedef struct Camera {
    vec3 position;
    float padA;
    vec3 forward;
    float padB;
    vec3 right;
    float padC;
    vec3 up;
    float padD;

    // these will only be used on cpu side to compute camera
    float yaw;
    float pitch;
    float lastX;
    float lastY;
} Camera;

#endif