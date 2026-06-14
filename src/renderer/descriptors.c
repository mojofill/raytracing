#include "descriptors.h"
#include "buffer.h"
#include "objects/objects.h"

void createDescriptorSetLayout(vk_context *vko) {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // storage image
    VkDescriptorSetLayoutBinding imageLayoutBinding = {0};
    imageLayoutBinding.binding = 1;
    imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageLayoutBinding.descriptorCount = 1;
    imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // sphere buffer
    VkDescriptorSetLayoutBinding sphereBufferLayoutBinding = {0};
    sphereBufferLayoutBinding.binding = 2;
    sphereBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sphereBufferLayoutBinding.descriptorCount = 1;
    sphereBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // triangle buffer
    VkDescriptorSetLayoutBinding triangleBufferLayoutBinding = {0};
    triangleBufferLayoutBinding.binding = 3;
    triangleBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    triangleBufferLayoutBinding.descriptorCount = 1;
    triangleBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // homogenous volumes buffer
    VkDescriptorSetLayoutBinding homogenousVolumesBufferLayoutBinding = {0};
    homogenousVolumesBufferLayoutBinding.binding = 4;
    homogenousVolumesBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    homogenousVolumesBufferLayoutBinding.descriptorCount = 1;
    homogenousVolumesBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // homogenous volumes buffer
    VkDescriptorSetLayoutBinding emissiveSpheresBufferLayoutBinding = {0};
    emissiveSpheresBufferLayoutBinding.binding = 5;
    emissiveSpheresBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    emissiveSpheresBufferLayoutBinding.descriptorCount = 1;
    emissiveSpheresBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // homogenous volumes buffer
    VkDescriptorSetLayoutBinding emissiveTrianglesBufferLayoutBinding = {0};
    emissiveTrianglesBufferLayoutBinding.binding = 6;
    emissiveTrianglesBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    emissiveTrianglesBufferLayoutBinding.descriptorCount = 1;
    emissiveTrianglesBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding bindings[7] = { uboLayoutBinding, imageLayoutBinding, sphereBufferLayoutBinding, triangleBufferLayoutBinding, homogenousVolumesBufferLayoutBinding, emissiveSpheresBufferLayoutBinding, emissiveTrianglesBufferLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pBindings = bindings;
    layoutInfo.bindingCount = 7;

    if (vkCreateDescriptorSetLayout(vko->device, &layoutInfo, NULL, &vko->descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(1);
    }
}

void createUniformBuffer(vk_context *vko) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    // create one for each image in flight (ie uniform buffer count)
    for (int i = 0; i < UNIFORM_BUFFER_COUNT; i++) {
        createBuffer(vko, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vko->uniformBuffers[i], &vko->uniformBufferMemories[i]);
        vkMapMemory(vko->device, vko->uniformBufferMemories[i], 0, bufferSize, 0, &vko->uniformBuffersMapped[i]);
    }
}

void createDescriptorPool(vk_context *vko) {
    VkDescriptorPoolSize poolSizes[7]; // adjust for however many uniforms you want
    // only one singular uniform buffer object needs to be allocated
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT; 
    // come back here and see if i can remove max frames in flight from descriptor count and replace with 1 for these 2
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[5].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[6].descriptorCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 7;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = (uint32_t) MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(vko->device, &poolInfo, NULL, &vko->descriptorPool) != VK_SUCCESS) {
        printf("failed to create descriptor pool\n");
        exit(1);
    }
}

void createDescriptorSets(vk_context *vko) {
    VkDescriptorSetAllocateInfo allocInfo = {0};

    // need identical layouts for each MAX_FRAMES_IN_FLIGHT descriptor set
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {0};
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) layouts[i] = vko->descriptorSetLayout;

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vko->descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(vko->device, &allocInfo, vko->descriptorSets) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor sets\n");
        exit(1);
    }

    // populate descriptors
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // add buffer to descriptor set
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer = vko->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // doesnt matter what layout, so set it to be general layout
        imageInfo.imageView = vko->storageImageView;
        imageInfo.sampler = vko->storageSampler;

        VkDescriptorBufferInfo sphereBufferInfo = {0};
        sphereBufferInfo.buffer = vko->sphereBuffer;
        sphereBufferInfo.offset = 0;
        sphereBufferInfo.range = vko->sphereCount * sizeof(Sphere);

        VkDescriptorBufferInfo triangleBufferInfo = {0};
        triangleBufferInfo.buffer = vko->triangleBuffer;
        triangleBufferInfo.offset = 0;
        triangleBufferInfo.range = vko->triangleCount * sizeof(Triangle);

        VkDescriptorBufferInfo homogenousVolumesBufferInfo = {0};
        homogenousVolumesBufferInfo.buffer = vko->homogenousVolumesBuffer;
        homogenousVolumesBufferInfo.offset = 0;
        homogenousVolumesBufferInfo.range = vko->homogenousVolumesCount * sizeof(HomogenousVolume);

        VkDescriptorBufferInfo emissiveSpheresBufferInfo = {0};
        emissiveSpheresBufferInfo.buffer = vko->emissiveSpheresBuffer;
        emissiveSpheresBufferInfo.offset = 0;
        emissiveSpheresBufferInfo.range = vko->emissiveSpheresCount * sizeof(Sphere);

        VkDescriptorBufferInfo emissiveTrianglesBufferInfo = {0};
        emissiveTrianglesBufferInfo.buffer = vko->emissiveTrianglesBuffer;
        emissiveTrianglesBufferInfo.offset = 0;
        emissiveTrianglesBufferInfo.range = vko->emissiveTrianglesCount * sizeof(Triangle);

        // write to descriptor set
        VkWriteDescriptorSet descriptorWrites[7] = {0}; // can add more descriptor sets if necessary
        // such as samplers or storage buffers

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = vko->descriptorSets[i]; // write to this descriptor set
        descriptorWrites[0].dstBinding = 0; // write to this binding (binding = 0)
        descriptorWrites[0].dstArrayElement = 0; // can have descriptor arrays, but we are not, so put first; ie index=0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1; // only updating one descriptor
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = vko->descriptorSets[i];
        descriptorWrites[1].dstBinding = 1; // binding = 1 in the shader
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = vko->descriptorSets[i];
        descriptorWrites[2].dstBinding = 2; // binding = 2 in the shader
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &sphereBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = vko->descriptorSets[i];
        descriptorWrites[3].dstBinding = 3; // binding = 3 in the shader
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &triangleBufferInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = vko->descriptorSets[i];
        descriptorWrites[4].dstBinding = 4; // binding = 4 in the shader
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &homogenousVolumesBufferInfo;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = vko->descriptorSets[i];
        descriptorWrites[5].dstBinding = 5; // binding = 5 in the shader
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo = &emissiveSpheresBufferInfo;

        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].dstSet = vko->descriptorSets[i];
        descriptorWrites[6].dstBinding = 6; // binding = 6 in the shader
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].pBufferInfo = &emissiveTrianglesBufferInfo;

        vkUpdateDescriptorSets(vko->device, 7, descriptorWrites, 0, NULL);
    }
}
