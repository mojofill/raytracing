#ifndef VK_TYPES_H
#define VK_TYPES_H

#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#define GLFW_INCLUDE_NONE
// for protection against vulkan
#include <GLFW/glfw3.h>
#include "objects/objects.h"
#include "camera/camera.h"

#define MAX_FRAMES_IN_FLIGHT 2
#define UNIFORM_BUFFER_COUNT MAX_FRAMES_IN_FLIGHT

typedef struct Vertex {
    float pos[2];
    // float color[3]; // for now dont need color
} Vertex;

typedef struct UniformBufferObject {
    int width;
    int height;
    int sphereCount;
    int triangleCount;
    Camera cam;
    int numSamples; // per frame!
    int maxDepth;
    int frameCount; // for fragment shader
    int homogenousVolumesCount; // adding here for maximum safety of std430 padding
    float g; // Henyey-Greenstein asymmetry parameter. Range: [-1, 1]
} UniformBufferObject;

typedef struct vk_context {
    // GLFW + Vulkan initialization
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    uint32_t queueFamilyCount;
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Swapchain and swapchain support
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkSwapchainKHR swapchain;
    uint32_t swapchainImageCount;
    VkImage *swapchainImages;
    VkImageView *swapchainImageViews;

    // Vertex buffer information
    VkVertexInputBindingDescription bindingDesc; // works for now, in the future when using more types of vertices, need a more robust system
    VkVertexInputAttributeDescription attrDescs[1]; // come back here when adding more attributes to a vertex
    VkBuffer screenQuadBuffer;
    VkDeviceMemory screenQuadBufferMemory;

    // Render pass + graphics pipeline
    VkRenderPass renderPass;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipeline computePipeline;
    VkFramebuffer *swapchainFramebuffers;

    // Descriptors (uniforms)
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSets[UNIFORM_BUFFER_COUNT];

    // just for testing!
    float angle;
    int width;
    int height;

    int frameCount;

    // Uniform buffer
    VkBuffer uniformBuffers[UNIFORM_BUFFER_COUNT];
    VkDeviceMemory uniformBufferMemories[UNIFORM_BUFFER_COUNT];
    void *uniformBuffersMapped[UNIFORM_BUFFER_COUNT];

    // Storage image
    VkImage storageImage;
    VkImageView storageImageView;
    VkDeviceMemory storageImageMemory;
    VkSampler storageSampler;

    VkBuffer sphereBuffer;
    VkDeviceMemory sphereMemory;
    VkBuffer triangleBuffer;
    VkDeviceMemory triangleMemory;
    VkBuffer homogenousVolumesBuffer;
    VkDeviceMemory homogenousVolumesMemory;

    Sphere spheres[500];
    int sphereCount;
    Triangle triangles[500];
    int triangleCount;
    HomogenousVolume homogenousVolumes[500];
    int homogenousVolumesCount;
    Camera cam;

    // Dynamic pipeline bindings
    VkViewport viewport;
    VkRect2D scissor;

    // Synchronization objects
    VkSemaphore *imageAvailableSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
    uint8_t framebufferResized;

    // Commands
    VkCommandPool commandPool;
    VkCommandBuffer *commandBuffers;
} vk_context;

#endif