#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform sampler2D textureSampler;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    float time;
} transform;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 texColor = texture(textureSampler, fragTexCoords);

    vec3 red = vec3(1.0, 0.0, 0.0);
    vec3 white = vec3(1.0, 1.0, 1.0);

    vec3 fogColor = mix(red, white, (sin(-transform.time) + 1.0) / 2.0);
    vec3 color = mix(fogColor, texColor.rgb, (cos(transform.time) + 1.0) / 2.0);

    fragmentColor = vec4(color, texColor.a);
}
