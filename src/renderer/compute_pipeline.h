#ifndef COMPUTE_PIPELINE_H
#define COMPUTE_PIPELINE_H

#include "vk_types.h"
#include "graphics_pipeline.h"

void createComputePipeline(vk_context *vko, const char *compute_path);
void createImageMemoryBarrier(vk_context *vko, VkCommandBuffer commandBuffer);

#endif