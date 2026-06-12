#version 450

layout(location = 0) in vec2 outNDC;
layout(location = 0) out vec4 outColor;
struct Camera {
    vec3 position;
    float padA;
    vec3 forward;
    float padB;
    vec3 right;
    float padC;
    vec3 up;
    float padD;

    float yaw;
    float pitch;
    float lastX;
    float lastY;
};
layout(binding = 0) uniform UniformBufferObject {
    int width;
    int height;
    int sphereCount;
    int triangleCount;
    Camera camera;
    int NUM_SAMPLES;
    int MAX_DEPTH;
    int frameCount;
};
layout(binding = 1, rgba32f) uniform readonly image2D outputImage;

void main() {
    vec2 uv = outNDC * 0.5 + 0.5;
    ivec2 imgSize = imageSize(outputImage);
    ivec2 texCoord = ivec2(uv * vec2(imgSize));

    // gamma correction
    vec3 color = imageLoad(outputImage, texCoord).rgb / float(NUM_SAMPLES * frameCount);
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
    if (texCoord.y == 0) outColor = vec4(0);
}
