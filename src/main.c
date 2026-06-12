#include "renderer/renderer.h"
#include "objects/sphere.h"
#include "objects/triangle.h"
#include "camera/camera.h"
#include "cglm/cglm.h"

#define NUM_SAMPLES 1
#define MAX_DEPTH 20
#define MAX_FRAMES_RAN 100
#define STOP 0

void updateUniforms(vk_context *vko, uint32_t currentFrame) {
    UniformBufferObject ubo = {0};
    ubo.width = vko->width;
    ubo.height = vko->height;
    ubo.sphereCount = vko->sphereCount;
    ubo.triangleCount = vko->triangleCount;
    ubo.cam = vko->cam;
    ubo.numSamples = NUM_SAMPLES;
    ubo.maxDepth = MAX_DEPTH;
    ubo.frameCount = vko->frameCount;
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

    // printf("yaw = %f, pitch = %f, xoffset = %f, yoffset = %f\n", camera->yaw, camera->pitch, xoffset, yoffset);

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

    if (glfwGetKey(vko->window, GLFW_KEY_W) == GLFW_PRESS) {
        vec3 scaledForward;
        glm_vec3_scale(camera->forward, speed, scaledForward);
        glm_vec3_add(camera->position, scaledForward, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_S) == GLFW_PRESS) {
        vec3 scaledForward;
        glm_vec3_scale(camera->forward, speed, scaledForward);
        glm_vec3_sub(camera->position, scaledForward, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_A) == GLFW_PRESS) {
        vec3 scaledRight;
        glm_vec3_scale(camera->right, speed, scaledRight);
        glm_vec3_sub(camera->position, scaledRight, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_D) == GLFW_PRESS) {
        vec3 scaledRight;
        glm_vec3_scale(camera->right, speed, scaledRight);
        glm_vec3_add(camera->position, scaledRight, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        vec3 scaledUp;
        glm_vec3_scale(camera->up, speed, scaledUp);
        glm_vec3_add(camera->position, scaledUp, camera->position);
        didMove = 1;
    }
    if (glfwGetKey(vko->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        vec3 scaledUp;
        glm_vec3_scale(camera->up, speed, scaledUp);
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
        if (STOP && vko->frameCount >= MAX_FRAMES_RAN) {
            continue;
        }
        vko->frameCount++;
        updateUniforms(vko, currentFrame);
        drawFrame(vko, &currentFrame);
        updateCamera(vko);
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
        .position = { 0, -9, 0 },
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
    for (int i = 0; i < 200; i++) {
        float r = 0.5 * (0.5 * (randf()) + 0.2);
        vko->spheres[vko->sphereCount++] = (Sphere) {
            .position = {5 * (2 * randf() - 1), 5 * (2 * randf() - 1), r},
            .radius = r,
            .mat = (Material) {
                .color = {randf(), randf(), randf()},
                .emissionColor = {1, 1, 1},
                .emissionStrength = 0.00,
                .reflectivity = randf()
            }
        };
    }
}

void initializeTrianglesCPU(vk_context *vko) {
    // triangle 1 for ceiling
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, 10, 10},
    //     .v1 = {10, -10, 10},
    //     .v2 = {-10, -10, 10},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {1, 1, 1},
    //         .emissionStrength = 0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 2 for ceiling
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, 10, 10},
    //     .v1 = {10, 10, 10},
    //     .v2 = {10, -10, 10},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {1, 1, 1},
    //         .emissionStrength = 0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 1 for left wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, 10, 10},
    //     .v1 = {-10, -10, 10},
    //     .v2 = {-10, -10, -10},
    //     .mat = (Material) {
    //         .color = {1, 0, 1},
    //         .emissionColor = {1, 0, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 2 for left wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, -10, -10},
    //     .v1 = {-10, 10, -10},
    //     .v2 = {-10, 10, 10},
    //     .mat = (Material) {
    //         .color = {1, 0, 1},
    //         .emissionColor = {1, 0, 0},
    //         .emissionStrength = 0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 1 for right wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {10, 10, 10},
    //     .v1 = {10, -10, 10},
    //     .v2 = {10, -10, -10},
    //     .mat = (Material) {
    //         .color = {0, 1, 1},
    //         .emissionColor = {0, 1, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 2 for right wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {10, -10, -10},
    //     .v1 = {10, 10, -10},
    //     .v2 = {10, 10, 10},
    //     .mat = (Material) {
    //         .color = {0, 1, 1},
    //         .emissionColor = {0, 1, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 1 for back wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, 10, 10},
    //     .v1 = {10, 10, 10},
    //     .v2 = {10, 10, -10},
    //     .mat = (Material) {
    //         .color = {0.5, 0.5, 0.5},
    //         .emissionColor = {0, 0, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 2 for back wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {10, 10, -10},
    //     .v1 = {-10, 10, -10},
    //     .v2 = {-10, 10, 10},
    //     .mat = (Material) {
    //         .color = {0.5, 0.5, 0.5},
    //         .emissionColor = {0, 0, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 1 for front wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {-10, -10, 10},
    //     .v1 = {10, -10, 10},
    //     .v2 = {10, -10, -10},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {0, 0, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // // triangle 2 for front wall
    // vko->triangles[vko->triangleCount++] = (Triangle) {
    //     .v0 = {10, -10, -10},
    //     .v1 = {-10, -10, -10},
    //     .v2 = {-10, -10, 10},
    //     .mat = (Material) {
    //         .color = {1, 1, 1},
    //         .emissionColor = {0, 0, 0},
    //         .emissionStrength = 0.0,
    //         .reflectivity = 1.00
    //     }
    // };

    // triangle 1 for floor
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-100, 100, 0},
        .v1 = {100, -100, 0},
        .v2 = {-100, -100, 0},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0,
            .reflectivity = 0
        }
    };

    // triangle 2 for floor
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-100, 100, 0},
        .v1 = {100, 100, 0},
        .v2 = {100, -100, 0},
        .mat = (Material) {
            .color = {0.5, 0.5, 0.5},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 0,
            .reflectivity = 0
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
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-50, 50, 100},
        .v1 = {50, -50, 100},
        .v2 = {-50, -50, 100},
        .mat = (Material) {
            .color = {1, 1, 1},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 1.0,
            .reflectivity = 0.00
        }
    };

    // triangle 2 for ceiling light
    vko->triangles[vko->triangleCount++] = (Triangle) {
        .v0 = {-50, 50, 100},
        .v1 = {50, 50, 100},
        .v2 = {50, -50, 100},
        .mat = (Material) {
            .color = {1, 1, 1},
            .emissionColor = {1, 1, 1},
            .emissionStrength = 1.0,
            .reflectivity = 0.00
        }
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
    initRenderer(&vko);
    initializeScreenQuadBuffer(&vko);
    initializeCamera(&vko);
    mainLoop(&vko);
    cleanupRenderer(&vko);
    return 0;
}
