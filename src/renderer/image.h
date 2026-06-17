#ifndef IMAGE_H
#define IMAGE_H

#include "vk_types.h"
#include "buffer.h"

void createImage(vk_context *vko, uint32_t width, uint32_t height, uint32_t depth, VkImageType imageType, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageLayout finalLayout, VkImage *pImage, VkDeviceMemory *pImageMemory);
void createImageView(vk_context *vko, VkImage image, VkImageViewType imageViewType, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView *imageView);
void createSampler(vk_context *vko, VkSampler *pSampler);
void transitionImageLayout(vk_context *vko, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

#endif