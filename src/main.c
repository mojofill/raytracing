#include "renderer/renderer.h"
#include "objects/objects.h"
#include "camera/camera.h"
#include "cglm/cglm.h"

#define NUM_SAMPLES 1
#define MAX_DEPTH 5
#define ASYMMETRY_PARAMETER 0.98 // Henyey-Greenstein asymmetry parameter. Range: [-1, 1]
#define MAX_FRAMES_RAN 100
#define STOP 0

// #define RENDER_SPHERES
// #define RENDER_TRIANGLES
#define RENDER_HOMOGENOUS_VOLUMES
#define RENDER_EMISSIVE_SPHERES
// #define RENDER_EMISSIVE_TRIANGLES

void updateUniforms(vk_context *vko, uint32_t currentFrame) {
    UniformBufferObject ubo = {0};
    ubo.width = vko->width;
    ubo.height = vko->height;
    
    #ifdef RENDER_SPHERES
        ubo.sphereCount = vko->sphereCount;
    #else
        ubo.sphereCount = 0;
    #endif
    
    #ifdef RENDER_TRIANGLES
        ubo.triangleCount = vko->triangleCount;
    #else
        ubo.triangleCount = 0;
    #endif

    #ifdef RENDER_HOMOGENOUS_VOLUMES
        ubo.homogenousVolumesCount = vko->homogenousVolumesCount;
    #else
        ubo.homogenousVolumesCount = 0;
    #endif

    ubo.cam = vko->cam;
    ubo.numSamples = NUM_SAMPLES;
    ubo.maxDepth = MAX_DEPTH;
    ubo.frameCount = vko->frameCount;
    ubo.g = ASYMMETRY_PARAMETER;

    #ifdef RENDER_EMISSIVE_SPHERES
        ubo.emissiveSpheresCount = vko->emissiveSpheresCount;
    #else
        ubo.emissiveSpheresCount = 0;
    #endif

    #ifdef RENDER_EMISSIVE_TRIANGLES
        ubo.emissiveTrianglesCount = vko->emissiveTrianglesCount;
    #else
        ubo.emissiveTrianglesCount = 0;
    #endif

    ubo.totalFrameCount = vko->totalFrameCount;

    memcpy(vko->uniformBuffersMapped[currentFrame], &ubo, sizeof(UniformBufferObject));
}

void resetStorageImage(vk_context *vko) {
    VkClearColorValue clearColor = {
        .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
    };

    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    VkCommandBuffer cmd = beginSingleTimeCommands(vko);
    vkCmdClearColorImage(
        cmd,
        vko->storageImage,
        VK_IMAGE_LAYOUT_GENERAL,
        &clearColor,
        1,
        &range
    );
    endSingleTimeCommands(vko, cmd);

    vko->frameCount = 0; // reset this
}

void updateCamera(vk_context *vko) {
    Camera *camera = &vko->cam;
    int didMove = 0;
    float speed = 0.25f;
    float sensitivity = 0.002f;

    double xpos;
    double ypos;
    glfwGetCursorPos(vko->window, &xpos, &ypos);
    
    float xoffset = -(float)(xpos - camera->lastX);
    float yoffset = (float)(camera->lastY - ypos);

    if (fabsf(xoffset) > 0.0f || fabsf(yoffset) > 0.0f) didMove = 1;

    camera->lastX = xpos;
    camera->lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update camera yaw and pitch
    camera->yaw += xoffset;
    camera->pitch += yoffset;

    // Clamp pitch
    float limit = GLM_PI_2 - 0.001f;
    camera->pitch = glm_clamp(camera->pitch, -limit, limit);

    // recompute forward
    camera->forward[0] = cosf(camera->pitch) * cosf(camera->yaw);
    camera->forward[1] = cosf(camera->pitch) * sinf(camera->yaw);
    camera->forward[2] = sinf(camera->pitch);

    glm_normalize(camera->forward);

    // essentially this
    // camera.right = normalize(cross(camera->forward, (vec3) {0, 0, 1}));
    glm_vec3_cross(camera->forward, (vec3) {0, 0, 1}, camera->right);
    glm_normalize(camera->right);

    // camera.up = cross(camera.right, camera.forward);
    glm_vec3_cross(camera->right, camera->forward, camera->up);
    glm_normalize(camera->up);

    vec3 flatForward;
    flatForward[0] = camera->forward[0];
    flatForward[1] = camera->forward[1];
    flatForward[2] = 0;

    glm_normalize(flatForward);

    vec3 flatRight;
    glm_cross(flatForward, (vec3) {0, 0, 1}, flatRight);
    glm_normalize(flatRight);

    if (glfwGetKey(vko->window, GLFW_KEY_W) == GLFW_PRESS) {
        vec3 scaledForward;
        glm_vec3_scale(flatForward, speed, scaledForward);
        glm_vec3_add(camera->position, scaledForward, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_S) == GLFW_PRESS) {
        vec3 scaledForward;
        glm_vec3_scale(flatForward, speed, scaledForward);
        glm_vec3_sub(camera->position, scaledForward, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_A) == GLFW_PRESS) {
        vec3 scaledRight;
        glm_vec3_scale(flatRight, speed, scaledRight);
        glm_vec3_sub(camera->position, scaledRight, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_D) == GLFW_PRESS) {
        vec3 scaledRight;
        glm_vec3_scale(flatRight, speed, scaledRight);
        glm_vec3_add(camera->position, scaledRight, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        vec3 scaledUp;
        glm_vec3_scale((vec3) {0, 0, 1}, speed, scaledUp);
        glm_vec3_add(camera->position, scaledUp, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        vec3 scaledUp;
        glm_vec3_scale((vec3) {0, 0, 1}, speed, scaledUp);
        glm_vec3_sub(camera->position, scaledUp, camera->position);
        didMove = 1;
    }

    if (didMove) {
        resetStorageImage(vko);
    }
}

void initializeCamera(vk_context *vko);

void mainLoop(vk_context *vko) {
    uint32_t currentFrame = 0;

    vko->frameCount = 0;
    vko->totalFrameCount = 0;

    double xpos;
    double ypos;

    glfwGetCursorPos(vko->window, &xpos, &ypos);

    vko->cam.lastX = xpos;
    vko->cam.lastY = ypos;

    while (!glfwWindowShouldClose(vko->window)) {
        glfwPollEvents();
        if (glfwGetKey(vko->window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(vko->window, VK_TRUE);
        }
        if (glfwGetKey(vko->window, GLFW_KEY_R)) {
            initializeCamera(vko);
            updateCamera(vko);
        }
        vko->totalFrameCount++;
        updateCamera(vko);
        if (STOP && vko->frameCount >= MAX_FRAMES_RAN) {
            continue;
        }
        vko->frameCount++;
        updateUniforms(vko, currentFrame);
        drawFrame(vko, &currentFrame);
    }
}

void initializeScreenQuadBuffer(vk_context *vko) {
    // staging
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // recall that order of vertices is important as well. need to make sure right hand rule works
    // y is flipped, so -1 is top and 1 is bottom
    Vertex vertices[] = {     // don't need color
        { { -1.0f, -1.0f } }, // { 1.0f, 0.0f, 0.0f } }, // top left
        { {  1.0f,  1.0f } }, // { 0.0f, 1.0f, 0.0f } }, // bottom right
        { { -1.0f,  1.0f } }, // { 0.0f, 0.0f, 1.0f } }, // bottom left
        { {  1.0f,  1.0f } }, // { 1.0f, 0.0f, 0.0f } }, // bottom right
        { { -1.0f, -1.0f } }, // { 0.0f, 1.0f, 0.0f } }, // top left
        { {  1.0f, -1.0f } }, // { 0.0f, 0.0f, 1.0f } }  // top right
    };

    // staging buffer must be host visible and host coherent so it can be copied
    createBuffer(vko, sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);

    // copy data onto staging buffer
    void *data;
    vkMapMemory(vko->device, stagingMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vko->device, stagingMemory);

    // actual vertex buffer
    createBuffer(vko, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &vko->screenQuadBuffer, &vko->screenQuadBufferMemory);

    // copy from staging to test buffer
    copyBuffer(vko, stagingBuffer, vko->screenQuadBuffer, sizeof(vertices));

    // destroy staging buffer
    vkDestroyBuffer(vko->device, stagingBuffer, NULL);
    vkFreeMemory(vko->device, stagingMemory, NULL);
}

void updateViewMatrix(Camera *cam, vec3 target, mat4 out_view) {
    vec3 up_reference = {0.0f, 0.0f, 1.0f}; // Your world up direction
    
    // Generates the view matrix directly into out_view
    glm_lookat(cam->position, target, up_reference, out_view);
}

void updateCameraOrientation(Camera *cam, mat4 view) {
    // Extract the directional vectors from the view matrix rows
    cam->right[0] = view[0][0]; cam->right[1] = view[1][0]; cam->right[2] = view[2][0];
    cam->up[0]    = view[0][1]; cam->up[1]    = view[1][1]; cam->up[2]    = view[2][1];
    
    // The forward vector in a look-at view matrix points backwards (RH system)
    cam->forward[0] = -view[0][2]; cam->forward[1] = -view[1][2]; cam->forward[2] = -view[2][2];
    
    // Normalize to clean up any floating-point drift
    glm_vec3_normalize(cam->right);
    glm_vec3_normalize(cam->up);
    glm_vec3_normalize(cam->forward);
}

void lookat(vk_context *vko, vec3 target) {
    mat4 view;
    updateViewMatrix(&vko->cam, target, view);
    updateCameraOrientation(&vko->cam, view);
}

void initializeCamera(vk_context *vko) {
    vko->cam = (Camera) {
        .position = { 0, -9, 10 },
        .forward = { 0, 1, 0 },
        .right = { 1, 0, 0 },
        .up = { 0, 0, 1 }
    };

    // how to initialize pitch and yaw to reflect the direction vectors?
    vko->cam.yaw = GLM_PI_2; // think this should work
}

float randf() {
    return rand() / (float) RAND_MAX;
}

void initializeSpheresCPU(vk_context *vko) {
    for (int i = 0; i < 100; i++) {
        float r = 0.5 * (0.5 * (randf()) + 0.2);
        vko->spheres[vko->sphereCount++] = (Sphere) {
            .position = {5 * (2 * randf() - 1), 5 * (2 * randf() - 1), r},
            .radius = r,
            .mat = (Material) {
                .color = {randf(), randf(), randf()},
                .emissionColor = {1, 1, 1},
                .emissionStrength = 0.00,
                // .reflectivity = randf()
                .reflectivity = 0.00
            }
        };
        glm_vec3_copy(vko->spheres[vko->sphereCount - 1].mat.color, vko->spheres[vko->sphereCount - 1].mat.emissionColor);
    }

    for (int i = 0; i < 1; i++) {
        vko->emissiveSpheres[vko->emissiveSpheresCount++] = (Sphere) {
            // .position = {-100, 0, 120},
            // .radius = 40,
            .position = {21, 0, 4},
            .radius = 10,
            // .position = {5 * (2 * randf() - 1), 5 * (2 * randf() - 1), 0.5},
            // .radius = 0.15 * randf() + 0.10,
            .mat = (Material) {
                .color = {1, 1, 1},
                .emissionColor = {1, 1, 1},
                .emissionStrength = 10.0,
                .reflectivity = 0.00
            }
        };
    }
}

void initializeTrianglesCPU(vk_context *vko) {
    // triangle 1 for ceiling
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-5.5, 6, 6},
        .v1 = {6, -6, 6},
        .v2 = {-5.5, -6, 6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for ceiling
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-5.5, 6, 6},
        .v1 = {6, 6, 6},
        .v2 = {6, -6, 6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0,
            .reflectivity = 0.00
        }
    };

    // triangle 1 for left wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, 6, 5.5},
        .v1 = {-6, -6, 5.5},
        .v2 = {-6, -6, -5.5},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 0, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for left wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, -6, -5.5},
        .v1 = {-6, 6, -5.5},
        .v2 = {-6, 6, 5.5},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 0, 0},
            .emissionStrength = 0,
            .reflectivity = 0.00
        }
    };

    // triangle 1 for right wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {6, 6, 6},
        .v1 = {6, -6, 6},
        .v2 = {6, -6, -6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 1, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for right wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {6, -6, -6},
        .v1 = {6, 6, -6},
        .v2 = {6, 6, 6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 1, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 1 for back wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, 6, 6},
        .v1 = {6, 6, 6},
        .v2 = {6, 6, -6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 0, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for back wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {6, 6, -6},
        .v1 = {-6, 6, -6},
        .v2 = {-6, 6, 6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 0, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 1 for front wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, -6, 6},
        .v1 = {6, -6, 6},
        .v2 = {6, -6, -6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 0, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for front wall
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {6, -6, -6},
        .v1 = {-6, -6, -6},
        .v2 = {-6, -6, 6},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {0, 0, 0},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 1 for floor
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, 6, 0},
        .v1 = {6, -6, 0},
        .v2 = {-6, -6, 0},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0.00,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for floor
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-6, 6, 0},
        .v1 = {6, 6, 0},
        .v2 = {6, -6, 0},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0.00,
            .reflectivity = 0.00
        }
    };

    // // triangle 1 for floor light
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-5, 5, -9.9},
    //     .v1 = {5, -5, -9.9},
    //     .v2 = {-5, -5, -9.9},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {1, 1, 1},
    //         .emissionStrength = 1,
    //         .reflectivity = 0.5
    //     }
    // };

    // // triangle 2 for floor light
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-5, 5, -9.9},
    //     .v1 = {5, 5, -9.9},
    //     .v2 = {5, -5, -9.9},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {1, 1, 1},
    //         .emissionStrength = 1,
    //         .reflectivity = 0.5
    //     }
    // };

    // triangle 1 for ceiling light
    vko->emissiveTriangles[vko->emissiveTrianglesCount++] = (Triangle) {
        .v0 = {-50, 50, 100},
        .v1 = {50, -50, 100},
        .v2 = {-50, -50, 100},
        .mat = (Material) {
            .color = {1, 1, 1},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for ceiling light
    vko->emissiveTriangles[vko->emissiveTrianglesCount++] = (Triangle) {
        .v0 = {-50, 50, 100},
        .v1 = {50, 50, 100},
        .v2 = {50, -50, 100},
        .mat = (Material) {
            .color = {1, 1, 1},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0.0,
            .reflectivity = 0.00
        }
    };
}

void initializeHomogenousVolumesCPU(vk_context *vko) {
    vec3 absorptionCoefficient = {0.000, 0.000, 0.000};
    vec3 scatteringCoefficient = {0.95, 0.95, 0.95};
    vec3 extinctionCoefficient;
    glm_vec3_add(absorptionCoefficient, scatteringCoefficient, extinctionCoefficient);
    vko->homogenousVolumes[vko->homogenousVolumesCount++] = (HomogenousVolume) {
        .absorptionCoefficient = {absorptionCoefficient[0], absorptionCoefficient[1], absorptionCoefficient[2]},
        .scatteringCoefficient = {scatteringCoefficient[0], scatteringCoefficient[1], scatteringCoefficient[2]},
        .extinctionCoefficient = {extinctionCoefficient[0], extinctionCoefficient[1], extinctionCoefficient[2]},
        .minXYZ = {-5.999, -5.999, 0.001}, // bounding box
        .maxXYZ = {5.999, 5.999, 5.999}
    };
}

int main() {
    vk_context vko = {0};
    
    // a storage image has been set up via vko->storageImage and vko->storageImageMemory
    // as well as a compute pipeline via vko->computePipeline and vko->computePipelineLayout

    srand(0);

    // must do this first
    initializeSpheresCPU(&vko);
    initializeTrianglesCPU(&vko);
    initializeHomogenousVolumesCPU(&vko);
    initRenderer(&vko);
    initializeScreenQuadBuffer(&vko);
    initializeCamera(&vko);
    mainLoop(&vko);
    cleanupRenderer(&vko);
    return 0;
}
