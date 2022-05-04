#version 450

layout(location = 0) out vec3 outColor;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

vec2 positions[] = {
    vec2(0.0, 0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, -0.5)
};

vec3 colors[] = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
};

void main() {
    gl_Position = camera.projection * camera.view * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outColor = colors[gl_VertexIndex];
}
