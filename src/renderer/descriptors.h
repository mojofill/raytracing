#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "vk_types.h"

void createDescriptorSetLayout(vk_context *vko); // defines memory layout for vulkan
void createUniformBuffer(vk_context *vko);
void createDescriptorPool(vk_context *vko); // where uniforms are allocated
void createDescriptorSets(vk_context *vko); // where GPU resources like uniform buffers live
// other examples are storage buffers, texture samplers, etc

#endif